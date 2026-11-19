// stream.h — RP2350 firmware variant 1 ("stream", D-031): PIO HSPI sender, see stream.c.
#ifndef RP569_STREAM_H
#define RP569_STREAM_H

#include <stdbool.h>

// Start the counter test stream at `mbs` MB/s of payload (1 MB = 1e6 bytes, 0 = as fast as the bus and the CH569
// allow) with an HSPI clock of about `clk_mhz` MHz and CRC16 variant `crc` (0 none, 1-4: see stream.c). False if
// already running.
bool stream_start(float mbs, float clk_mhz, int crc);
void stream_stop(void);                 // stops and gives the bus pins back (inputs, HSPI_D0 pull-up)
bool stream_running(void);
void stream_seq_reset(void);            // the CH569 restarted: its HSPI receive sequence is back to 0
void stream_print_status(void);         // "stream on|off, packets N, payload N bytes, T s, R MB/s, target ..., bus ..."

#endif
