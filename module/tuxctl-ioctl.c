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
#define TWO		0xC8
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

    /*printk("packet : %x %x %x\n", a, b, c); */
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
		tuxctl_ldisc_put(tty, buffer, 2);
		return 0;
	}

	case TUX_BUTTONS: {

		return 0;
	}

	/* arg - 
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


		for(i = 0; i < 4; i++) {
			int num = temp & 0xF;
			temp = temp >> 4;
			unsigned char let;
			let = letters[num];
			buffer[3 + i] = let;
		}
		for(j = 0; j < 4; j++) {
			int turn_on = temp & 0x1;
			temp = temp >> 1;
			if(!turn_on) {
				buffer[4 - j] = 0x0;
			}
		}
		temp =temp >> 4;
		for(k = 0; k < 4; k++) {
			int decimal_on = temp & 0x1;
			temp = temp >> 1;
			if(decimal_on) {
				buffer[4 - k] += DECIMAL;
			}
		}	

		tuxctl_ldisc_put(tty, buffer, 6);
		return 0;

		// // get the last four bits in decimal form for the fourth number
		// // change numbers to hex later
		// int fourth_num = arg & 15;
		// unsigned char fourth_letter = letters[fourth_num];

		// // get the next four bits from last in decimal form for the third number
		// int third_num = arg & 240;
		// unsigned char third_letter = letters[third_num];

		// // get the next four bits from last in decimal form for the second number
		// int second_num = arg & 3840;
		// unsigned char second_letter = letters[second_num];

		// // get the next four bits from last in decimal form for the first number
		// int first_num = arg & 61440;
		// unsigned char first_letter = letters[first_num];

		// int first_on = arg & 524288;
		// int second_on = arg & 262144;
		// int third_on = arg & 131072;
		// int fourth_on = arg & 65536;

		// int first_decimal = arg & 134217728;
		// int second_decimal = arg & 67108864;
		// int third_decimal = arg & 33554432;
		// int fourth_decimal = arg & 16777216;

		// if(fourth_on) {
		// 	if(fourth_decimal) {
		// 		buffer[2] = fourth_letter + DECIMAL;
		// 	} else {
		// 		buffer[2] = fourth_letter;
		// 	}

		// } else {
		// 	if(fourth_decimal) {
		// 		buffer[2] = DECIMAL;
		// 	}
		// }
		// if(third_on) {
		// 	if(third_decimal) {
		// 		buffer[3] = third_letter + DECIMAL;
		// 	} else {
		// 		buffer[3] = third_letter;
		// 	}
		// } else {
		// 	if(third_decimal) {
		// 		buffer[3] = DECIMAL;
		// 	}
		// }
		// if(second_on) {
		// 	if(second_decimal) {
		// 		buffer[4] = second_letter + DECIMAL;
		// 	} else {
		// 		buffer[4] = second_letter;
		// 	}
		// } else {
		// 	if(second_decimal) {
		// 		buffer[4] = DECIMAL;
		// 	}
		// }
		// if(first_on) {
		// 	if(first_decimal) {
		// 		buffer[5] = first_letter + DECIMAL;
		// 	} else {
		// 		buffer[5] = first_letter;
		// 	}
		// } else {
		// 	if(first_decimal) {
		// 		buffer[5] = DECIMAL;
		// 	}
		// }
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

