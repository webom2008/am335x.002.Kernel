#ifdef STATIC
/* Pre-boot environment: included */

/* prevent inclusion of _LINUX_KERNEL_H in pre-boot environment: lots
 * errors about console_printk etc... on ARM */
#define _LINUX_KERNEL_H

#include "zlib_inflate/inftrees.c"
#include "zlib_inflate/inffast.c"
#include "zlib_inflate/inflate.c"

#else /* STATIC */
/* initramfs et al: linked */

#include <linux/zutil.h>
#include "zlib_inflate/inftrees.h"
#include "zlib_inflate/inffast.h"
#include "zlib_inflate/inflate.h"

#include "zlib_inflate/infutil.h"

#endif /* STATIC */

#include <linux/decompress/mm.h>

#define GZIP_IOBUF_SIZE (16*1024)

static int INIT nofill(void *buffer, unsigned int len)
{
	return -1;
}

//Add by QWB 20150609 for initilze watchdog used GPIO1.30
#define CM_PER_BASE                 (unsigned int)(0x44E00000)
#define CM_PER_L4LS_CLKSTCTRL       (unsigned int)(CM_PER_BASE+0x0)
#define CM_PER_GPIO1_CLKCTRL        (unsigned int)(CM_PER_BASE+0xAC)
#define GPIO1_BASE                  (unsigned int)(0x4804C000)
#define GPIO1_OE                    (unsigned int)(GPIO1_BASE+0x134)
#define GPIO1_CLEARDATAOUT          (unsigned int)(GPIO1_BASE+0x190)
#define GPIO1_SETDATAOUT            (unsigned int)(GPIO1_BASE+0x194)

static void INIT early_ext_wdt_reset(void)
{
    *(volatile unsigned int *)GPIO1_SETDATAOUT = (unsigned int)(0x1 << 30);
    *(volatile unsigned int *)GPIO1_CLEARDATAOUT = (unsigned int)(0x1 << 30);
}

//because u-boot had initilze this function, so needn't to init again.
#if 0
static void INIT early_watchdog_init(void)
{
    //1. configure GPIO1 clock
    //CM_PER Registers
    //CM_PER_GPIO1_CLKCTRL config
    //CM_PER_L4LS_CLKSTCTRL config
    
    //2. gpio port function
    //CONTROL_MODULE Registers

    //3. reset GPIO module
    //SOFTRESET R/W Software reset

    //4. set GPIO direction
    //default all input function 0xFFFFFFFF
    *(volatile unsigned int *)GPIO1_OE &= (unsigned int)0xBFFFFFFF; //Pin30=0x0 as output

    //5. set GPIO data
    early_watchdog_reset();
}
#endif

/* Included from initramfs et al code */
STATIC int INIT gunzip(unsigned char *buf, int len,
		       int(*fill)(void*, unsigned int),
		       int(*flush)(void*, unsigned int),
		       unsigned char *out_buf,
		       int *pos,
		       void(*error)(char *x)) {
	u8 *zbuf;
	struct z_stream_s *strm;
	int rc;
	size_t out_len;
    
	rc = -1;
	if (flush) {
		out_len = 0x8000; /* 32 K */
		out_buf = malloc(out_len);
	} else {
		out_len = 0x7fffffff; /* no limit */
	}
	if (!out_buf) {
		error("Out of memory while allocating output buffer");
		goto gunzip_nomem1;
	}

	if (buf)
		zbuf = buf;
	else {
		zbuf = malloc(GZIP_IOBUF_SIZE);
		len = 0;
	}
	if (!zbuf) {
		error("Out of memory while allocating input buffer");
		goto gunzip_nomem2;
	}

	strm = malloc(sizeof(*strm));
	if (strm == NULL) {
		error("Out of memory while allocating z_stream");
		goto gunzip_nomem3;
	}

	strm->workspace = malloc(flush ? zlib_inflate_workspacesize() :
				 sizeof(struct inflate_state));
	if (strm->workspace == NULL) {
		error("Out of memory while allocating workspace");
		goto gunzip_nomem4;
	}

	if (!fill)
		fill = nofill;

	if (len == 0)
		len = fill(zbuf, GZIP_IOBUF_SIZE);

	/* verify the gzip header */
	if (len < 10 ||
	   zbuf[0] != 0x1f || zbuf[1] != 0x8b || zbuf[2] != 0x08) {
		if (pos)
			*pos = 0;
		error("Not a gzip file");
		goto gunzip_5;
	}

	/* skip over gzip header (1f,8b,08... 10 bytes total +
	 * possible asciz filename)
	 */
	strm->next_in = zbuf + 10;
	strm->avail_in = len - 10;
	/* skip over asciz filename */
	if (zbuf[3] & 0x8) {
		do {
			/*
			 * If the filename doesn't fit into the buffer,
			 * the file is very probably corrupt. Don't try
			 * to read more data.
			 */
			if (strm->avail_in == 0) {
				error("header error");
				goto gunzip_5;
			}
			--strm->avail_in;
		} while (*strm->next_in++);
	}

	strm->next_out = out_buf;
	strm->avail_out = out_len;

	rc = zlib_inflateInit2(strm, -MAX_WBITS);

	if (!flush) {
		WS(strm)->inflate_state.wsize = 0;
		WS(strm)->inflate_state.window = NULL;
	}

	while (rc == Z_OK) {
        early_ext_wdt_reset();
		if (strm->avail_in == 0) {
			/* TODO: handle case where both pos and fill are set */
			len = fill(zbuf, GZIP_IOBUF_SIZE);
			if (len < 0) {
				rc = -1;
				error("read error");
				break;
			}
			strm->next_in = zbuf;
			strm->avail_in = len;
		}
		rc = zlib_inflate(strm, 0);

		/* Write any data generated */
		if (flush && strm->next_out > out_buf) {
			int l = strm->next_out - out_buf;
			if (l != flush(out_buf, l)) {
				rc = -1;
				error("write error");
				break;
			}
			strm->next_out = out_buf;
			strm->avail_out = out_len;
		}

		/* after Z_FINISH, only Z_STREAM_END is "we unpacked it all" */
		if (rc == Z_STREAM_END) {
			rc = 0;
			break;
		} else if (rc != Z_OK) {
			error("uncompression error");
			rc = -1;
		}
	}

	zlib_inflateEnd(strm);
	if (pos)
		/* add + 8 to skip over trailer */
		*pos = strm->next_in - zbuf+8;

gunzip_5:
	free(strm->workspace);
gunzip_nomem4:
	free(strm);
gunzip_nomem3:
	if (!buf)
		free(zbuf);
gunzip_nomem2:
	if (flush)
		free(out_buf);
gunzip_nomem1:
	return rc; /* returns Z_OK (0) if successful */
}

#define decompress gunzip
