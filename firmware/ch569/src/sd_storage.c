/********************************************************************************
 * sd_storage.c — SD-card init for the CH569 EMMC controller.
 * Ported from the WCH SD EVT example (SD/User/Main.c: EMMC_IO_init +
 * EMMC_function_init) using the SD.c command helpers. See sd_storage.h.
 *******************************************************************************/
#include "sd_storage.h"
#include "SD.h"
#include "SW_UDISK.h"
#include "bridge_config.h"

/* Scratch for the SCR read at the end of card init (DMA target). */
__attribute__((aligned(16))) static UINT8 sd_scratch[512] __attribute__((section(".DMADATA")));

/*******************************************************************************
 * IO/timing init, one of 8 adaptive modes (clock phase / negedge sampling
 * combinations) exactly as the vendor SD example probes them.
 ******************************************************************************/
static void sd_io_init(UINT8 mode)
{
    /* GPIO: pull-ups on CMD + DAT lines, CLK as output */
    R32_PB_PU  |= bSDCMD;
    R32_PB_PU  |= (0x1f << 17);          /* DAT lines PB17..PB21 */
    R32_PA_PU  |= (7 << 0);
    R32_PB_DIR |= bSDCK;

    R32_PA_DRV |= (7 << 0);              /* drive strength */
    R32_PB_DRV |= (0x1f << 17);
    R32_PB_DRV |= bSDCMD;
    R32_PB_DRV |= bSDCK;

    /* Controller reset */
    R8_EMMC_CONTROL = RB_EMMC_ALL_CLR | RB_EMMC_RST_LGC;

    if((mode == 0) || (mode == 1) || (mode == 6) || (mode == 7))
        R8_EMMC_CONTROL = RB_EMMC_DMAEN;
    else
        R8_EMMC_CONTROL = RB_EMMC_DMAEN | RB_EMMC_NEGSMP;

    R8_EMMC_CONTROL = (R8_EMMC_CONTROL & (~RB_EMMC_LW_MASK)) | bLW_OP_DAT0;  /* 1-bit for ident */

    if(mode < 4)
        R16_EMMC_CLK_DIV = RB_EMMC_CLKOE | RB_EMMC_PHASEINV | LOWEMMCCLK;
    else
        R16_EMMC_CLK_DIV = RB_EMMC_CLKOE | LOWEMMCCLK;

    /* Error interrupts */
    R16_EMMC_INT_FG = 0xffff;
    R16_EMMC_INT_EN = RB_EMMC_IE_FIFO_OV |
                      RB_EMMC_IE_TRANERR |
                      RB_EMMC_IE_DATTMO |
                      RB_EMMC_IE_REIDX_ER |
                      RB_EMMC_IE_RECRC_WR |
                      RB_EMMC_IE_RE_TMOUT;

    R8_EMMC_TIMEOUT = 14;
}

/*******************************************************************************
 * SD-card identification + switch to 4-bit / full clock.
 ******************************************************************************/
static UINT8 sd_card_init(UINT8 mode)
{
    UINT8  sta;
    UINT32 i;

    /* Warm-card preamble: the card stays powered across CH569 resets (the MUX
     * keeps its rail up), so on a reflash/reset it may still be mid-transfer or
     * out of idle from the previous session. CMD12 aborts any lingering
     * read/write; the double CMD0 then reliably returns it to idle so ACMD41's
     * power-up negotiation behaves like a cold card. (CMD12 on an idle card just
     * errors harmlessly.) */
    TF_EMMCParam.EMMCOpErr = 0;
    EMMCSendCmd(0, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_R1b | EMMC_CMD12);
    mDelaymS(2);
    R16_EMMC_INT_FG = 0xffff;

    EMMCResetIdle(&TF_EMMCParam);        /* CMD0 */
    mDelaymS(30);
    EMMCResetIdle(&TF_EMMCParam);
    mDelaymS(30);

    /* CMD8: voltage check, mandatory for SD v2+/SDHC/SDXC */
    for(i = 0; i < 3; i++)
    {
        EMMCSendCmd(0x01AA, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_48 | EMMC_CMD8);
        { UINT32 g = MSC_EMMC_GUARD;
          while((sta = CheckCMDComp(&TF_EMMCParam)) == CMD_NULL)
              if(--g == 0) { sta = CMD_FAILED; break; } }
        if(sta == CMD_SUCCESS)
            break;
        mDelaymS(30);
    }
    if(sta != CMD_SUCCESS)
        return OP_FAILED;

    sta = SDReadOCR(&TF_EMMCParam);      /* ACMD41 loop (3.3V, HCS) */
    if(sta != CMD_SUCCESS) return OP_FAILED;

    sta = EMMCReadCID(&TF_EMMCParam);    /* CMD2 */
    if(sta != CMD_SUCCESS) return OP_FAILED;

    sta = SDSetRCA(&TF_EMMCParam);       /* CMD3: card publishes its RCA */
    if(sta != CMD_SUCCESS) return OP_FAILED;

    sta = SDReadCSD(&TF_EMMCParam);      /* CMD9: capacity (SDHC/SDXC aware) */
    if(sta != CMD_SUCCESS) return OP_FAILED;

    mDelaymS(5);
    sta = SelectEMMCCard(&TF_EMMCParam); /* CMD7 */
    if(sta != CMD_SUCCESS) return OP_FAILED;

    sta = SDSetBusWidth(&TF_EMMCParam, 1); /* ACMD6: 4-bit */
    if(sta != CMD_SUCCESS) return OP_FAILED;
    R8_EMMC_CONTROL = (R8_EMMC_CONTROL & (~RB_EMMC_LW_MASK)) | bLW_OP_DAT4;

    sta = SD_ReadSCR(&TF_EMMCParam, sd_scratch);
    if(sta != CMD_SUCCESS) return OP_FAILED;

    /* Full data clock (48 MHz, divider 10) with the probed phase setting */
    if(mode < 4)
        R16_EMMC_CLK_DIV = RB_EMMC_CLKMode | RB_EMMC_PHASEINV | RB_EMMC_CLKOE | 10;
    else
        R16_EMMC_CLK_DIV = RB_EMMC_CLKMode | RB_EMMC_CLKOE | 10;

    return OP_SUCCESS;
}

UINT8 sd_storage_mount(void)
{
    UINT8 mode, s;

    for(mode = 0; mode < 8; mode++)
    {
        sd_io_init(mode);
        if(sd_card_init(mode) != OP_SUCCESS)
            continue;

        /* Identification (CMD2/3/9) passes at low clock; it does NOT prove the
         * 48 MHz data path samples correctly for this PHASEINV/NEGSMP mode.
         * Validate by actually reading LBA 0 before accepting the mode — else a
         * mode that "identifies" but can't read data locks in and every host
         * READ10 fails. */
        TF_EMMCParam.EMMCOpErr = 0;
        s = SDCardReadOneSec(&TF_EMMCParam, sd_scratch, 0);
        if(s == OP_SUCCESS)
        {
            TF_EMMCParam.EMMCOpErr = 0;
            R32_EMMC_TRAN_MODE = 0;
            /* Do NOT enable EMMC_IRQn. The error ISR (EMMC_IRQHandler) latches the
             * whole INT_FG into EMMCOpErr and clears the register — including the
             * TRANDONE bit that SD_WaitTranDone polls. On this marginal 48 MHz data
             * path the controller raises benign soft errors (TRANERR/RECRC/DATTMO)
             * that do NOT corrupt the transfer: TRANDONE still asserts and the data
             * is good. With the IRQ on, every one of those soft errors turned into a
             * hard OP_FAILED -> EP2 STALL -> host DID_ERROR on READ(10), even though
             * this very mode just validated an LBA-0 read above with the IRQ off.
             * The bounded MSC_EMMC_GUARD spin in SD_WaitTranDone already prevents a
             * dead card from hanging the device, so the ISR adds no value here.
             * PFIC_EnableIRQ(EMMC_IRQn);  */
            PRINT("SD: init OK (io mode %d), type %d, %ld sectors, sig %02x %02x\n",
                  mode, TF_EMMCParam.EMMCType, TF_EMMCParam.EMMCSecNum,
                  sd_scratch[510], sd_scratch[511]);
            PRINT("sec0: %02x %02x %02x %02x ... %02x %02x %02x %02x  end %02x %02x\n",
      sd_scratch[0],sd_scratch[1],sd_scratch[2],sd_scratch[3], sd_scratch[12],sd_scratch[13],sd_scratch[14],sd_scratch[15], sd_scratch[510],sd_scratch[511]);
            return OP_SUCCESS;
        }
        PRINT("SD: io mode %d identifies but data read fails\n", mode);
    }
    PRINT("SD: init failed in all IO modes\n");
    return OP_FAILED;
}

void sd_storage_unmount(void)
{
    PFIC_DisableIRQ(EMMC_IRQn);
    R32_EMMC_TRAN_MODE = 0;
    R16_EMMC_INT_FG = 0xffff;
}
