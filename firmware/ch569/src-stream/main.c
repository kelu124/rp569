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
static UINT8  rx_tog;                 /* register the next packet completes on (RX toggle, see hspi_rx_poll) */
static UINT16 usb_off;                /* bytes of the oldest slot already received by the host */
static UINT8  arm_idx;                /* slots ahead of fifo_head for the next chunk to arm */
static UINT16 arm_off;                /* byte offset in that slot */
static UINT16 len_min = 0xFFFF, len_max, len_r0, len_r1;   /* receive length diagnostics (STAT line) */
static UINT8  last_done, last_tog_after;
static UINT8  last_slot = TRASH;

/* ------------------------------------------------------------------ link UART1 (RP2350) */
static void link_init(void)
{
    UINT32 x = ((FREQ_SYS / 8) * 10 / LINK_BAUD + 5) / 10;   /* = FREQ_SYS * 2 / 16 / baud, rounded; no 32-bit overflow at 120 MHz */
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

/* Vendor request STREAM_REQ_RESET: statistics only. The ring and the chunk cursors keep their state (a USB packet
 * may be armed on a slot: clearing them under it desynchronised the cursors and freed slots still being sent). */
void stream_reset(void)
{
    memset((void *)&stream_stats, 0, sizeof(stream_stats));
}

/* Boot: free list, empty FIFO, both RX registers on real slots. */
static void ring_init(void)
{
    UINT8 i;
    fifo_head = fifo_count = 0;
    usb_off = 0;
    arm_idx = 0;
    arm_off = 0;
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
    stream_reset();
    ring_init();                              /* free list + both RX registers on real slots */
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
        /* The packet completed on the register given by the RX toggle read after the previous packet. That read is
         * only right while the bus is idle: once the next packet has started the toggle has moved on, and with
         * back-to-back packets the firmware stayed one register out of phase and streamed slots still being written
         * (board #1: old words interleaved with new ones). The RP2350 leaves 12 us between packets (stream.c GAP_US),
         * more than one main-loop pass (loop_max_us). No per-register packet number exists to check it against:
         * R32_HSPI_UDF0 reads 0 and UDF1 holds the raw header of the latest packet. */
        UINT8  done = rx_tog;
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
/* Chunk cursor for the USB side (512-byte chunks of the filled slots, oldest first). `take` gives the next chunk to
 * arm (up to two may be armed ahead with double buffering), `done` records one chunk received by the host: a slot is
 * freed only once all its chunks are done, so the HSPI DMA never overwrites data the USB DMA may still read. */

UINT8 *stream_chunk_take(UINT16 *len)
{
    UINT8 i, s;
    UINT16 n;
    if(arm_idx >= fifo_count)
        return 0;
    i = (fifo_head + arm_idx) % STREAM_RING_SLOTS;
    s = fifo[i];
    n = fifo_len[i] - arm_off;
    if(n > 512)
        n = 512;
    *len = n;
    {
        UINT8 *p = ring[s] + arm_off;
        arm_off += n;
        if(arm_off >= fifo_len[i])
        {
            arm_off = 0;
            arm_idx++;
        }
        return p;
    }
}

void stream_chunk_done(UINT16 n)
{
    stream_stats.bytes_out += n;
    stream_stats.usb_packets++;
    usb_off += n;
    if(usb_off >= fifo_len[fifo_head])
    {
        usb_off = 0;
        free_stack[free_count++] = fifo[fifo_head];
        fifo_head = (fifo_head + 1) % STREAM_RING_SLOTS;
        fifo_count--;
        if(arm_idx)
            arm_idx--;
    }
}

void stream_chunk_rewind(void)        /* nothing armed any more (reset / clear halt): re-arm from the done cursor */
{
    arm_idx = 0;
    arm_off = usb_off;
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

    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    SysTick->CTLR = 1;                        /* free-running, FREQ_SYS / 8: main-loop timing (loop_max_us) */
    while(1)
    {
        {
            static UINT32 t_prev;
            UINT32 t = (UINT32)SysTick->CNT;
            UINT32 us = (t_prev - t) / (FREQ_SYS / 8000000);   /* SysTick counts down here */
            t_prev = t;
            if(us > stream_stats.loop_max_us && stream_stats.packets > 4)
                stream_stats.loop_max_us = us;
        }
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
        usb_stream_fill();
        link_poll();
        if(usb_stream_configured())
            GPIOB_ResetBits(LED_G);
        else
            GPIOB_SetBits(LED_G);
    }
}
