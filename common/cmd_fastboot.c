/*
 *  www.100ask.net,°ÍÔúºÚ
 */
#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <def.h>

#ifdef CONFIG_USB_DEVICE
extern int usb_init(void);
extern int download_run;
extern volatile U32 dwUSBBufBase;
extern volatile u32 dwUSBBufSize;

extern __u32 usb_receive(char *buf, size_t len, U32 wait);
extern volatile U32 downloadAddress;
extern volatile int isUsbdSetConfiguration;

#define DOWNADDR 0x31000000

static void enable_irq(void) {
	asm volatile (
		"mrs r4,cpsr\n\t"
		"bic r4,r4,#0x80\n\t"
		"msr cpsr,r4\n\t"
		:::"r4"
	);
}

int do_fastboot (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	usb_init();
    	usb_receive(0, 0, 0);   
	return 0;
}

U_BOOT_CMD(
	fastboot,	3,	0,	do_fastboot,
	"usbslave - get file from host(PC)\n",
	"[wait] [loadAddress]\n"
	"\"wait\" is 0 or 1, 0 means for return immediately, not waits for the finish of transferring\n"
);

#endif






