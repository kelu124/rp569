/********************************************************************************
 * main.c — CH569 firmware variant 1 ("stream"), see stream.h and D-031.
 *
 * HSPI receiver (8-bit, "down" end) <- RP2350 PIO sender; packets land in a ring
 * of 16 x 4 KB RAMX slots (dual-DMA register pair re-armed after every packet);
 * the main loop sends the filled slots to the host on bulk IN 0x82 (USB2 high
 * speed, polled). RP2350 link UART1 (PA7/PA8): every byte echoed back (link
 * test), "\x01MSTR" announced at boot (RP2350: "CH569 mode: stream").
 * Packets failing the CRC (CRC-16/USB, as the RP2350 computes it) are dropped and counted, never forwarded.
 * RGB LED (low = on): R = running, G = USB configured, B = toggles every 256 HSPI packets.
 *******************************************************************************/
#include "CH56x_common.h"
#include "stream.h"
#include "chip_uid.h"
#include "fw_version.h"
#include <string.h>
#include <stdio.h>

#define LINK_BAUD     115200
#define LED_R         GPIO_Pin_0
#define LED_G         GPIO_Pin_1
#define LED_B         GPIO_Pin_2
#define TRASH         0xFF
#ifndef STREAM_DEBUG_STAT
#define STREAM_DEBUG_STAT 0     /* 1 = STAT/DATA lines on the link about once a second (costs packets) */
#endif

__attribute__((aligned(16))) static UINT8 ring[STREAM_RING_SLOTS][STREAM_SLOT_SIZE] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) static UINT8 trash[STREAM_SLOT_SIZE] __attribute__((section(".DMADATA")));

volatile stream_stats_t stream_stats;
UINT8 stream_rx_edge = 0;

/* Filled slots, oldest first (FIFO), and free slots (stack). */
static UINT8  fifo[STREAM_RING_SLOTS];
static UINT16 fifo_len[STREAM_RING_SLOTS];
static UINT8  fifo_head, fifo_count;
static UINT8  free_stack[STREAM_RING_SLOTS];
static UINT8  free_count;
static UINT8  reg_slot[2];            /* slot behind RX_ADDR0 / RX_ADDR1 (or TRASH) */
static UINT8  rx_tog;                 /* register the next packet goes to */
static UINT16 usb_off;                /* bytes of the oldest slot already sent */
static UINT8  release_slot = TRASH;   /* slot whose last USB packet is in flight */
static UINT16 len_min = 0xFFFF, len_max, len_r0, len_r1;   /* receive length diagnostics (STAT line) */
static UINT8  last_done, last_tog_after;
static UINT8  last_slot = TRASH;

/* ------------------------------------------------------------------ link UART1 (RP2350) */
static void link_init(void)
{
    UINT32 x = (10 * FREQ_SYS * 2 / 16 / LINK_BAUD + 5) / 10;
    R8_UART1_DIV = 1;
    R16_UART1_DL = x;
    R8_UART1_FCR = RB_FCR_FIFO_TRIG | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
    R8_UART1_LCR = RB_LCR_WORD_SZ;
    R8_UART1_IER = RB_IER_TXD_EN;
    GPIOA_SetBits(GPIO_Pin_8);
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_PU_NSMT);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_Slowascent_PP_8mA);
}

static void link_send(const char *s)
{
    for(; *s; s++)
    {
        while(R8_UART1_TFC == UART_FIFO_SIZE)
            ;
        R8_UART1_THR = (UINT8)*s;
    }
}

static void link_poll(void)               /* echo, as the main firmware does */
{
    while(R8_UART1_RFC && R8_UART1_TFC < UART_FIFO_SIZE)
        R8_UART1_THR = R8_UART1_RBR;
}

/* ------------------------------------------------------------------ ring */
static UINT8 slot_take(void)
{
    return free_count ? free_stack[--free_count] : TRASH;
}

static UINT8 *slot_addr(UINT8 s)
{
    return s == TRASH ? trash : ring[s];
}

void stream_reset(void)
{
    UINT8 i;
    memset((void *)&stream_stats, 0, sizeof(stream_stats));
    fifo_head = fifo_count = 0;
    usb_off = 0;
    release_slot = TRASH;
    free_count = 0;
    for(i = 0; i < STREAM_RING_SLOTS; i++)
        free_stack[free_count++] = STREAM_RING_SLOTS - 1 - i;
    /* the two registers keep their current slots: put those back in use */
    for(i = 0; i < 2; i++)
        if(reg_slot[i] != TRASH)
        {
            UINT8 j;
            for(j = 0; j < free_count; j++)
                if(free_stack[j] == reg_slot[i])
                {
                    free_stack[j] = free_stack[--free_count];
                    break;
                }
        }
    /* registers left on the trash slot (ring full when the last run ended): back on real slots, or the first two
     * packets of the next run are dropped */
    if(reg_slot[0] == TRASH && free_count)
    {
        reg_slot[0] = slot_take();
        R32_HSPI_RX_ADDR0 = (UINT32)slot_addr(reg_slot[0]);
    }
    if(reg_slot[1] == TRASH && free_count)
    {
        reg_slot[1] = slot_take();
        R32_HSPI_RX_ADDR1 = (UINT32)slot_addr(reg_slot[1]);
    }
}

/* ------------------------------------------------------------------ HSPI receiver */
void hspi_rx_set_edge(UINT8 falling)
{
    stream_rx_edge = falling;
    if(falling)
        R8_HSPI_AUX |= RB_HSPI_RCK_MOD;
    else
        R8_HSPI_AUX &= ~RB_HSPI_RCK_MOD;
}

void hspi_rx_init(void)
{
    /* CH569 transmitter-side lines are outputs (HTCLK PA11, HTREQ PA9, HTVLD PA21: RP2350 inputs), HTACK (PA10)
     * is the receiver's ready output to the RP2350 (GPIO19). HD0..7 and HRCLK/HRACT/HRVLD stay inputs. */
    R32_PA_DIR |= (1 << 9) | (1 << 11) | (1 << 21) | (1 << 10);
    mDelayuS(2);                              /* GPIO settle before HSPI configuration (BSP note) */

    R8_HSPI_CTRL = RB_HSPI_ALL_CLR | RB_HSPI_TRX_RST;
    R8_HSPI_CFG = RB_HSPI_DAT8_MOD | RB_HSPI_RX_TOG_EN | RB_HSPI_DUALDMA;   /* down end, 8-bit, no HW ack */
    R8_HSPI_AUX = RB_HSPI_REQ_FT;
    hspi_rx_set_edge(stream_rx_edge);
    R8_HSPI_INT_EN = 0;                       /* polled */
    R16_HSPI_BURST_CFG = 0;

    reg_slot[0] = reg_slot[1] = TRASH;        /* stream_reset() builds the free list first */
    stream_reset();                           /* free list + both RX registers on real slots */
    R16_HSPI_RX_LEN0 = 0;                     /* 0 = up to 4096 bytes */
    R16_HSPI_RX_LEN1 = 0;
    R8_HSPI_RX_SC = 0;                        /* sequence 0, register 0 */
    rx_tog = 0;

    R8_HSPI_INT_FLAG = 0x0F;
    R8_HSPI_CTRL = RB_HSPI_ENABLE | RB_HSPI_DMA_EN;
}

static void hspi_rx_poll(void)
{
    UINT8 fg = R8_HSPI_INT_FLAG;

    if(fg & RB_HSPI_IF_FIFO_OV)
    {
        stream_stats.fifo_ov++;
        R8_HSPI_INT_FLAG = RB_HSPI_IF_FIFO_OV;
    }
    if(!(fg & RB_HSPI_IF_R_DONE))
        return;

    {
        UINT8  done = rx_tog;                 /* the packet went to the register selected before it */
        UINT8  st = R8_HSPI_RTX_STATUS;
        UINT16 len = done ? R16_HSPI_RX_LEN1 : R16_HSPI_RX_LEN0;
        UINT8  s = reg_slot[done];
        last_slot = s;

        len_r0 = R16_HSPI_RX_LEN0;
        len_r1 = R16_HSPI_RX_LEN1;
        if(len < len_min) len_min = len;
        if(len > len_max) len_max = len;
        last_done = done;
        /* RX_LEN0/1 read 0 or 1 on board #1 (CRC never valid yet, or another meaning): the variant's packets have
         * a fixed 4096-byte payload, the host checks every word. Diagnostics in the STAT line. */
        len = STREAM_SLOT_SIZE;
        stream_stats.packets++;
        stream_stats.bytes_in += len;
        if(st & RB_HSPI_CRC_ERR)
            stream_stats.crc_errors++;
        /* Sequence check: informational. With CRC-16/USB the CRC is clean, but RB_HSPI_NUM_MIS stays set on most
         * packets whatever RX_SC is set to (board #1, 2026-11-19, open in todo); the host checks every word. */
        if(st & RB_HSPI_NUM_MIS)
            stream_stats.seq_errors++;
        if(s == TRASH)
            stream_stats.overflows++;
        else if(st & RB_HSPI_CRC_ERR)
            free_stack[free_count++] = s;     /* corrupted on the bus (CRC-16/USB): dropped, the host sees a gap */
        else
        {
            UINT8 tail = (fifo_head + fifo_count) % STREAM_RING_SLOTS;
            fifo[tail] = s;
            fifo_len[tail] = len;
            fifo_count++;
        }
        /* re-arm the register just used with a free slot (the trash slot when the ring is full) */
        reg_slot[done] = slot_take();
        if(done)
            R32_HSPI_RX_ADDR1 = (UINT32)slot_addr(reg_slot[1]);
        else
            R32_HSPI_RX_ADDR0 = (UINT32)slot_addr(reg_slot[0]);
        rx_tog = (R8_HSPI_RX_SC & RB_HSPI_RX_TOG) ? 1 : 0;
        last_tog_after = rx_tog;
        R8_HSPI_INT_FLAG = RB_HSPI_IF_R_DONE;

        if((stream_stats.packets & 0xFF) == 0)
            R32_PB_OUT ^= LED_B;
    }
}

/* ------------------------------------------------------------------ USB pump */
static void usb_pump(void)
{
    if(!usb_stream_configured() || usb_stream_in_busy())
        return;
    /* The previous packet has gone out: only now may its slot be filled again (freeing it when the last packet was
     * merely queued let the HSPI DMA overwrite it while the USB DMA was still reading it). */
    if(release_slot != TRASH)
    {
        free_stack[free_count++] = release_slot;
        release_slot = TRASH;
    }
    if(!fifo_count)
        return;
    {
        UINT8  s = fifo[fifo_head];
        UINT16 len = fifo_len[fifo_head];
        UINT16 n = len - usb_off;
        if(n > 512)
            n = 512;
        usb_stream_in_send(ring[s] + usb_off, n);
        stream_stats.bytes_out += n;
        stream_stats.usb_packets++;
        usb_off += n;
        if(usb_off >= len)
        {
            usb_off = 0;
            fifo_head = (fifo_head + 1) % STREAM_RING_SLOTS;
            fifo_count--;
            release_slot = s;
        }
    }
}

/* ------------------------------------------------------------------ main */
int main(void)
{
    SystemInit(FREQ_SYS);
    Delay_Init(FREQ_SYS);

    GPIOB_SetBits(LED_R | LED_G | LED_B);                       /* off (common anode) */
    GPIOB_ModeCfg(LED_R | LED_G | LED_B, GPIO_Slowascent_PP_8mA);
    GPIOB_ResetBits(LED_R);

    link_init();
    link_send("\x01" "MSTR\n");
    link_send("rp569 CH569W stream firmware " FW_VERSION " git " FW_GIT "\n");

    hspi_rx_init();
    usb_stream_init();

    while(1)
    {
        static UINT32 loops, last_packets = 0xFFFFFFFF;
        /* Debug only: the lines go out on the 115200-baud link with blocking writes (~17 ms each) while the ring holds
         * ~4 ms of stream at 16 MB/s, so they cost packets. The statistics are on USB (STREAM_REQ_STATS). */
        if(STREAM_DEBUG_STAT && (++loops & 0x7FFFF) == 0 && stream_stats.packets != last_packets)
        {
            char line[200];
            last_packets = stream_stats.packets;
            snprintf(line, sizeof(line), "STAT pk %lu in %lu crc %lu seq %lu ovf %lu fov %lu out %lu len %u..%u r0 %u r1 %u done %u tog %u sc %02x udf %lx/%lx\n",
                     (unsigned long)stream_stats.packets, (unsigned long)stream_stats.bytes_in,
                     (unsigned long)stream_stats.crc_errors, (unsigned long)stream_stats.seq_errors,
                     (unsigned long)stream_stats.overflows, (unsigned long)stream_stats.fifo_ov,
                     (unsigned long)stream_stats.bytes_out, len_min, len_max, len_r0, len_r1, last_done,
                     last_tog_after, R8_HSPI_RX_SC, (unsigned long)R32_HSPI_UDF0, (unsigned long)R32_HSPI_UDF1);
            link_send(line);
            {   /* first 16 bytes of the last packet (slot or trash) */
                UINT8 *d = slot_addr(last_slot);
                snprintf(line, sizeof(line), "DATA %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n",
                         d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
                link_send(line);
            }
        }
        hspi_rx_poll();
        usb_stream_poll();
        usb_pump();
        link_poll();
        if(usb_stream_configured())
            GPIOB_ResetBits(LED_G);
        else
            GPIOB_SetBits(LED_G);
    }
}
