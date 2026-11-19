/********************************************************************************
 * stream.h — CH569 firmware variant 1 ("stream", specification D-031): the
 * RP2350 sends packets on the 8-bit HSPI bus, the CH569 receives them into a
 * RAMX ring and streams the payload to the host on a vendor bulk IN endpoint.
 *
 * USB (high speed): 1a86:fe2c, product "rp569 CH569 stream", serial = chip UID,
 * one vendor interface (class 0xFF) with bulk IN 0x82 (512 B packets).
 * Ring: STREAM_RING_SLOTS x 4 KB in RAMX; the host must keep IN requests flowing (python/rp569/stream.py).
 * Vendor requests (bmRequestType 0xC0 / 0x40, interface-independent):
 *   STREAM_REQ_STATS  0x60 IN  : struct stream_stats (little endian)
 *   STREAM_REQ_RESET  0x61 OUT : clear the statistics and the ring
 *   STREAM_REQ_EDGE   0x62 OUT : wValue 0 = sample HD on the rising HRCLK edge, 1 = falling
 *******************************************************************************/
#ifndef STREAM_H_
#define STREAM_H_

#include "CH56x_common.h"

#define STREAM_RING_SLOTS     20          /* 20 x 4096 B HSPI packets in RAMX (~5 ms at 16 MB/s) */
#define STREAM_SLOT_SIZE      4096

#define STREAM_REQ_STATS      0x60
#define STREAM_REQ_RESET      0x61
#define STREAM_REQ_EDGE       0x62

typedef struct __attribute__((packed))
{
    UINT32 packets;        /* HSPI packets received                         */
    UINT32 bytes_in;       /* payload bytes received                        */
    UINT32 crc_errors;     /* packets flagged RB_HSPI_CRC_ERR (dropped)     */
    UINT32 seq_errors;     /* packets flagged RB_HSPI_NUM_MIS (data kept)   */
    UINT32 overflows;      /* packets dropped: ring full                    */
    UINT32 fifo_ov;        /* RB_HSPI_IF_FIFO_OV events                     */
    UINT32 bytes_out;      /* bytes sent to the host                        */
    UINT32 usb_packets;    /* bulk IN packets sent                          */
} stream_stats_t;

extern volatile stream_stats_t stream_stats;
extern UINT8 stream_rx_edge;      /* 0 rising, 1 falling */

void   usb_stream_init(void);     /* USB2 high-speed device, vendor bulk IN       */
UINT8  usb_stream_configured(void);
UINT8  usb_stream_in_busy(void);
void   usb_stream_in_send(UINT8 *buf, UINT16 len);
void   usb_stream_poll(void);     /* EP0 + bulk IN completion (polled, no IRQ)    */
void   stream_reset(void);        /* statistics + ring                            */
void   hspi_rx_init(void);
void   hspi_rx_set_edge(UINT8 falling);

#endif /* STREAM_H_ */
