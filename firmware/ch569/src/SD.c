/********************************** (C) COPYRIGHT *******************************
* File Name          : SD.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/07/31
* Description 
*******************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/


#include <string.h>
#include "SD.h"
#include "CH56x_common.h"
#include "bridge_config.h"

/* Bounded replacement for the stock `while(1){CheckCMDComp}` waits: returns
 * CMD_FAILED instead of spinning forever if the card never answers a command
 * (e.g. a warm/unreachable card). A hung command here used to wedge the whole
 * device — and with USB IRQs live, starve EP0. */
static UINT8 SD_WaitCmd(PSD_PARAMETER p)
{
    UINT32 g = MSC_EMMC_GUARD;
    UINT8  s;
    while((s = CheckCMDComp(p)) == CMD_NULL)
        if(--g == 0) return CMD_FAILED;
    return s;
}

/* Bounded wait for an EMMC data-transfer-done flag. Returns OP_FAILED on guard
 * expiry or a latched EMMC error. */
static UINT8 SD_WaitTranDoneN(PSD_PARAMETER p, UINT16 n)
{
    UINT32 g = MSC_EMMC_GUARD * (n ? n : 1);   /* the guard is per block */
    while(!(R16_EMMC_INT_FG & RB_EMMC_IF_TRANDONE))
        if(p->EMMCOpErr || --g == 0) return OP_FAILED;
    return OP_SUCCESS;
}
#define SD_WaitTranDone(p) SD_WaitTranDoneN((p), 1)

/* Wait (500 ms max) until the card no longer holds DAT0 low: it stays busy while it programs a write. */
static UINT8 SD_WaitReady(void)
{
    UINT32 us;
    for(us = 0; us < 500000; us += 10)
    {
        if(R32_EMMC_STATUS & (1<<17))
            return OP_SUCCESS;
        mDelayuS(10);
    }
    return OP_FAILED;
}

/* CMD12 STOP_TRANSMISSION after CMD18/CMD25 (R1b), flags cleared. */
static UINT8 SD_Stop(PSD_PARAMETER p)
{
    UINT8 sta;
    EMMCSendCmd(0, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_R1b | EMMC_CMD12);
    sta = SD_WaitCmd(p);
    R16_EMMC_INT_FG = 0xffff;
    return sta;
}

/***************************************************************
 * @fn        SDCardReadMulSec
 *
 * @brief     read n consecutive sectors (CMD18 + CMD12; CMD17 for n == 1,
 *            a single-block CMD18 never raises TRANDONE on this controller).
 *            The multi-block pattern is the HydraUSB3 BSP's EMMCCardReadMulSec
 *            (wch-ch56x-bsp drv/CH56x_emmc.c) with bounded waits.
 *
 * @return    OP_SUCCESS / OP_FAILED / OP_INVALID_ADD
 */
UINT8 SDCardReadMulSec( PSD_PARAMETER pEMMCPara, PUINT8 pRdatbuf, UINT32 Lbaaddr, UINT16 n )
{
    if(n == 1)
        return SDCardReadOneSec(pEMMCPara, pRdatbuf, Lbaaddr);
    if(n == 0 || Lbaaddr + n > pEMMCPara->EMMCSecNum)
        return OP_INVALID_ADD;
    if(SD_WaitReady() != OP_SUCCESS)
        return OP_FAILED;

    pEMMCPara->EMMCOpErr = 0;
    R16_EMMC_INT_FG = 0xffff;
    R32_EMMC_DMA_BEG1 = (UINT32)pRdatbuf;
    R32_EMMC_TRAN_MODE = 0;
    R32_EMMC_BLOCK_CFG = (pEMMCPara->EMMCSecSize)<<16 | n;
    EMMCSendCmd(Lbaaddr, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_48 | EMMC_CMD18);

    if(SD_WaitTranDoneN(pEMMCPara, n) != OP_SUCCESS)
    {
        PRINT("SD: read %lu+%u failed, err %04x flags %04x\n", (unsigned long)Lbaaddr, n,
              pEMMCPara->EMMCOpErr, R16_EMMC_INT_FG);
        SD_Stop(pEMMCPara);
        return OP_FAILED;
    }
    R16_EMMC_INT_FG = RB_EMMC_IF_CMDDONE | RB_EMMC_IF_TRANDONE;
    return SD_Stop(pEMMCPara) == CMD_SUCCESS ? OP_SUCCESS : OP_FAILED;
}

/***************************************************************
 * @fn        SD_SwitchHighSpeed
 *
 * @brief     CMD6: check, then select the high-speed access mode (function
 *            group 1 = 1, up to 50 MHz). SD Physical Layer spec 4.3.10; same
 *            sequence as phantomdrive src/sd.c SD_EnableHighSpeed (MIT).
 *            Card must be selected (CMD7) and on the 4-bit bus.
 *
 * @param     status - 64-byte DMA buffer (16-byte aligned, RAMX)
 * @return    1 = high speed selected, 0 = card stays at default speed
 */
static UINT8 SD_Switch(PSD_PARAMETER p, UINT32 arg, PUINT8 status)
{
    UINT8 sta;
    p->EMMCOpErr = 0;
    R16_EMMC_INT_FG = 0xffff;
    R32_EMMC_DMA_BEG1 = (UINT32)status;
    R32_EMMC_TRAN_MODE = 0;
    R32_EMMC_BLOCK_CFG = 64<<16 | 1;
    EMMCSendCmd(arg, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_48 | EMMC_CMD6);
    sta = SD_WaitCmd(p);
    if(sta == CMD_SUCCESS && SD_WaitTranDone(p) != OP_SUCCESS)
        sta = CMD_FAILED;
    R16_EMMC_INT_FG = 0xffff;
    p->EMMCOpErr = 0;
    return sta;
}

/* CMD6 status lands in its own zeroed buffer: reusing the scratch buffer the SCR read had just used returned stale
 * or shifted data on board #1, and the card was never switched (2026-11-19). 128 bytes for the 64-byte status. */
__attribute__((aligned(16))) static UINT8 sd_switch_status[128] __attribute__((section(".DMADATA")));

UINT8 SD_SwitchHighSpeed(PSD_PARAMETER pEMMCPara, PUINT8 unused)
{
    PUINT8 status = sd_switch_status;
    (void)unused;
    memset(sd_switch_status, 0, sizeof(sd_switch_status));
    if(SD_Switch(pEMMCPara, 0x00FFFFF1, status) != CMD_SUCCESS)   /* mode 0: check */
        return 0;
    if(!(status[13] & 0x02))                                       /* group 1 supports function 1? */
        return 0;
    memset(sd_switch_status, 0, sizeof(sd_switch_status));
    if(SD_Switch(pEMMCPara, 0x80FFFFF1, status) != CMD_SUCCESS)   /* mode 1: switch */
        return 0;
    mDelayuS(10);                                                  /* switch takes effect within 8 clocks */
    return (status[16] & 0x0F) == 1;
}

/***************************************************************
 * @fn        SDReadOCR
 *
 * @brief     when SD waiting status,do the OCR analysis
 *
 * @param     pEMMCPara
 *
 * @return    CMD_SUCCESS
 *            OP_FAILED
 */
UINT8 SDReadOCR( PSD_PARAMETER pEMMCPara )
{
	UINT8  i;
	UINT32 cmd_arg_val;
	UINT16 cmd_set_val;
	UINT8  sta = 0;
	UINT32 cmd_rsp_val;                 //command returned value

	for(i=0; i<100; i++)
	{
		cmd_arg_val = 0;                //request switching voltage
		cmd_set_val = RB_EMMC_CKIDX |   //ACK's index
		              RB_EMMC_CKCRC |   //CRC
					  RESP_TYPE_48  |   //ACK type
					  55;               //command's index this moment
		EMMCSendCmd(cmd_arg_val,  cmd_set_val);
		mDelayuS(2);
		sta = SD_WaitCmd( pEMMCPara );
        if(sta == CMD_FAILED)
        {
            mDelaymS(20);
            continue;
        }
        cmd_arg_val = 0x40FF8000;       //Request voltage switching, 0x41FF8000-->Request voltage switching
        cmd_set_val = 0 |               //The command index of the verification response
                      0 |               //Check the CRC of the response
                      RESP_TYPE_48 |    //expected response type
                      41;               //The index number of the currently sent command
        EMMCSendCmd(cmd_arg_val,  cmd_set_val);
        mDelayuS(2);
        sta = SD_WaitCmd( pEMMCPara );
        if(sta == CMD_SUCCESS)
        {
            cmd_rsp_val = R32_EMMC_RESPONSE3 ;
            if(cmd_rsp_val & (1<<31))                                       // Card initialization complete
            {
                if(cmd_rsp_val & (1<<30))
                    pEMMCPara->EMMCType = EMMCIO_HIGH_CAPACITY_SD_CARD;     //high capacity card
                else
                    pEMMCPara->EMMCType = EMMCIO_CAPACITY_SD_CARD_V2_0;     //Standard Capacity Card

                if(cmd_rsp_val & (1<<24))                                   //Support cut voltage
                {
                    printf(" support low vol..\n ");
                }
                break;
            }
        }
        mDelaymS(20);
	}
	if(i == 100)		return OP_FAILED;

	return sta;
}

/***************************************************************
 * @fn        SDSetRCA
 *
 * @brief     Assign relative address to deviceARC   16bit
 *
 * @param     pEMMCPara
 *
 * @return    OP_SUCCESS
 *            OP_FAILED
 */
UINT8 SDSetRCA( PSD_PARAMETER pEMMCPara )
{
	UINT32 cmd_arg_val;
	UINT16 cmd_set_val;
	UINT8  sta;

	cmd_arg_val = 0;
	cmd_set_val = RB_EMMC_CKIDX |
				  RB_EMMC_CKCRC |
				  RESP_TYPE_48  |
				  EMMC_CMD3;
	EMMCSendCmd(cmd_arg_val,cmd_set_val);
	sta = SD_WaitCmd( pEMMCPara );
	if(sta == CMD_SUCCESS)
	{
		pEMMCPara->EMMC_RCA = R32_EMMC_RESPONSE3 >> 16;
	}
	return sta;
}

/***************************************************************
 * @fn        SDReadCSD
 *
 * @brief     Acquire 128bit CSD parameter and get it analyzed
 *
 * @param     pEMMCPara
 *
 * @return    OP_SUCCESS
 *            OP_FAILED
 */
UINT8 SDReadCSD( PSD_PARAMETER pEMMCPara )
{
	UINT32 cmd_arg_val;
	UINT16 cmd_set_val;
	UINT8  sta;
	UINT32 disk_block_num = 0;

	cmd_arg_val = pEMMCPara->EMMC_RCA<<16;
	cmd_set_val = 0 |
				  0 |
				  RESP_TYPE_136 |
				  EMMC_CMD9;
	EMMCSendCmd(cmd_arg_val, cmd_set_val);
	sta = SD_WaitCmd( pEMMCPara );

	if(sta == CMD_SUCCESS)
	{
		pEMMCPara->EMMC_CSD[0] = R32_EMMC_RESPONSE0;
		pEMMCPara->EMMC_CSD[1] = R32_EMMC_RESPONSE1;
		pEMMCPara->EMMC_CSD[2] = R32_EMMC_RESPONSE2;
		pEMMCPara->EMMC_CSD[3] = R32_EMMC_RESPONSE3;

		if(pEMMCPara->EMMC_CSD[3]>>30)   //High Capacity and Extended Capacity Cards
        {
		    pEMMCPara->EMMCType = EMMCIO_HIGH_CAPACITY_SD_CARD;
            /* C_Size memory capacity = (C_SIZE+1)*512K byte */
            disk_block_num = ( (((pEMMCPara->EMMC_CSD[2]&0x0ff)<<16) | ((pEMMCPara->EMMC_CSD[1]&0xffff0000)>>16)) + 1 ) << 10;
        }
        else //dard card
        {
            pEMMCPara->EMMCType = EMMCIO_CAPACITY_SD_CARD_V2_0;
            /* memory capacity = BLOCKNR*BLOCK_LEN = (C_SIZE+1)<<(C_SIZE_MULT+2)<<(READ_BL_LEN) */
            disk_block_num = ( (((pEMMCPara->EMMC_CSD[2]&0x3ff)<<2) | (pEMMCPara->EMMC_CSD[1]>>30)) + 1 );
            disk_block_num = ( (disk_block_num) << (((pEMMCPara->EMMC_CSD[1]>>15)&0x07) + 2));
        }

	}
	pEMMCPara->EMMCSecNum = disk_block_num;                             //total number of sectors
    pEMMCPara->EMMCSecSize = 1<<((pEMMCPara->EMMC_CSD[2]>>16)&0x000f);  //sector size
	return sta;
}

/***************************************************************
 * @fn        SDSetBusWidth
 *
 * @brief     Set bus width.
 *
 * @param     pEMMCPara
 *            bus_mode
 *
 * @return    OP_SUCCESS
 *            OP_FAILED
 */
UINT8 SDSetBusWidth(PSD_PARAMETER pEMMCPara, UINT8 bus_mode)
{
	UINT32 cmd_arg_val;
	UINT16 cmd_set_val;
	UINT8  sta;

    cmd_arg_val = (pEMMCPara->EMMC_RCA)<<16;
    cmd_set_val = RB_EMMC_CKIDX |            //The command index of the verification response
                  RB_EMMC_CKCRC |            //Check the CRC of the response
                  RESP_TYPE_48  |            //expected response type
                  55;                        //The index number of the currently sent command
    EMMCSendCmd(cmd_arg_val,cmd_set_val);
    sta = SD_WaitCmd( pEMMCPara );
    if(sta == CMD_SUCCESS)
    {
        if(bus_mode == 0)
            cmd_arg_val = 0x0;               //single wire interface
        else
            cmd_arg_val = 0x2;               //4-wire interface

        cmd_set_val = RB_EMMC_CKIDX |        //The command index of the verification response
                      RB_EMMC_CKCRC |        //Check the CRC of the response
                      RESP_TYPE_48  |        //expected response type
                      EMMC_CMD6;             //The index number of the currently sent command
        EMMCSendCmd(cmd_arg_val, cmd_set_val);
        while(1)
        {
            sta = CheckCMDComp( pEMMCPara );
            if( sta != CMD_NULL ) break;
        }
    }
	return sta;
}

/***************************************************************
 * @fn        SD_ReadSCR
 *
 * @brief     Read SCR
 *
 * @param     pEMMCPara
 *            pRdatbuf
 *
 * @return    OP_SUCCESS
 *            OP_FAILED
 */
UINT8 SD_ReadSCR(PSD_PARAMETER pEMMCPara, PUINT8 pRdatbuf)
{
    UINT32 cmd_arg_val;
    UINT16 cmd_set_val,t;
    UINT8  sta;

    cmd_arg_val = (pEMMCPara->EMMC_RCA)<<16;
    cmd_set_val = RB_EMMC_CKIDX |               //The command index of the verification response
                  RB_EMMC_CKCRC |               //CRC of the verification response
                  RESP_TYPE_48  |               //expected response type
                  55;                           //The index number of the currently sent command
    EMMCSendCmd(cmd_arg_val,cmd_set_val);
    sta = SD_WaitCmd( pEMMCPara );

    if(sta == CMD_SUCCESS)
    {
        R32_EMMC_DMA_BEG1 = (UINT32)pRdatbuf;
        R32_EMMC_TRAN_MODE = 0;
        R32_EMMC_BLOCK_CFG = 8 << 16 | 1;        //The number of data bytes received per block | Set the number of blocks to receive and start the transfer

        /* Send CMD51 */
        cmd_arg_val = 0;
        cmd_set_val = RB_EMMC_CKIDX |            //The command index of the verification response
                      RB_EMMC_CKCRC |            // CRC of the verification response
                      RESP_TYPE_48  |            // expected response type
                      51;                        // The index number of the currently sent command
        EMMCSendCmd(  cmd_arg_val, cmd_set_val );
        while(1)
        {
            sta = CheckCMDComp( pEMMCPara );
            if( sta != CMD_NULL )
                break;
        }
        if( SD_WaitTranDone( pEMMCPara ) != OP_SUCCESS )
            return CMD_FAILED;
        t=R16_EMMC_INT_FG;
        R16_EMMC_INT_FG = t;
    }

    return sta;
}

/***************************************************************
 * @fn        SDCardReadOneSec
 *
 * @brief     Read single section
 *
 * @param     pEMMCPara
 *            pRdatbuf
 *            Lbaaddr
 *
 * @return    OP_SUCCESS
 *            OP_FAILED
 */
UINT8 SDCardReadOneSec( PSD_PARAMETER pEMMCPara, PUINT8 pRdatbuf, UINT32 Lbaaddr )
{
	UINT32 cmd_arg_val;
	UINT16 cmd_set_val;

	UINT32 g;

	if(Lbaaddr > (pEMMCPara->EMMCSecNum))
	    return  OP_INVALID_ADD;

    g = MSC_EMMC_GUARD;
    while(!(R32_EMMC_STATUS & (1<<17)))             /* controller ready (bounded) */
        if(--g == 0) return OP_FAILED;

    pEMMCPara->EMMCOpErr = 0;
    R32_EMMC_DMA_BEG1 = (UINT32)pRdatbuf;
    R32_EMMC_TRAN_MODE = 0;
    R32_EMMC_BLOCK_CFG = (pEMMCPara->EMMCSecSize)<<16 | 1;

	cmd_arg_val = Lbaaddr;
	cmd_set_val = RB_EMMC_CKIDX |
				  RB_EMMC_CKCRC |
				  RESP_TYPE_48  |
				  EMMC_CMD17;  //read single block (CMD17, not CMD18: a single-block
				               //CMD18 never raises TRANDONE on this controller)

	EMMCSendCmd(cmd_arg_val, cmd_set_val);

	if( SD_WaitTranDone( pEMMCPara ) != OP_SUCCESS )
	{
	    R16_EMMC_INT_FG = 0xffff;
	    return OP_FAILED;
	}
	R16_EMMC_INT_FG = RB_EMMC_IF_CMDDONE|RB_EMMC_IF_TRANDONE;
	return 	OP_SUCCESS;
}

/***************************************************************
 * @fn        SDCardWriteONESec
 *
 * @brief     write ONE sections
 *
 * @param     pSDPara
 *            pReqnum
 *            pWdatbuf
 *            Lbaaddr
 *
 * @return    OP_SUCCESS
 *            OP_FAILED
 */
UINT8 SDCardWriteONESec( PSD_PARAMETER pEMMCPara,  PUINT8 pWdatbuf, UINT32 Lbaaddr )
{
    return SDCardWriteMulSec(pEMMCPara, pWdatbuf, Lbaaddr, 1);
}

/* n consecutive sectors: CMD25, n blocks of DMA, CMD12 (HydraUSB3 BSP EMMCCardWriteMulSec, bounded waits). */
UINT8 SDCardWriteMulSec( PSD_PARAMETER pEMMCPara,  PUINT8 pWdatbuf, UINT32 Lbaaddr, UINT16 n )
{
	UINT32 cmd_arg_val;
	UINT16 cmd_set_val;
	UINT8  sta;

	if(n == 0 || Lbaaddr + n > (pEMMCPara->EMMCSecNum))
	    return  OP_INVALID_ADD;

    if(SD_WaitReady() != OP_SUCCESS)               /* previous write still programming */
        return OP_FAILED;

    pEMMCPara->EMMCOpErr = 0;

    //cmd25
	cmd_arg_val = Lbaaddr;
	cmd_set_val = RB_EMMC_CKIDX |
				  RB_EMMC_CKCRC |
				  RESP_TYPE_48  |
				  EMMC_CMD25;
	EMMCSendCmd(cmd_arg_val, cmd_set_val);
	sta = SD_WaitCmd( pEMMCPara );

	if( sta == CMD_FAILED )
	{	//The operation failed
	    PRINT("SD: CMD25 %lu failed, err %04x\n", (unsigned long)Lbaaddr, pEMMCPara->EMMCOpErr);
	    return OP_FAILED;
	}

	//DAT. Single block: the vendor example's mode (bit 6 set). Multi-block: the HydraUSB3 BSP's (DMA_DIR only, and
	//R32_EMMC_RESPONSE3 written at each block gap): with bit 6 the DMA stopped after the first block (2026-11-19).
	R32_EMMC_TRAN_MODE = (n == 1) ? (RB_EMMC_DMA_DIR|(1<<6)) : RB_EMMC_DMA_DIR;
	R32_EMMC_DMA_BEG1 = (UINT32)pWdatbuf;
	R32_EMMC_BLOCK_CFG = (pEMMCPara->EMMCSecSize)<<16 | n;

	{ UINT32 g = MSC_EMMC_GUARD * n;               /* per block: the card is busy between blocks */
	while(1)
	{
		if(R16_EMMC_INT_FG & RB_EMMC_IF_BKGAP)
		{
			R32_EMMC_RESPONSE3 = 0;
			R16_EMMC_INT_FG = RB_EMMC_IF_BKGAP;
		}
		else if(R16_EMMC_INT_FG & RB_EMMC_IF_TRANDONE)
		{
			R16_EMMC_INT_FG = RB_EMMC_IF_TRANDONE | RB_EMMC_IF_CMDDONE;
			//cmd12
			cmd_arg_val = 0;
			cmd_set_val = RB_EMMC_CKIDX |
						  RB_EMMC_CKCRC |
						  RESP_TYPE_R1b |
						  EMMC_CMD12;
			EMMCSendCmd(cmd_arg_val, cmd_set_val);
			break;
		}
		if( pEMMCPara->EMMCOpErr || --g == 0 )
		{
		    PRINT("SD: write %lu+%u failed, err %04x flags %04x\n", (unsigned long)Lbaaddr, n,
		          pEMMCPara->EMMCOpErr, R16_EMMC_INT_FG);
		    SD_Stop(pEMMCPara);                      /* leave the card in transfer state, not receive-data */
		    return OP_FAILED;
		}
	} }
	sta = SD_WaitCmd( pEMMCPara );
	R16_EMMC_INT_FG = RB_EMMC_IF_CMDDONE;
	return 	sta;
}
