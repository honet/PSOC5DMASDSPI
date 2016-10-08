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
#if !defined(`$INSTANCE_NAME`_API_H)
#define `$INSTANCE_NAME`_API_H

#include <stdint.h>

typedef void (*`$INSTANCE_NAME`_VOIDFXN)(void);
typedef void (*`$INSTANCE_NAME`_PRNTFXN)(char*);
    
//Start component (preferred method - this sets up the filesystem too!)
void `$INSTANCE_NAME`_Start(`$INSTANCE_NAME`_VOIDFXN FnSelSlowCk, `$INSTANCE_NAME`_VOIDFXN FnSelFastCk, `$INSTANCE_NAME`_PRNTFXN FnPrint);

//Disable component, release DMAs, stop ISRs, etc
void `$INSTANCE_NAME`_Stop();

//Control the CS line
void `$INSTANCE_NAME`_SelectCard();
void `$INSTANCE_NAME`_DeselectCard();

uint8_t `$INSTANCE_NAME`_CardStatus();

//Send a block of data and discard received bytes.
void `$INSTANCE_NAME`_SendData(const uint8_t* pBuffer, uint32_t nLength);

//Send a block of NULL data (default 0xFF) and place received bytes into buffer.
void `$INSTANCE_NAME`_ReceiveData(uint8_t* pBuffer, uint32_t nLength);

//Swap a single byte (send it, and receive RX byte)
uint8_t `$INSTANCE_NAME`_SwapByte(uint8_t data);

//Enable / Disable DMA transfers
void `$INSTANCE_NAME`_SetDMAMode(uint8_t bEnableDMA);



#endif

/* [] END OF FILE */
