/* ================================================================================
 *
 * DMASDSPI test code.
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
#include <string.h>
#include <stdio.h>
#include "project.h"
#include "SDSPI_api.h"
#include "SDSPI_fatfs.h"

volatile unsigned long nmillis = 0; //used for time measurement

#define BUFFER_SIZE 32768
uint8_t buffer[BUFFER_SIZE];

FIL File; // result message txt-file

#define dbgprint(fmt, ...) \
do{ \
	char str[128]; \
	sprintf(str, fmt, ## __VA_ARGS__); \
	while(!USBUART_1_CDCIsReady()); \
	USBUART_1_PutString(str); \
}while(0)

#define dumpmsg(msg, ...) \
do{ \
	UINT BytesWritten; \
	size_t sz=strlen(msg); \
	dbgprint(msg, ## __VA_ARGS__); \
	FRESULT res = f_write(&File, msg, sz, &BytesWritten); \
	chkres("f_write", res); \
}while(0)


void MillisISR()
{
    nmillis++;          //used for time measurements
    disk_timerproc();   //MUST attach this to 1kHz interrupt for file system timeouts to work
}

// switch SCLK to fast mode.
void SDCARD_FAST()
{
	// 0: 24MHz, 1; 12MHz, 2:8MHz, 3: 6Mhz (@BusClk=48MHz)
	SD_CLK_SetDivider(1);
}

// switch SCLK to fast mode.
void SDCARD_SLOW()
{
	// notice: SD card must be initialised at max. 400 kHz
	// 199: 240kHz  (@BusClk=48MHz)
	SD_CLK_SetDivider(199);
}

void PrintFxn(char* pString) {
}

void chkres(char *func, FRESULT exp){
	if (exp!=FR_OK){
		dbgprint("%s ERR! res=", func);
		switch(exp){
		case FR_OK: dbgprint("FR_OK"); break;
		case FR_DISK_ERR: dbgprint("FR_DISK_ERR"); break;
		case FR_INT_ERR: dbgprint("FR_INT_ERR"); break;
		case FR_NOT_READY: dbgprint("FR_NOT_READY"); break;
		case FR_NO_FILE: dbgprint("FR_NO_FILE"); break;
		case FR_NO_PATH: dbgprint("FR_NO_PATH"); break;
		case FR_INVALID_NAME: dbgprint("FR_INVALID_NAME"); break;
		case FR_DENIED: dbgprint("FR_DENIED"); break;
		case FR_EXIST: dbgprint("FR_EXIST"); break;
		case FR_INVALID_OBJECT: dbgprint("FR_INVALID_OBJECT"); break;
		case FR_WRITE_PROTECTED: dbgprint("FR_WRITE_PROTECTED"); break;
		case FR_INVALID_DRIVE: dbgprint("FR_INVALID_DRIVE"); break;
		case FR_NOT_ENABLED: dbgprint("FR_NOT_ENABLED"); break;
		case FR_NO_FILESYSTEM: dbgprint("FR_NO_FILESYSTEM"); break;
		case FR_MKFS_ABORTED: dbgprint("FR_MKFS_ABORTED"); break;
		case FR_TIMEOUT: dbgprint("FR_TIMEOUT"); break;
		case FR_LOCKED: dbgprint("FR_LOCKED"); break;
		case FR_NOT_ENOUGH_CORE: dbgprint("FR_NOT_ENOUGH_CORE"); break;
		case FR_TOO_MANY_OPEN_FILES: dbgprint("FR_TOO_MANY_OPEN_FILES"); break;
		case FR_INVALID_PARAMETER: dbgprint("FR_INVALID_PARAMETER"); break;
		default: dbgprint("%d", exp);
		}
		dbgprint("\n");
	}
}


// SD Card access test.
void sdcheck()
{
	uint32_t nvals = BUFFER_SIZE/sizeof(double);
	double *vals = (double*)&buffer[0];
    
	FRESULT res;
	UINT BytesWritten, BytesRead;
	
	FATFS FileSys;

	dbgprint("Try to mount...\n");
	
	//initialise FatFS
	DSTATUS dret = disk_initialize(0);
	dbgprint("disk_initialize ret=%d\n", dret);
	if(dret) goto err_exit;

	//and mount.
	FRESULT nRes;
	if ((nRes = f_mount(&FileSys, "/", 1)) != FR_OK) {
		//Fail to mount...
		dbgprint("SD-CARD mount ERROR!! nRes=");
		switch(nRes) {    //reasons...
		case FR_INVALID_DRIVE:	dbgprint("FR_INVALID_DRIVE\n"); break;
		case FR_DISK_ERR:		dbgprint("FR_DISK_ERR\n"); break;
		case FR_NOT_READY:		dbgprint("FR_NOT_READY\n"); break;
		case FR_NO_FILESYSTEM:	dbgprint("FR_NO_FILESYSTEM\n"); break;
		default: break;
		}
		goto err_exit;
	} else {
		//Mounted
		dbgprint("SD-CARD mounted. FSTYPE=");
		switch(FileSys.fs_type) { //In case filesys type is of interest
		case FS_FAT12:	dbgprint("FAT12\n"); break;
		case FS_FAT16:	dbgprint("FAT16\n"); break;
		case FS_FAT32:	dbgprint("FAT32\n"); break;
		default:		dbgprint("UNKNOWN\n"); break;
		}
	}
	
	/// --------------------------------------------------------------------------
    CyDelay(500);

	//Now for the benchmarking step...
	dbgprint(">>>>Testing PIO mode...\n");
	SDSPI_SetDMAMode(0);

	//Open a file and write "Testing Output" to it
	res = f_open(&File, "//test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	chkres("f_open", res);
	if (res!=FR_OK) goto err_exit;

	dumpmsg("Testing Speed\n");

	//generate dataset...
	for(int i = 0; i < nvals; i++) {
		vals[i] = (double)i*i;
	}

	//First, we'll write the dataset to a file in the root direcory, time the operation, then
	//output results to test.txt in the SD card root.
	
	dumpmsg("Writing %d bytes to file, non-DMA\n", BUFFER_SIZE);

	//perform operation
	unsigned long nCurrentMillis = nmillis; //get current millisecond count
	FIL BinFile;
	res = f_open(&BinFile, "//TESTPIO.bin", FA_CREATE_ALWAYS | FA_WRITE);
	chkres("f_open", res);
	if (res==FR_OK) {
		res = f_write(&BinFile, vals, BUFFER_SIZE, &BytesWritten);
		chkres("f_write", res);
		res = f_close(&BinFile);
		chkres("f_close", res);
	}

	//indicate in test.txt how long it took
	unsigned long nTime = nmillis-nCurrentMillis;
	dumpmsg("Took %lu ms\n", nTime);
	unsigned long kbps = BUFFER_SIZE*8 / nTime;
	dumpmsg( "Rate %lu kbps\n\n", kbps);

	//Next, read file in
	dumpmsg("Reading %d bytes from file, non-DMA\n", BUFFER_SIZE);

	memset(vals, 0x00, BUFFER_SIZE);

	//perform operation
	nCurrentMillis = nmillis;

	res = f_open(&BinFile, "//TESTPIO.bin", FA_OPEN_EXISTING | FA_READ);
	chkres("f_open", res);
	if (res==FR_OK){
		res = f_read(&BinFile, vals, BUFFER_SIZE, &BytesRead);
		chkres("f_read", res);
		res = f_close(&BinFile);
		chkres("f_close", res);
	}

	//indicate in test.txt how long it took
	nTime = nmillis-nCurrentMillis;
	dumpmsg("Took %lu ms\nVerifying...", nTime);

	// verifying the result.
	unsigned char bVerifyFail = 0;
	for(int i = 0; i < 500; i++) {
		if(vals[(int)i] != i*i) bVerifyFail = 1;
	}
	if(bVerifyFail)
		dumpmsg("FAIL...\n");
	else
		dumpmsg(" All Good! :D\n");

	kbps = BUFFER_SIZE*8 / nTime;
	dumpmsg("Rate %lu kbps\n\n", kbps);

	// ----------------------------------------------------------------------------
	dbgprint(">>>>Testing DMA mode...\n");
	//Now, enable DMA...
	SDSPI_SetDMAMode(1);
    
	//and repeat tests
	dumpmsg( "Writing %d bytes to file, DMA\n", BUFFER_SIZE);

	//perform operation
	nCurrentMillis = nmillis;
	res = f_open(&BinFile, "//TESTDMA.bin", FA_CREATE_ALWAYS | FA_WRITE);
	chkres("f_open", res);
	if (res==FR_OK){
		res = f_write(&BinFile, vals, BUFFER_SIZE, &BytesWritten);
		chkres("f_write", res);
		res = f_close(&BinFile);
		chkres("f_close", res);
	}

	//indicate in test.txt how long it took
	nTime = nmillis-nCurrentMillis;
	dumpmsg("Took %lu ms\n", nTime);
	kbps = BUFFER_SIZE*8 / nTime;
	dumpmsg("Rate %lu kbps\n\n", kbps);

	dumpmsg("Reading %d bytes from file, DMA\n", BUFFER_SIZE);

	memset(vals, 0x00, BUFFER_SIZE);

	//perform operation
	nCurrentMillis = nmillis;

	res = f_open(&BinFile, "//TESTDMA.bin", FA_OPEN_EXISTING | FA_READ);
	chkres("f_open", res);
	if(res==FR_OK){
		res = f_read(&BinFile, vals, BUFFER_SIZE, &BytesRead);
		chkres("f_read", res);
		res = f_close(&BinFile);
		chkres("f_close", res);
	}

	nTime = nmillis-nCurrentMillis;
	dumpmsg("Took %lu ms\nVerifying...", nTime);
	// verifying the result.
	bVerifyFail = 0;
	for(int i = 0; i < 500; i++) {
		if(vals[i] != i*i) bVerifyFail = 1;
	}
	
	if(bVerifyFail)
		dumpmsg("FAIL :O\n");
	else
		dumpmsg(" All Good! :D\n");

	kbps = BUFFER_SIZE*8 / nTime;
	dumpmsg("Rate %lu kbps\n\n", kbps);

    CyDelay(1000);

	res = f_close(&File);
	chkres("f_close", res);


err_exit:
	LEDControl_Write(0);
	dbgprint("TEST FINISHED!\n");
}


// SD Card speed benchmark.
void sdbench()
{
	uint32_t nvals = BUFFER_SIZE/sizeof(double);
	double *vals = (double*)&buffer[0];
    
	FATFS FileSys;
	FRESULT res;
	UINT BytesWritten, BytesRead;
	uint32_t size = 1;
	uint32_t nrepeat = 0;
	const uint32_t nrepeat1 = 10000;
	const uint32_t nrepeat2 = 100;
	
	//generate dataset...
	for(int i = 0; i < nvals; i++) {
		vals[i] = (double)i*i;
	}

	dbgprint("Try to mount...\n");
	//initialise FatFS
	DSTATUS dret = disk_initialize(0);
	dbgprint("disk_initialize ret=%d\n", dret);
	if(dret) goto err_exit;

	FRESULT nRes;
	if ((nRes = f_mount(&FileSys, "/", 1)) != FR_OK) {
		//Fail to mount...
		dbgprint("SD-CARD mount ERROR!! nRes=");
		switch(nRes) {
		case FR_INVALID_DRIVE:	dbgprint("FR_INVALID_DRIVE\n"); break;
		case FR_DISK_ERR:		dbgprint("FR_DISK_ERR\n"); break;
		case FR_NOT_READY:		dbgprint("FR_NOT_READY\n"); break;
		case FR_NO_FILESYSTEM:	dbgprint("FR_NO_FILESYSTEM\n"); break;
		default: break;
		}
		goto err_exit;
	} else {
		//Mounted
		dbgprint("SD-CARD mounted. FSTYPE=");
		switch(FileSys.fs_type) {
		case FS_FAT12:	dbgprint("FAT12\n"); break;
		case FS_FAT16:	dbgprint("FAT16\n"); break;
		case FS_FAT32:	dbgprint("FAT32\n"); break;
		default:		dbgprint("UNKNOWN\n"); break;
		}
	}
	
	//Now, enable DMA...
	SDSPI_SetDMAMode(1);
	dbgprint(">>>>> Start Benchmark.....\n");
	dbgprint("size, write(ms), write(kbps), read(ms), read(kbps)\n");
	
	for(size=1; size<0x10000; size*=2){
		dbgprint("%d, ",size);

		if(size<0x20) nrepeat = nrepeat1;
		else nrepeat = nrepeat2;
		
		uint32_t nCurrentMillis;
		FIL BinFile;
		res = f_open(&BinFile, "//TESTDMA.bin", FA_CREATE_ALWAYS | FA_WRITE);
		if (res==FR_OK){
			nCurrentMillis = nmillis; //get current millisecond count
			for(uint32_t k=0; k<nrepeat; k++){
				//res = f_lseek(&BinFile, 0);
				res = f_write(&BinFile, vals, size, &BytesWritten);
			}
			uint32_t nTime = nmillis-nCurrentMillis;
			res = f_close(&BinFile);

			dbgprint("%u, ", nTime);
			uint32_t kbps = nrepeat*size*8 / nTime;
			dbgprint("%u, ", kbps);
		}

		res = f_open(&BinFile, "//TESTDMA.bin", FA_OPEN_EXISTING | FA_READ);
		if(res==FR_OK){
			nCurrentMillis = nmillis; //get current millisecond count
			for(uint32_t k=0; k<nrepeat; k++){
				//res = f_lseek(&BinFile, 0);
				res = f_read(&BinFile, vals, size, &BytesRead);
			}
			uint32_t nTime = nmillis-nCurrentMillis;
			res = f_close(&BinFile);

			dbgprint("%u, ", nTime);
			uint32_t kbps = nrepeat*size*8 / nTime;
			dbgprint("%u, ", kbps);
		}
		//CyDelay(1000);
		dbgprint("\n");
	}

err_exit:
	LEDControl_Write(0);
	dbgprint("TEST FINISHED!\n");
}


int main()
{
//    SysResetReg_Write(1);
//    CyDelay(1);
//    SysResetReg_Write(0);

	CyDelay(1000);
	LEDControl_Write(1);
	CyGlobalIntEnable;

	USBUART_1_Start(0, USBUART_1_DWR_VDDD_OPERATION);
	while(!USBUART_1_bGetConfiguration());
	USBUART_1_CDC_Init();

	//Initialise SD card and file system (note: PrintFxn can be NULL)
	SDSPI_Start(SDCARD_SLOW, SDCARD_FAST, PrintFxn);

	//start millisec ISR
	MillisecISR_StartEx(MillisISR);

	for(;;){
		for(int i=5; i>0; i--){
			dbgprint("%d.\n", i);
			CyDelay(1000);
		}
		dbgprint("Hello SDC\n");

		//sdcheck();
		sdbench();
	}


	for (;;) { } // never reach here.
}

/* [] END OF FILE */
