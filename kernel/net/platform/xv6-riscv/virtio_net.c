#include "platform.h"
#include "memlayout.h"
#include "virtio.h"

#include "util.h"
#include "net.h"
#include "ether.h"

#define R(r) ((volatile uint32_t *)(VIRTIO1 + (r)))

/*
 * virtq
 */

struct virtq {
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
    int num;
    int last_used_idx;
    char *free;
};

void
virtq_init(struct virtq *vq, int sel, int num)
{
    uint32_t max;

    // select queue
    *R(VIRTIO_MMIO_QUEUE_SEL) = sel;

    // ensure selected queue is not in use.
    if (*R(VIRTIO_MMIO_QUEUE_READY)) {
        panic("queue already in use");
    }

    // check maximum queue size.
    max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0) {
        panic("queue not available");
    }
    if (max < num) {
        panic("queue too short");
    }

    // allocate and zero queue memory.
    vq->desc = kalloc();
    vq->avail = kalloc();
    vq->used = kalloc();
    vq->free = kalloc();
    if (!vq->desc || !vq->avail || !vq->used || !vq->free) {
        panic("kalloc failed");
    }
    memset(vq->desc, 0, PGSIZE);
    memset(vq->avail, 0, PGSIZE);
    memset(vq->used, 0, PGSIZE);
    memset(vq->free, 0,  PGSIZE);

    // set queue size.
    vq->num = num;
    *R(VIRTIO_MMIO_QUEUE_NUM) = vq->num;

    // write physical addresses.
    *R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64_t)vq->desc;
    *R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64_t)vq->desc >> 32;
    *R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64_t)vq->avail;
    *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64_t)vq->avail >> 32;
    *R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64_t)vq->used;
    *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64_t)vq->used >> 32;

    // queue is ready.
    *R(VIRTIO_MMIO_QUEUE_READY) = 1;

    // all descriptors start out unused.
    for (int i = 0; i < vq->num; i++) {
        vq->free[i] = 1;
    }
    vq->last_used_idx = 0;
}

// find a free descriptor, mark it non-free, return its index.
int
virtq_alloc_desc(struct virtq *vq)
{
    for (int i = 0; i < vq->num; i++) {
        if(vq->free[i]){
            vq->free[i] = 0;
            return i;
        }
    }
    return -1;
}

// mark a descriptor as free.
void
virtq_free_desc(struct virtq *vq, int i)
{
    if (i >= vq->num) {
        panic("virtq_free_desc: invalid index");
    }
    if (vq->free[i]) {
        panic("virtq_free_desc: freeing free descriptor");
    }
    vq->desc[i].addr = 0;
    vq->desc[i].len = 0;
    vq->desc[i].flags = 0;
    vq->desc[i].next = 0;
    vq->free[i] = 1;
}

/*
 * virtio-net
 */

#define RXQ 0
#define TXQ 1

#define QSIZE NUM
#define RX_BUF_SIZE 2048

#define VIRTIO_MMIO_CONFIG 0x100
#define VIRTIO_NET_F_CSUM 0
#define VIRTIO_NET_F_GUEST_CSUM 1
#define VIRTIO_NET_F_MAC 5
#define VIRTIO_F_VERSION_1 32

struct virtio_net {
    struct net_device *dev;
    uint32_t status;
    uint64_t features;
    struct spinlock lock;
    struct virtq rx_q;
    struct virtq tx_q;
    char rx_bufs[QSIZE][RX_BUF_SIZE];
    char tx_bufs[QSIZE][RX_BUF_SIZE];
    char tx_status[QSIZE];
} _nic0;

struct virtio_net_hdr {
#define VIRTIO_NET_HDR_F_NEEDS_CSUM 1
#define VIRTIO_NET_HDR_F_DATA_VALID 2
#define VIRTIO_NET_HDR_F_RSC_INFO 4
    uint8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE 0
#define VIRTIO_NET_HDR_GSO_TCPV4 1
#define VIRTIO_NET_HDR_GSO_UDP 3
#define VIRTIO_NET_HDR_GSO_TCPV6 4
#define VIRTIO_NET_HDR_GSO_ECN 0x80
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;
};

#define PRIV(x) ((struct virtio_net *)(x)->priv)

static int
virtio_net_open(struct net_device *dev)
{
    struct virtio_net *nic = PRIV(dev);

    acquire(&nic->lock);

    // set receive buffers
    for (int i = 0; i < QSIZE; i++) {
        nic->rx_q.desc[i].addr = (uint64)nic->rx_bufs[i];
        nic->rx_q.desc[i].len = RX_BUF_SIZE;
        nic->rx_q.desc[i].flags = VRING_DESC_F_WRITE;
        nic->rx_q.avail->ring[i] = i;
        nic->rx_q.free[i] = 0;
    }
    __sync_synchronize();
    nic->rx_q.avail->idx = QSIZE;
    nic->rx_q.last_used_idx = 0;

    // tell device we're completely ready.
    nic->status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    // Notify the device of new RX buffers.
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = RXQ;

    release(&nic->lock);

    return 0;
}

static int
virtio_net_close(struct net_device *dev)
{
    struct virtio_net *nic = PRIV(dev);

    acquire(&nic->lock);

    nic->status = *R(VIRTIO_MMIO_STATUS);

    // clear DRIVER_OK bit
    nic->status &= ~VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    release(&nic->lock);
    return 0;
}

static ssize_t
virtio_net_write(struct net_device *dev, const uint8_t *data, size_t len)
{
    struct virtio_net *nic = PRIV(dev);
    int idx;
    void *buf;
    struct virtio_net_hdr *hdr;

    if (len > RX_BUF_SIZE) {
        return -1;
    }

    acquire(&nic->lock);

    // Allocate descriptor
    idx = virtq_alloc_desc(&nic->tx_q);
    if (idx == -1) {
        release(&nic->lock);
        return -1;
    }

    buf = nic->tx_bufs[idx];
    hdr = buf;

    // Setup virtio-net header.
    hdr->flags = 0; // assume the packet is completely checksummed
    hdr->gso_type = VIRTIO_NET_HDR_GSO_NONE;
    hdr->hdr_len = 0; // unused
    hdr->gso_size = 0; // unused
    hdr->csum_start = 0; // unused
    hdr->csum_offset = 0; // unused
    hdr->num_buffers = 0; // driver must set num_buffers to 0
    memmove(nic->tx_bufs[idx] + sizeof(*hdr), data, len);

    // Configure descriptor
    nic->tx_q.desc[idx].addr = (uint64_t)nic->tx_bufs[idx];
    nic->tx_q.desc[idx].len = sizeof(*hdr) + len;
    nic->tx_q.desc[idx].flags = 0; // read by device

    // Deploy descriptor in the available ring.
    nic->tx_q.avail->ring[nic->tx_q.avail->idx % nic->tx_q.num] = idx;
    __sync_synchronize();
    nic->tx_q.avail->idx++;
    __sync_synchronize();

    release(&nic->lock);

    // Notify the device of a new TX packet.
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = TXQ;

    return len;
}

static int
virtio_net_transmit(struct net_device *dev, uint16_t type, const uint8_t *packet, size_t len, const void *dst)
{
  return ether_transmit_helper(dev, type, packet, len, dst, virtio_net_write);;
}

static ssize_t
virtio_net_read(struct net_device *dev, uint8_t *buf, size_t size)
{
    struct virtio_net *nic = PRIV(dev);

    int ring_idx = nic->rx_q.last_used_idx % nic->rx_q.num;
    int idx = nic->rx_q.used->ring[ring_idx].id;
    int len = nic->rx_q.used->ring[ring_idx].len;

    // The actual data starts after the virtio-net header.
    int hdrlen = sizeof(struct virtio_net_hdr);
    memcpy(buf, nic->rx_bufs[idx] + hdrlen, len - hdrlen);

    // Recycle the receive buffer descriptor.
    nic->rx_q.desc[idx].addr = (uint64_t)nic->rx_bufs[idx];
    nic->rx_q.desc[idx].len = RX_BUF_SIZE;

    // Return the descriptor to the available ring.
    nic->rx_q.avail->ring[nic->rx_q.avail->idx % nic->rx_q.num] = idx;

    return len - hdrlen;
}

void
virtio_net_intr(void)
{
    struct virtio_net *nic = &_nic0;

    acquire(&nic->lock);

    // Acknowledge the interrupt and clear the status by writing it back.
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
    __sync_synchronize();

    // Process completed descriptors from the tx used ring.
    while (nic->tx_q.last_used_idx != nic->tx_q.used->idx) {
        int ring_idx = nic->tx_q.last_used_idx % nic->tx_q.num;
        int idx = nic->tx_q.used->ring[ring_idx].id;
        virtq_free_desc(&nic->tx_q, idx);
        nic->tx_q.last_used_idx++;
    }

    // Process incoming packets from the rx used ring.
    while (nic->rx_q.last_used_idx != nic->rx_q.used->idx) {
        ether_input_helper(nic->dev, virtio_net_read);
        __sync_synchronize();
        nic->rx_q.avail->idx++;
        nic->rx_q.last_used_idx++;
    }
    __sync_synchronize();

    release(&nic->lock);

    // Notify the device of new RX buffers.
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = RXQ;

    intr_raise_irq(INTR_IRQ_SOFTIRQ);
}

struct net_device_ops virtio_net_ops = {
    .open = virtio_net_open,
    .close = virtio_net_close,
    .transmit = virtio_net_transmit,
};

#include "ip.h"

void
virtio_net_init(void)
{
    struct virtio_net *nic = &_nic0;
    uint8_t addr[6];
    struct net_device *dev;
    char mac[ETHER_ADDR_STR_LEN];

    initlock(&nic->lock, "virtio-net");

    // find virtio-net device
    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        *R(VIRTIO_MMIO_VERSION) != 2 ||
        *R(VIRTIO_MMIO_DEVICE_ID) != 1 || // network device
        *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
        panic("virtio-net: device not found");
    }

    debugf("device found");

    // reset device
    nic->status = 0;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    // set ACKNOWLEDGE status bit
    nic->status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    // set DRIVER status bit
    nic->status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    // negotiate features
    nic->features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    nic->features &= ~(1ULL << VIRTIO_RING_F_EVENT_IDX);
    nic->features &= ~(1ULL << VIRTIO_NET_F_CSUM);
    nic->features |= (1ULL << VIRTIO_NET_F_GUEST_CSUM);
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = nic->features;

    // tell device that feature negotiation is complete.
    nic->status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    // re-read status to ensure FEATURES_OK is set.
    if (!(*R(VIRTIO_MMIO_STATUS) & VIRTIO_CONFIG_S_FEATURES_OK)) {
        panic("virtio-net: FEATURES_OK failed");
    }

    // read MAC address
    if (nic->features & (1 << VIRTIO_NET_F_MAC)) {
        for (int i = 0; i < 6; i++) {
            addr[i] = *(volatile uint8_t *)((uint64)(VIRTIO1 + VIRTIO_MMIO_CONFIG + i));
        }
    } else {
        debugf("device does not provide a MAC address");
    }

    // initialize TXQ
    virtq_init(&nic->tx_q, TXQ, QSIZE);

    // initialize RXQ
    virtq_init(&nic->rx_q, RXQ, QSIZE);
    for (int i = 0; i < QSIZE; i++) {
        nic->rx_q.desc[i].addr = (uint64_t)nic->rx_bufs[i];
        nic->rx_q.desc[i].len = RX_BUF_SIZE;
        nic->rx_q.desc[i].flags = VRING_DESC_F_WRITE;
        nic->rx_q.avail->ring[i] = i;
        nic->rx_q.free[i] = 0;
    }
    __sync_synchronize();
    nic->rx_q.avail->idx = QSIZE;

    // tell device we're completely ready.
    nic->status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = nic->status;

    // setup device driver structure
    dev = net_device_alloc();
    if (!dev) {
        errorf("net_device_alloc() failure");
        return;
    }
    ether_setup_helper(dev);
    memcpy(dev->addr, addr, sizeof(addr));
    dev->priv = nic;
    dev->ops = &virtio_net_ops;
    if (net_device_register(dev) == -1) {
        errorf("net_device_register() failure");
        memory_free(dev);
        return;
    }
    nic->dev = dev;

    debugf("initialized, addr=%s", ether_addr_ntop(dev->addr, mac, sizeof(mac)));
}
