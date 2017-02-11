/****************************************************************
 NAME: usbout.c
 DESC: USB bulk-OUT operation related functions
 HISTORY:
 Mar.25.2002:purnnamu: ported for S3C2410X.
 Mar.27.2002:purnnamu: DMA is enabled.
 ****************************************************************/
#include <common.h>
#if defined(CONFIG_S3C2400)
#include <s3c2400.h>
#elif defined(CONFIG_S3C2410)
#include <s3c2410.h>
#endif

#include <def.h>
 
#include "2440usb.h"
#include "usbmain.h"
#include "usb.h"
#include "usblib.h"
#include "usbsetup.h"
#include "usbout.h"

#include "usbinit.h"

extern volatile U32 dwUSBBufReadPtr;
extern volatile U32 dwUSBBufWritePtr;
extern volatile U32 dwWillDMACnt;
extern volatile U32 bDMAPending;
extern volatile U32 dwUSBBufBase;
extern volatile U32 dwUSBBufSize;

extern S3C24X0_INTERRUPT * intregs;
extern S3C24X0_USB_DEVICE * usbdevregs;
extern S3C24X0_DMAS * dmaregs;

static void PrintEpoPkt(U8 *pt,int cnt);
static void RdPktEp3_CheckSum(U8 *buf,int num);



// ===================================================================
// All following commands will operate in case 
// - out_csr3 is valid.
// ===================================================================

 

#define CLR_EP3_OUT_PKT_READY() usbdevregs->OUT_CSR1_REG= ( out_csr3 &(~ EPO_WR_BITS)\
					&(~EPO_OUT_PKT_READY) ) 
#define SET_EP3_SEND_STALL()	usbdevregs->OUT_CSR1_REG= ( out_csr3 & (~EPO_WR_BITS)\
					| EPO_SEND_STALL) )
#define CLR_EP3_SENT_STALL()	usbdevregs->OUT_CSR1_REG= ( out_csr3 & (~EPO_WR_BITS)\
					&(~EPO_SENT_STALL) )
#define FLUSH_EP3_FIFO() 	usbdevregs->OUT_CSR1_REG= ( out_csr3 & (~EPO_WR_BITS)\
					|EPO_FIFO_FLUSH) )

// ***************************
// *** VERY IMPORTANT NOTE ***
// ***************************
// Prepare for the packit size constraint!!!

// EP3 = OUT end point. 

U8 ep3Buf[EP3_PKT_SIZE];
static U8 tempBuf[64+1];

void Ep3Handler(void)
{
    U8 out_csr3;
	char buf[64];
	char cmdbuf[64];
	char response[64];
	char cmdline[128];
    int fifoCnt;
    usbdevregs->INDEX_REG = 3;

    out_csr3 = usbdevregs->OUT_CSR1_REG;
    
    DbgPrintf("<3:%x]",out_csr3);

    if(out_csr3 & EPO_OUT_PKT_READY)
    {   
		fifoCnt = usbdevregs->OUT_FIFO_CNT1_REG; 

		if(downloadFileSize == 0)
		{
	   	    RdPktEp3(cmdbuf, fifoCnt); //The first 8-bytes are deleted.	   
	   	    	if (memcmp(cmdbuf, "getvar:partition-type:", 22) == 0)
			{
				strncpy(response, "OKAY", 4);
				WrPktEp1(response, 4);
				ClearEp3OutPktReady();
				SetEp1InPktReady();
			}
			else if (memcmp(cmdbuf, "getvar:max-download-size", 24) == 0)
			{
				strncpy(response, "OKAY", 4);
				strncpy(response + 4, "03200000", 8);
				WrPktEp1(response, 12);
				ClearEp3OutPktReady();
				SetEp1InPktReady();
			}
			else if (memcmp(cmdbuf, "download:", 9) == 0)//download:00003800
			{
				strncpy(cmdbuf + 17,"Z",1);
				downloadFileSize = simple_strtoul (cmdbuf + 9, NULL, 16);
				sprintf(buf, "%X", downloadFileSize);
    			setenv("filesize", buf);
				
				printf("downsize %d\n",downloadFileSize);
				strncpy(response, "DATA", 4);
				strncpy(response + 4, cmdbuf + 9, 8);
				WrPktEp1(response, 12);
				ClearEp3OutPktReady();
				SetEp1InPktReady();
				//屏蔽USBD中断，此时先返回去初始化DMA，然后在 CLR_EP3_OUT_PKT_READY()进行下一次传输 		
				intregs->INTMSK = intregs->INTMSK | BIT_USBD;
			}
			else if (memcmp(cmdbuf, "flash:", 6) == 0)
			{
				if (memcmp(cmdbuf + 6, "bootloader", 10) == 0)
				{
					run_command("nand erase bootloader", 0);
					run_command("nand write 0x30000000 bootloader", 0);
					strncpy(response, "OKAY", 4);
					WrPktEp1(response, 4);
					ClearEp3OutPktReady();
					SetEp1InPktReady();
				}
				if (memcmp(cmdbuf + 6, "kernel", 6) == 0)
				{
					run_command("nand erase kernel", 0);
					run_command("nand write 0x30000000 kernel", 0);
					strncpy(response, "OKAY", 4);
					WrPktEp1(response, 4);
					ClearEp3OutPktReady();
					SetEp1InPktReady();
				}
				if (memcmp(cmdbuf + 6, "yaffs2", 6) == 0)
				{
					run_command("nand erase root", 0);
					run_command("nand write.yaffs 0x30000000 root $(filesize)", 0);
					strncpy(response, "OKAY", 4);
					WrPktEp1(response, 4);
					ClearEp3OutPktReady();
					SetEp1InPktReady();
				}
				if (memcmp(cmdbuf + 6, "jffs2", 5) == 0)
				{
					run_command("nand erase root", 0);
					run_command("nand write.jffs2 0x30000000 bootloader $(filesize)", 0);
					strncpy(response, "OKAY", 4);
					WrPktEp1(response, 4);
					ClearEp3OutPktReady();
					SetEp1InPktReady();
				}
				if (memcmp(cmdbuf + 6, "onlydownload", 12) == 0)
				{
					strncpy(response, "OKAY", 4);
					WrPktEp1(response, 4);
					ClearEp3OutPktReady();
					SetEp1InPktReady();
				}
			}
			else if(memcmp(cmdbuf, "reboot", 6) == 0)
			{
				run_command("reset", 0);
			}
	   	    
	  	    return;	

		}
		else
		{
			printf("error\n");
		}

	   	CLR_EP3_OUT_PKT_READY();

  		return;
    }

    
    //I think that EPO_SENT_STALL will not be set to 1.
    if(out_csr3 & EPO_SENT_STALL)
    {   
	   	DbgPrintf("[STALL]");
	   	CLR_EP3_SENT_STALL();
	   	return;
    }	
}



void PrintEpoPkt(U8 *pt,int cnt)
{
    int i;
    DbgPrintf("[BOUT:%d:",cnt);
    for(i=0;i<cnt;i++)
    	DbgPrintf("%x,",pt[i]);
    DbgPrintf("]");
}


void RdPktEp3_CheckSum(U8 *buf,int num)
{
    int i;
    	
    for(i=0;i<num;i++)
    {
        buf[i]=(U8)usbdevregs->fifo[3].EP_FIFO_REG;
        checkSum+=buf[i];
    }
}

void IsrDma2(void)
{
    U8 out_csr3;
    U32 dwEmptyCnt;
    U8 saveIndexReg=usbdevregs->INDEX_REG;
    usbdevregs->INDEX_REG=3;
    out_csr3=usbdevregs->OUT_CSR1_REG;

    ClearPending(BIT_DMA2);	    

    totalDmaCount += dwWillDMACnt;

    dwUSBBufWritePtr = dwUSBBufWritePtr + dwWillDMACnt;
	
    if(totalDmaCount>=downloadFileSize)// is last?
    {
    	totalDmaCount=downloadFileSize;
	
    	ConfigEp3IntMode();	

    	if(out_csr3& EPO_OUT_PKT_READY)
    	{
       	    CLR_EP3_OUT_PKT_READY();
	    }
        intregs->INTMSK |= BIT_DMA2;  
        intregs->INTMSK &= ~(BIT_USBD);  
    }
    else
    {
    	if((totalDmaCount + 0x80000) < downloadFileSize)	
    	{
    	    dwWillDMACnt = 0x80000;
	    }
    	else
    	{
    	    dwWillDMACnt = downloadFileSize - totalDmaCount;
    	}
		printf("current addr %x len %d\n", dwUSBBufWritePtr, dwWillDMACnt);
       	ConfigEp3DmaMode(dwUSBBufWritePtr, dwWillDMACnt);
    }
    usbdevregs->INDEX_REG = saveIndexReg;
}

void ClearEp3OutPktReady(void)
{
    U8 out_csr3;
    usbdevregs->INDEX_REG=3;
    out_csr3=usbdevregs->OUT_CSR1_REG;
    CLR_EP3_OUT_PKT_READY();
}

void SetEp1InPktReady(void)
{
    U8 in_csr1;
	usbdevregs->INDEX_REG = 1;
	in_csr1 = usbdevregs->EP0_CSR_IN_CSR1_REG;
    SET_EP1_IN_PKT_READY();
}
