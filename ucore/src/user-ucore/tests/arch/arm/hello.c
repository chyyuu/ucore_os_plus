#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stat.h>
#include <file.h>
#include <dir.h>
#include <unistd.h>
#include <malloc.h>
#include "goldfish_logo.h"

#ifndef __u32
#define __u32 uint32_t
#endif

struct fb_bitfield {
	__u32 offset;		/* beginning of bitfield        */
	__u32 length;		/* length of bitfield           */
	__u32 msb_right;	/* != 0 : Most significant bit is */
	/* right */
};

struct fb_var_screeninfo {
	__u32 xres;		/* visible resolution           */
	__u32 yres;
	__u32 xres_virtual;	/* virtual resolution           */
	__u32 yres_virtual;
	__u32 xoffset;		/* offset from virtual to visible */
	__u32 yoffset;		/* resolution                   */

	__u32 bits_per_pixel;	/* guess what                   */
	__u32 grayscale;	/* != 0 Graylevels instead of colors */

	struct fb_bitfield red;	/* bitfield in fb mem if true color, */
	struct fb_bitfield green;	/* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency                 */

	__u32 nonstd;		/* != 0 Non standard pixel format */

	__u32 activate;		/* see FB_ACTIVATE_*            */

	__u32 height;		/* height of picture in mm    */
	__u32 width;		/* width of picture in mm     */

	__u32 accel_flags;	/* (OBSOLETE) see fb_info.flags */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	__u32 pixclock;		/* pixel clock in ps (pico seconds) */
	__u32 left_margin;	/* time from sync to picture    */
	__u32 right_margin;	/* time from picture to sync    */
	__u32 upper_margin;	/* time from sync to picture    */
	__u32 lower_margin;
	__u32 hsync_len;	/* length of horizontal sync    */
	__u32 vsync_len;	/* length of vertical sync      */
	__u32 sync;		/* see FB_SYNC_*                */
	__u32 vmode;		/* see FB_VMODE_*               */
	__u32 rotate;		/* angle we rotate counter clockwise */
	__u32 reserved[5];	/* Reserved for future compatibility */
};

#define FBIOGET_VSCREENINFO     0x4600

static void logo_loop()
{
	int fd;
	struct fb_var_screeninfo vinfo;

	int i, j;
	int w = 800;
	int h = 600;
	int x = (w - logo_width) / 2;
	int y = (h - logo_height) / 2;
	int v = 2, u = 2;
	fd = open("fb0:", O_RDWR);
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
		cprintf("ioctl error\n");
	else {
		cprintf("fb0 vinfo: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres,
			vinfo.bits_per_pixel);
		w = vinfo.xres;
		h = vinfo.yres;
	}
	close(fd);

	assert(logo_height < h && logo_width < w);

	int size = w * h * vinfo.bits_per_pixel / 8;

	fd = open("fb0:", O_RDWR);
	unsigned short *buf = (unsigned short *)sys_linux_mmap(0, size, fd, 0);
	cprintf("linux_mmap %08x\n", buf);
	//close(fd);

	//unsigned short* buf = (unsigned short*)malloc(size);
	//assert(buf);
	memset(buf, 0x00, size);
	while (1) {
		x += u;
		y += v;
		if (x < 0) {
			x = 0;
			u = -u;
		} else if (x + logo_width >= w) {
			x = w - logo_width - 1;
			u = -u;
		}
		if (y < 0) {
			y = 0;
			v = -v;
		} else if (y + logo_height >= h) {
			y = h - logo_height - 1;
			v = -v;
		}
		//fd = open("fb0:", O_RDWR);
		//cprintf("fd = %d\n", fd);
		if (fd < 0)
			return;
		unsigned char pixel[4];
		uint16_t color;
		char *data = logo_header_data;
		for (i = y * w; i < w * (y + logo_height); i += w) {
			for (j = x; j < x + logo_width; j++) {
				HEADER_PIXEL(data, pixel);
				color = ((pixel[0] >> 3) << 11)
				    | ((pixel[1] >> 2) << 5)
				    | ((pixel[2] >> 3));
				buf[i + j] = color;
			}
		}
		//write(fd, buf, size);
		//close(fd);
		sleep(2);
	}

	free(buf);
	close(fd);

}

int main(int argc, char *argv[])
{
	int i;
	cprintf("Hello world!!.\n");
	cprintf("I am process %d.\n", getpid());
	cprintf("ARG: %d:\n", argc);
	for (i = 0; i < argc; i++)
		cprintf("ARG %d: %s\n", i, argv[i]);

	int fd = -1;
#if 0
	int test = 1234567;
	fd = open("hzfchar:", O_RDWR);
	cprintf("fd = %d\n", fd);
	test = write(fd, &test, sizeof(int));
	cprintf("ret = %d\n", test);
	i = read(fd, &test, sizeof(int));
	cprintf("ret = %d, %d\n", i, test);
	i = read(fd, &test, sizeof(int));
	cprintf("ret = %d, %d\n", i, test);
#endif
	logo_loop();

	cprintf("hello pass.\n");
	return 0;
}
