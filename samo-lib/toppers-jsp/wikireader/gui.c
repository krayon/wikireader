#include <wikilib.h>
#include <guilib.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: don't use bootloader includes */
#include <regs.h>

unsigned char *framebuffer;

void fb_init(void)
{
	framebuffer = (unsigned char *) malloc((guilib_framebuffer_width() * guilib_framebuffer_height()) / 2);
	REG_LCDC_MADD = (unsigned int) framebuffer;
}

void fb_clear(void)
{
	memset(framebuffer, 0, (guilib_framebuffer_width() * guilib_framebuffer_height()) / 2);
}

void fb_refresh(void)
{
	/* is there a pragma to omit the frame pointer? */
	return;
}

/* 1bpp implementation */

void fb_set_pixel(int x, int y, int val)
{
	char mask = 1 << (x % 8);
	unsigned int byte = (y * guilib_framebuffer_width() + x) / 8;

	framebuffer[byte] &= ~mask;

	if (val)
		framebuffer[byte] |= mask;
}
