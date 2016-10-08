/* ================================================================================
 *
 * DMASDSPI API.
 *
 * Copyright (c) 2016, KIICHIRO KOTAJIMA
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ================================================================================
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

//Return Card Status.
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
