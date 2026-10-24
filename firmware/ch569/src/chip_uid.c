/********************************************************************************
 * chip_uid.c — the CH569's factory unique ID (8 bytes, read-only flash area).
 *
 * CH569DS1 §2.2.3: "Each chip has a unique ID ... 8 bytes in total, stored in
 * the read-only area of chip". Address 0x77FE4 and the ROM access sequence come
 * from HydraUSB3 (hydrausb3_fw/HydraUSB3_USB/User/Main.c, wch-ch56x-bsp
 * drv/CH56x_flash.c FLASH_ROMA_READ), Copyright (c) 2022 Hans Baier,
 * Benjamin VERNOUX, Apache-2.0.
 *******************************************************************************/
#include "CH56x_common.h"
#include "chip_uid.h"

/* SPI flash ROM access registers (missing from this SDK's CH56xSFR.h) */
#define ROM_CR        (*((PUINT8V)0x4000101A))
#define ROM_CTRL      (*((PUINT8V)0x40001018))
#define ROM_DATA8     (*((PUINT8V)0x40001018))
#define ROM_DATA32    (*((PUINT32V)0x40001014))

#define ROM_ADDR_OFFSET  0x8000
#define ROM_BEGIN_READ   0x0B
#define UID_ADDR         0x77FE4

static void rom_wait(void)
{
    while((INT8)ROM_CR < 0)
        ;
}

static UINT8 rom_read8(void)
{
    rom_wait();
    return ROM_DATA8;
}

static void rom_write8(UINT8 b)
{
    rom_wait();
    ROM_DATA8 = b;
}

void chip_uid_read(UINT8 uid[CHIP_UID_LEN])
{
    UINT32 addr = UID_ADDR + ROM_ADDR_OFFSET;
    UINT32 w[2];
    UINT8 i;

    ROM_CR = 0;
    ROM_CR = 0x07;
    ROM_CTRL = ROM_BEGIN_READ;
    rom_write8((addr >> 16) & 0xFF);
    rom_write8((addr >> 8) & 0xFF);
    rom_write8(addr & 0xFF);
    rom_read8();
    rom_read8();
    for(i = 0; i < 2; i++)
    {
        rom_read8();
        rom_read8();
        rom_read8();
        rom_read8();
        w[i] = ROM_DATA32;
    }
    rom_wait();
    ROM_CR = 0;
    for(i = 0; i < CHIP_UID_LEN; i++)
        uid[i] = (w[i / 4] >> (8 * (i % 4))) & 0xFF;
}

void chip_uid_hex(char out[2 * CHIP_UID_LEN + 1])
{
    static const char hex[] = "0123456789ABCDEF";
    UINT8 uid[CHIP_UID_LEN];
    UINT8 i;

    chip_uid_read(uid);
    for(i = 0; i < CHIP_UID_LEN; i++)
    {
        out[2 * i]     = hex[uid[i] >> 4];
        out[2 * i + 1] = hex[uid[i] & 0x0F];
    }
    out[2 * CHIP_UID_LEN] = 0;
}
