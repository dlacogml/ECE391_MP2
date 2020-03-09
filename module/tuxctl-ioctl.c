/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

#define ZERO	0xE7
#define ONE		0x06
#define TWO		0xCB
#define THREE	0x8F
#define FOUR	0x2E
#define FIVE	0xAD
#define SIX		0xED
#define SEVEN	0x86
#define EIGHT	0xEF
#define NINE	0xAF
#define A		0xEE
#define B		0x6D
#define C		0xE1
#define D		0x4F
#define E		0xE9
#define F		0xE8
#define DECIMAL	0x10

static unsigned char letters[] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, A, B, C, D, E, F};
static void set_buttons(int buttons, int dirs);
int swapBits(unsigned int x, unsigned int p1, unsigned int p2, unsigned int n);
int the_buttons;
int reset_state = 0;
char save_led[6];

static void set_buttons(int buttons, int dirs) {
	int number = 0;
	buttons = ~buttons;
	dirs = ~dirs;

	number = 0x0F & buttons;
	
	printk("the condition is: %x",((dirs >> 1) & 1) ^ ((dirs >> 2) & 1));
	if(((dirs >> 1) & 1) ^ ((dirs >> 2) & 1)) {
		// then swap
		dirs = (dirs ^ 0x02);
		dirs = (dirs ^ 0x04); 
	}

	dirs = dirs << 4;
	printk("dirs is%x\n",dirs);
	number = number | dirs;

	the_buttons = number;
}


/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];
	if(MTCP_ACK){
		if(reset_state == 1) {
			unsigned char buffer[1];
			buffer[0] = MTCP_LED_USR;
			tuxctl_ldisc_put(tty, buffer, 1);
			reset_state = 2;
		}
		else if(reset_state == 2) {
			tuxctl_ldisc_put(tty, save_led, 6);
			reset_state = 0;
		}
	}

	if(a == MTCP_RESET) {
		unsigned char buffer[1];
		buffer[0] = MTCP_BIOC_ON;
		tuxctl_ldisc_put(tty, buffer, 1);
		reset_state = 1;
	}

	if(a == MTCP_BIOC_EVENT) {
		int buttons = b & 0x0f;
		int dir = c & 0x0f;
		set_buttons(buttons, dir);
	}

    printk("packet : %x %x %x\n", a, b, c);
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {

	case TUX_INIT: {
		unsigned char buffer[2];
		buffer[0] = MTCP_BIOC_ON;
		buffer[1] = MTCP_LED_USR;
		// EXPECT_BIOC_EVT = 1;
		if(tuxctl_ldisc_put(tty, buffer, 2	) != 0) {
			return -EINVAL;
		}
		return 0;
	}

	case TUX_BUTTONS: {
		int * ptr = (int *) arg;
		if(copy_to_user(ptr, &the_buttons, 1) != 0) {
			return -EINVAL;
		}
		return 0;
	}

	/* 
	 *
	 */
	case TUX_SET_LED: {
		int temp = arg;
		int i;
		int j;
		int k;
		unsigned char buffer[6];

		buffer[0] = MTCP_LED_SET;
		buffer[1] = 0x0F;

		// check each numbers
		for(i = 0; i < 4; i++) {
			unsigned char let;
			int num = temp & 0xF;
			temp = temp >> 4;
			let = letters[num];
			buffer[2 + i] = let;
		}
		// check which leds are turned on and which are turned off
		for(j = 0; j < 4; j++) {
			int turn_on = temp & 0x1;
			temp = temp >> 1;
			if(!turn_on) {
				buffer[5 - j] = 0x0;
			}
		}
		// skip the next four bits
		temp = temp >> 4;
		// check which decimals are on
		for(k = 0; k < 4; k++) {
			int decimal_on = temp & 0x1;
			temp = temp >> 1;
			if(decimal_on) {
				buffer[5 - k] += DECIMAL;
			}
		}	

		for(i = 0; i < 6; i++) {
			save_led[i] = buffer[i];
		}

		if(tuxctl_ldisc_put(tty, buffer, 6) != 0) {
			return -EINVAL;
		}
		return 0;
	}

	case TUX_LED_ACK: {
		return 0;
	}

	case TUX_LED_REQUEST: {
		return 0;
	}

	case TUX_READ_LED: {
		return 0;
	}

	default:
	    return -EINVAL;
    }
}

