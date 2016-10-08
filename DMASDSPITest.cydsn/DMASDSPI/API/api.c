/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "`$INSTANCE_NAME`_api.h"
#include "`$INSTANCE_NAME`_fatfs.h"

#include "`$INSTANCE_NAME`_SPIMaster.h"
#include "`$INSTANCE_NAME`_CSControl.h"
#include "`$INSTANCE_NAME`_CardStatus.h"
#include "`$INSTANCE_NAME`_RX_dma.h"
#include "`$INSTANCE_NAME`_TX_dma.h"
#include "`$INSTANCE_NAME`_RX_DMA_IRQ.h"

/* DMA Configuration for NSDSPI_RX */
#define DMA_RX_BYTES_PER_BURST 1
#define DMA_RX_REQUEST_PER_BURST 1
#define DMA_RX_SRC_BASE (CYDEV_PERIPH_BASE)
#define DMA_RX_DST_BASE (CYDEV_SRAM_BASE)

/* DMA Configuration for NSDSPI_TX */
#define DMA_TX_BYTES_PER_BURST 1
#define DMA_TX_REQUEST_PER_BURST 1
#define DMA_TX_SRC_BASE (CYDEV_SRAM_BASE)
#define DMA_TX_DST_BASE (CYDEV_PERIPH_BASE)

#define TRANSFER_WAIT_RXDMAISR     1

static volatile uint8_t nTransferFlags = 0;

static uint8_t bEnableDMA = 0;

/* Variable declarations for DMA RX */
static uint8_t RXDMA_Channel = 0;
static uint8_t RXDMA_TD[1];
/* Variable declarations for DMA TX */
static uint8_t TXDMA_Channel = 0;
static uint8_t TXDMA_TD[1];

static uint8_t dummy_data[1] = { 0xff };

// DMA termination ISRs
void `$INSTANCE_NAME`_RX_DMA_ISR(){
	nTransferFlags &= ~TRANSFER_WAIT_RXDMAISR;
}


void `$INSTANCE_NAME`_Start(`$INSTANCE_NAME`_VOIDFXN FnSelSlowCk, `$INSTANCE_NAME`_VOIDFXN FnSelFastCk, `$INSTANCE_NAME`_PRNTFXN FnPrint)
{
	nTransferFlags = 0;
	`$INSTANCE_NAME`_SPIMaster_Start();
	`$INSTANCE_NAME`_SetDMAMode(0);
	
	SPI_FUNCTIONS SpiFuncs;
	SpiFuncs.SPRXSTREAM = `$INSTANCE_NAME`_ReceiveData;
	SpiFuncs.SPTXSTREAM = `$INSTANCE_NAME`_SendData;
	SpiFuncs.SPEXCHANGE = `$INSTANCE_NAME`_SwapByte;
	SpiFuncs.SPSELECTCARD = `$INSTANCE_NAME`_SelectCard;
	SpiFuncs.SPDESELECTCARD = `$INSTANCE_NAME`_DeselectCard;
	SpiFuncs.SPFASTCK = FnSelFastCk;
	SpiFuncs.SPSLOWCK = FnSelSlowCk;
	SpiFuncs.CARDSTATUS = `$INSTANCE_NAME`_CardStatus;
	SpiFuncs.PRINT = FnPrint;
	
	disk_attach_spifuncs(&SpiFuncs);
}


void `$INSTANCE_NAME`_Stop()
{
	`$INSTANCE_NAME`_DeselectCard();
	`$INSTANCE_NAME`_SetDMAMode(0);
	`$INSTANCE_NAME`_SPIMaster_Stop();
}

void `$INSTANCE_NAME`_SelectCard() {
	`$INSTANCE_NAME`_CSControl_Write(0);
}

void `$INSTANCE_NAME`_DeselectCard() {
	`$INSTANCE_NAME`_CSControl_Write(1);
}

uint8_t `$INSTANCE_NAME`_CardStatus() {
	return (`$INSTANCE_NAME`_CardStatus_Read() << 1) & 0x06;
}

//Send a block of data and discard received bytes.
void `$INSTANCE_NAME`_SendData(const uint8_t* pBuffer, uint32_t nLength)
{
	if((nLength > 4095)||(nLength < 1)){
		return; //this never happens - filesys transfers max. 512 bytes at a time
	}

	if(bEnableDMA) {
		// 送信DMA TD設定
		//  - single TD
		//  - pBuffer(アドレス自動加算) => SPI_TXDATA
		CyDmaTdSetConfiguration(TXDMA_TD[0], nLength, CY_DMA_DISABLE_TD, `$INSTANCE_NAME`_TX__TD_TERMOUT_EN | TD_INC_SRC_ADR);
		CyDmaTdSetAddress(TXDMA_TD[0], LO16((uint32)pBuffer), LO16((uint32)`$INSTANCE_NAME`_SPIMaster_TXDATA_PTR));
		CyDmaChSetInitialTd(TXDMA_Channel, TXDMA_TD[0]);
		
		// 受信DMA TD設定(ダミーリード)
		//  - single TD
		//  - SPI_RXDATA => dummy_data(アドレス加算無し)
		CyDmaTdSetConfiguration(RXDMA_TD[0], nLength, CY_DMA_DISABLE_TD, `$INSTANCE_NAME`_RX__TD_TERMOUT_EN);
		CyDmaTdSetAddress(RXDMA_TD[0], LO16((uint32)`$INSTANCE_NAME`_SPIMaster_RXDATA_PTR), LO16((uint32)(&dummy_data[0])));
		CyDmaChSetInitialTd(RXDMA_Channel, RXDMA_TD[0]);

		//`$INSTANCE_NAME`_RX_DMA_IRQ_ClearPending();
		CyDmaClearPendingDrq(TXDMA_Channel);
		CyDmaClearPendingDrq(RXDMA_Channel);
		//`$INSTANCE_NAME`_SPIMaster_ClearFIFO();
		
		nTransferFlags |= TRANSFER_WAIT_RXDMAISR;   //Set flag
		
		CyDmaChEnable(RXDMA_Channel, 1);
		CyDmaChEnable(TXDMA_Channel, 1);
		
		CyDmaChSetRequest(TXDMA_Channel, CPU_REQ);  //trigger transfer

		// wait for transfer
		while(nTransferFlags & TRANSFER_WAIT_RXDMAISR);

		// stop DMA.
		CyDmaChSetRequest(TXDMA_Channel, CPU_TERM_CHAIN);
		CyDmaChSetRequest(RXDMA_Channel, CPU_TERM_CHAIN);
		return;
	}else{
		//Slow (non DMA) implementation...
		unsigned int i;
		for(i = 0; i < nLength; i++){
			`$INSTANCE_NAME`_SwapByte(pBuffer[i]);
		}
	}
}

//Send a block of NULL data (default 0xFF) and place received bytes into buffer.
void `$INSTANCE_NAME`_ReceiveData(uint8_t* pBuffer, uint32_t nLength)
{
	if((nLength > 4095)||(nLength < 1)){
		return; //this never happens - filesys transfers max. 512 bytes at a time
	}

	if(bEnableDMA) {
		// 送信DMA TD設定(ダミーライト)
		//  - single TD
		//  - dummy_data(アドレス加算無し) => SPI_TXDATA
		CyDmaTdSetConfiguration(TXDMA_TD[0], nLength, CY_DMA_DISABLE_TD, `$INSTANCE_NAME`_TX__TD_TERMOUT_EN);
		CyDmaTdSetAddress(TXDMA_TD[0], LO16((uint32)(&dummy_data[0])), LO16((uint32)`$INSTANCE_NAME`_SPIMaster_TXDATA_PTR));
		CyDmaChSetInitialTd(TXDMA_Channel, TXDMA_TD[0]);
		
		// 受信DMA TD設定
		//  - single TD
		//  - SPI_RXDATA => pBuffer(アドレス自動加算)
		CyDmaTdSetConfiguration(RXDMA_TD[0], nLength, CY_DMA_DISABLE_TD, `$INSTANCE_NAME`_RX__TD_TERMOUT_EN | TD_INC_DST_ADR);
		CyDmaTdSetAddress(RXDMA_TD[0], LO16((uint32)`$INSTANCE_NAME`_SPIMaster_RXDATA_PTR), LO16((uint32)pBuffer));
		CyDmaChSetInitialTd(RXDMA_Channel, RXDMA_TD[0]);

		dummy_data[0] = 0xff;
		nTransferFlags |= TRANSFER_WAIT_RXDMAISR;   //Set flag
		
		//`$INSTANCE_NAME`_RX_DMA_IRQ_ClearPending();
		CyDmaClearPendingDrq(TXDMA_Channel);
		CyDmaClearPendingDrq(RXDMA_Channel);
		//`$INSTANCE_NAME`_SPIMaster_ClearFIFO();
		
		CyDmaChEnable(RXDMA_Channel, 1);
		CyDmaChEnable(TXDMA_Channel, 1);

		CyDmaChSetRequest(TXDMA_Channel, CPU_REQ);  //trigger transfer

		//wait until done.
		while(nTransferFlags & TRANSFER_WAIT_RXDMAISR);

		// stop DMA.
		CyDmaChSetRequest(TXDMA_Channel, CPU_TERM_CHAIN);
		CyDmaChSetRequest(RXDMA_Channel, CPU_TERM_CHAIN);
		return;
	} else {
		//Slow (non DMA) implementation...
		unsigned int i;
		for(i = 0; i < nLength; i++){
			pBuffer[i] = `$INSTANCE_NAME`_SwapByte(0xFF);
		}
	}
}

//Swap a single byte (send it, and receive RX byte / without DMA)
uint8_t `$INSTANCE_NAME`_SwapByte(uint8_t data)
{
	//write byte to TX
	`$INSTANCE_NAME`_SPIMaster_WriteTxData(data);
	//wait for RX interrupt
	while(!(`$INSTANCE_NAME`_SPIMaster_ReadStatus() & `$INSTANCE_NAME`_SPIMaster_STS_BYTE_COMPLETE));
	//read byte from RX
	return `$INSTANCE_NAME`_SPIMaster_ReadRxData();
}

void `$INSTANCE_NAME`_SetDMAMode(unsigned char enable)
{
	if ((bEnableDMA && enable) || (!bEnableDMA && !enable))
		return;

	//make absolutely sure no transfers are in progress
	while(nTransferFlags);

	if (enable) {
		`$INSTANCE_NAME`_RX_DMA_IRQ_StartEx(`$INSTANCE_NAME`_RX_DMA_ISR);

		RXDMA_Channel = `$INSTANCE_NAME`_RX_DmaInitialize(
			DMA_RX_BYTES_PER_BURST, DMA_RX_REQUEST_PER_BURST,
			HI16(DMA_RX_SRC_BASE), HI16(DMA_RX_DST_BASE));

		TXDMA_Channel = `$INSTANCE_NAME`_TX_DmaInitialize(
			DMA_TX_BYTES_PER_BURST, DMA_TX_REQUEST_PER_BURST,
			HI16(DMA_TX_SRC_BASE), HI16(DMA_TX_DST_BASE));

		//allocate a TD
		TXDMA_TD[0] = CyDmaTdAllocate();
		RXDMA_TD[0] = CyDmaTdAllocate();

		CyDmaClearPendingDrq(RXDMA_Channel);
		CyDmaClearPendingDrq(TXDMA_Channel);

		bEnableDMA = 1;
	} else {
		//Free Transfer Descriptor
		CyDmaTdFree(TXDMA_TD[0]);
		CyDmaTdFree(RXDMA_TD[0]);

		`$INSTANCE_NAME`_RX_DMA_IRQ_Stop();
		`$INSTANCE_NAME`_RX_DmaRelease();
		`$INSTANCE_NAME`_TX_DmaRelease();

		bEnableDMA = 0;
	}
}


/* [] END OF FILE */
