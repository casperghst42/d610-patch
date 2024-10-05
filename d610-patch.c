/*  resolution patch for 845G/855GM/865G
*
*	Fixes the i810-series video BIOS to support other resolution
*	like 1280x768, 1024x600. Currently only tested against the
*	845G VBIOS, revision #2686 #2720 and the 865G VBIOS, revision
*	#2831 #2919 #3144.
*
*	*VERY* likely that this won't work yet on any other revisions
*	or chipsets!!!
*
*	This code is based on the technique used in 845Gpatch and 1280
*	patch for 855GM. Many thanks to Christian Zietz <czietz@gmx.net>
*	and Andrew Tipton <andrewtipton@null.li>.
*
*	resolution patch for 845G/855GM/865G is public domain source code.
*	Use at your own risk!
*
*	Leslie Chu <kongkong_chu@yahoo.ca>
*
*	Add NetBSD/FreeBSD support by FUKAUMI Naoki <fun AT naobsd DOT org>.
*
*	Original code:
*	  http://kongkong.no-ip.info:8080/kongkong/1280patch-845g-855gm-865g.c
*	Discussion:
*	  http://www.leog.net/fujp_forum/topic.asp?TOPIC_ID=5371
*
*/

#include <stdio.h>
#include <stdlib.h>
#if !defined(__FreeBSD__) && !defined(__NetBSD__)
#include <sys/io.h>
#endif
#include <unistd.h>
#if !defined(__FreeBSD__) && !defined(__NetBSD__)
#define __USE_GNU
#endif
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#if defined(__FreeBSD__)
#include <machine/cpufunc.h>
#define OUTB(a,b)	outb(b,a)
#define OUTL(a,b)	outl(b,a)
#elif defined(__NetBSD__)
#include <sys/types.h>
#include <machine/pio.h>
#include <machine/sysarch.h>
#define iopl	i386_iopl
#define OUTB(a,b)	outb(b,a)
#define OUTL(a,b)	outl(b,a)
#else
#define OUTB	outb
#define OUTL	outl
#endif

#define VBIOS					0xc0000
#define VBIOS_SIZE				0x10000
#define CFG_SIGNATURE			"BIOS_DATA_BLOCK "
#define CFG_VERSION				29

/* for 845G and old 865G vbios */
#define TIMING_CLOCK			0
#define TIMING_HTOTAL			4
#define TIMING_HBLANK			8
#define TIMING_HSYNC			12
#define TIMING_VTOTAL			16
#define TIMING_VBLANK			20
#define TIMING_VSYNC			24
#define TIMING_H				28
#define TIMING_W				30

#define TIMING_60				0
#define TIMING_75				1
#define TIMING_85				2

/* for 855GM and new 865G vbios (3144) */
#define TIMING_X1				2
#define TIMING_X2				4
#define TIMING_Y1				5
#define TIMING_Y2				7

/* I shouldn't use globals, but that's too bad! */
unsigned char *bios = 0;
int biosfd = 0;
unsigned char *bioscfg = 0;
int oldpam1, oldpam2;

struct vbios_mode
{
	unsigned char mode;
	unsigned char bits_per_pixel;
	unsigned short resolution;
	unsigned char unknow;
} __attribute__((packed));

int parse_args(int argc, char *argv[], int *mode, int *x, int *y, int *type) {

	*mode = *x = *y = 0;

	if (argc != 4 && argc != 5) {
		return -1;
	}

	*mode = (int) strtol(argv[1], NULL, 16);
	*x = atoi(argv[2]);
	*y = atoi(argv[3]);
	if (argc == 5)
		*type = atoi(argv[4]);

	return 0;
}

void usage(char *name) {
	printf("Usage: %s mode X Y [type]\n", name);
	printf("	Set the resolution to X*Y for mode\n");
}

unsigned int get_chipset() {
	OUTL(0x80000000, 0xcf8);
	return inl(0xcfc);
}

int unlock_bios_845g_865g() {
	// set permissions to read and write so I can write into shadowed VideoBIOS
	OUTL(0x80000090, 0xcf8);
	oldpam1 = inb(0xcfd);
	oldpam2 = inb(0xcfe);
	OUTL(0x80000090, 0xcf8);
	OUTB(0x33, 0xcfd);
	OUTB(0x33, 0xcfe);

	return 1;
}

int relock_bios_845g_865g() {
	// reset permissions
	OUTL(0x80000090, 0xcf8);
	OUTB(oldpam1, 0xcfd);
	OUTB(oldpam2, 0xcfe);  

	return 1;
}

int unlock_bios_855gm() {
	// set permissions to read and write so I can write into shadowed VideoBIOS
	OUTL(0x8000005a, 0xcf8);
	OUTB(0x33, 0xcfe);

	return 1;
}

int relock_bios_855gm() {
	// reset permissions
	OUTL(0x8000005a, 0xcf8);
	OUTB(0x11, 0xcfe);

	return 1;
}

void open_bios() {
	biosfd = open("/dev/mem", O_RDWR);
	if (biosfd < 0) {
		perror("Unable to open /dev/mem");
		exit(2);
	}

	bios = mmap((void *)VBIOS, VBIOS_SIZE,
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
		biosfd, VBIOS);
	if (bios == NULL) {
		fprintf(stderr, "Cannot mmap() the video VBIOS\n");
		close(biosfd);
		exit(2);
	}
}

void close_bios() {
	if (bios == NULL) {
		fprintf(stderr, "VBIOS should be open already!\n");
		exit(2);
	}

	munmap(bios, VBIOS_SIZE);
	close(biosfd);
}

/* Finds the EMode table in the BIOS */
unsigned char *find_emode_table() {
	struct vbios_mode *modes;
	unsigned char *p = bios;

	while(p < (bios + VBIOS_SIZE - 3 * sizeof(struct vbios_mode))) {
		modes = (struct vbios_mode *) p;
 
		if (((modes[0].mode == 0x30) && (modes[1].mode == 0x32) && (modes[2].mode == 0x34))			// for 855GM/865G vbios
			|| ((modes[0].mode == 0x30) && (modes[1].mode == 0x31) && (modes[2].mode == 0x32))){	// for 845G vbios
			return p;
		}
		p++;
	}

	return NULL;
}

/* Lists the modes available */
void list_modes(unsigned char *table) {
	return;
}

/* Returns a pointer to the parameter block for a mode */
unsigned char *get_emode_params(unsigned char *table, int mode) {
	unsigned short offset;

	while (*table != 0xff) {
		if (*table == mode) {
			offset = *(unsigned short *)(table + 2);
			printf("Timing parameter block: %04x\n", offset);
			return (bios + offset);
		}
		table += 5;			/* next record */
	}

	return 0;
}

/* Given an XFree86-compatible modeline, replaces the
 * timing block at *ptr.  No error checking yet!
 */
void set_timing(unsigned char *ptr, int type, int timing, int clock,
	int hdisp, int hsyncstart, int hsyncend, int htotal,
	int vdisp, int vsyncstart, int vsyncend, int vtotal) {

	switch (type) {
	case 1:		/* for 855GM and new 865G vbios (3144) */
		*(unsigned char *)(ptr + 18 * timing + TIMING_X1) = hdisp & 0xff;
		*(unsigned char *)(ptr + 18 * timing + TIMING_X2) = (*(unsigned char *)(ptr + 18 * timing + TIMING_X2) & 0x0f) | ((hdisp >> 4) & 0xf0);
		*(unsigned char *)(ptr + 18 * timing + TIMING_Y1) = vdisp & 0xff;
		*(unsigned char *)(ptr + 18 * timing + TIMING_Y2) = (*(unsigned char *)(ptr + 18 * timing + TIMING_Y2) & 0x0f) | ((vdisp >> 4) & 0xf0);;
		break;
	case 2:		/* for 845G vbios (2686/2720) */
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_CLOCK) = clock;
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_HTOTAL) = (htotal << 16) | (hdisp - 1);
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_HBLANK) = (htotal << 16) | (hdisp - 1);
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_HSYNC) = (hsyncend << 16) | hsyncstart;
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_VTOTAL) = (vtotal << 16) | (vdisp - 1);
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_VBLANK) = (vtotal << 16) | (vdisp - 1);
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_VSYNC) = (vsyncend << 16) | vsyncstart;
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_H) = vdisp - 1;
		*(unsigned int *)(ptr + 6 + 38 * timing + TIMING_W) = hdisp - 1;
		break;
	case 3:		/* for old 865G vbios (2831/2919) */
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_CLOCK) = clock;
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_HTOTAL) = (htotal << 16) | (hdisp - 1);
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_HBLANK) = (htotal << 16) | (hdisp - 1);
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_HSYNC) = (hsyncend << 16) | hsyncstart;
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_VTOTAL) = (vtotal << 16) | (vdisp - 1);
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_VBLANK) = (vtotal << 16) | (vdisp - 1);
		*(unsigned int *)(ptr + 6 + 28 * timing + TIMING_VSYNC) = (vsyncend << 16) | vsyncstart;
		break;
	default:	/* other vbios */
		fprintf(stderr, "Couldn't handle this VBIOS version now!\n");
		break;
	}
}

int main (int argc, char *argv[]) {
	unsigned int chipset;
	unsigned char *emode_table;
	unsigned char *mode_params;
	unsigned int mode, x, y;
	unsigned int vbios_type = 0;
#if defined(__FreeBSD__)
	int deviofd;
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__)
	const char *vbios_sign = CFG_SIGNATURE;
#endif

	if (parse_args(argc, argv, &mode, &x, &y, &vbios_type) == -1) {
		usage(argv[0]);
		return 2;
	}

#if defined(__FreeBSD__)
	deviofd = open("/dev/io", O_RDONLY);
	if (deviofd == -1) {
#else
	if (iopl(3) < 0) {
#endif
		perror("Unable to obtain the proper IO permissions");
		exit(2);
	}

	open_bios();

	/* Find the configuration area of the BIOS */
#if defined(__FreeBSD__) || defined(__NetBSD__)
	for (bioscfg = bios;
	     memcmp(bioscfg, vbios_sign, strlen(vbios_sign)) != 0;)
	{
		bioscfg++;
		bioscfg = memchr(bioscfg, vbios_sign[0],
			(VBIOS + VBIOS_SIZE) - (unsigned long)bioscfg);
	}
#else
	bioscfg = memmem(bios, VBIOS_SIZE, CFG_SIGNATURE, strlen(CFG_SIGNATURE));
#endif
	if (bioscfg == NULL) {
		fprintf(stderr, "Couldn't find the configuration area in the VBIOS!\n");
		close_bios();
		exit(2);
	}

	printf("VBIOS Configuration area offset: 0x%04x bytes\n", bioscfg - bios);
	if (*(bioscfg + CFG_VERSION) < 0x31) {
		printf("VBIOS Version: %.4s\n", bioscfg + CFG_VERSION + 2);
	} else {
		printf("VBIOS Version: %.4s\n", bioscfg + CFG_VERSION);
	}

	chipset = get_chipset();
	printf("Chipset: ");
	switch (chipset) {
	//case 0x25608086:
	case 0x25908086:
		printf("845G\n");
		unlock_bios_845g_865g();
		break;
	case 0x35808086:
		printf("855GM\n");
		unlock_bios_855gm();
		break;
	case 0x25708086:
		printf("865G\n");
		unlock_bios_845g_865g();
		break;
	default:
		printf("Unknown (0x%08x)\n", chipset);
		close_bios();
		exit(2);
	}

	/* Get the timing block */
	emode_table = find_emode_table();
	mode_params = get_emode_params(emode_table, mode);

	if (vbios_type == 0)
	{
	/* Check VBIOS type 0x2f80/1024x768 0x2464/800x600 0x1d50/640x480 */
	if (!((*(unsigned char *)(mode_params) == 0x80) && (*(unsigned char *)(mode_params + 1) == 0x2f))
		&& !((*(unsigned char *)(mode_params) == 0x64) && (*(unsigned char *)(mode_params + 1) == 0x24))
		&& !((*(unsigned char *)(mode_params) == 0x50) && (*(unsigned char *)(mode_params + 1) == 0x1d))) {
		/* for 855GM and new 865G vbios (3144) */
		vbios_type = 1;
	} else if ((((*(unsigned char *)(mode_params) == 0x80) && (*(unsigned char *)(mode_params + 1) == 0x2f))
		|| ((*(unsigned char *)(mode_params) == 0x64) && (*(unsigned char *)(mode_params + 1) == 0x24))
		|| ((*(unsigned char *)(mode_params) == 0x50) && (*(unsigned char *)(mode_params + 1) == 0x1d)))
		&& (((*(unsigned char *)(mode_params + 6 + 28) == 0xff) && (*(unsigned char *)(mode_params + 6 + 28 + 1) == 0x02))
		|| ((*(unsigned char *)(mode_params + 6 + 28) == 0x57) && (*(unsigned char *)(mode_params + 6 + 28 + 1) == 0x02))
		|| ((*(unsigned char *)(mode_params + 6 + 28) == 0xdf) && (*(unsigned char *)(mode_params + 6 + 28 + 1) == 0x01)))) {
		/* for 845G vbios (2686/2720) */
		vbios_type = 2;
	} else if ((((*(unsigned char *)(mode_params) == 0x80) && (*(unsigned char *)(mode_params + 1) == 0x2f))
		|| ((*(unsigned char *)(mode_params) == 0x64) && (*(unsigned char *)(mode_params + 1) == 0x24))
		|| ((*(unsigned char *)(mode_params) == 0x50) && (*(unsigned char *)(mode_params + 1) == 0x1d)))
		&& (((*(unsigned char *)(mode_params + 6 + 28) != 0xff) && (*(unsigned char *)(mode_params + 6 + 28 + 1) != 0x02))
		&& ((*(unsigned char *)(mode_params + 6 + 28) != 0x57) && (*(unsigned char *)(mode_params + 6 + 28 + 1) != 0x02))
		&& ((*(unsigned char *)(mode_params + 6 + 28) != 0xdf) && (*(unsigned char *)(mode_params + 6 + 28 + 1) != 0x01)))) {
		/* for old 865G vbios (2831/2919) */
		vbios_type = 3;
	} else {
		/* other vbios */
		vbios_type = 0;
		fprintf(stderr, "Couldn't handle this VBIOS type now!\n");
		close_bios();
		exit(2);
	}
	}

	printf("VBIOS Type: %d\n", vbios_type);

	/* Now it's time to patch the BIOS */
	if (x == 1280 && y == 768) {				/* 1280x768 */
		set_timing(mode_params, vbios_type, TIMING_60, 80140, 1280, 1343, 1479, 1679, 768, 768, 771, 794);
		set_timing(mode_params, vbios_type, TIMING_75, 102980, 1280, 1359, 1495, 1711, 768, 768, 771, 801);
		set_timing(mode_params, vbios_type, TIMING_85, 118530, 1280, 1367, 1503, 1727, 768, 768, 771, 806);
	} else if (x == 1024 && y == 600) {			/* 1024x600 */
		set_timing(mode_params, vbios_type, TIMING_60, 65000, 1024, 1032, 1176, 1344, 600, 637, 643, 666);
		set_timing(mode_params, vbios_type, TIMING_75, 65000, 1024, 1032, 1176, 1344, 600, 637, 643, 666);
		set_timing(mode_params, vbios_type, TIMING_85, 65000, 1024, 1032, 1176, 1344, 600, 637, 643, 666);
	} else if (x == 1400 && y == 1050) {
		set_timing(mode_params, vbios_type, TIMING_60, 176640, 1400, 1432, 2096, 2128, 1050, 1070, 1083, 1103);
		set_timing(mode_params, vbios_type, TIMING_75, 176640, 1400, 1432, 2096, 2128, 1050, 1070, 1083, 1103);
		set_timing(mode_params, vbios_type, TIMING_85, 176640, 1400, 1432, 2096, 2128, 1050, 1070, 1083, 1103);
	} else {									/* other mode */
		fprintf(stderr, "Couldn't handle this mode now!\n");
		close_bios();
		exit(2);
	}

	/* Finished */
	switch (chipset) {
	case 0x25608086:
		relock_bios_845g_865g();
		break;
	case 0x35808086:
		relock_bios_855gm();
		break;
	case 0x25708086:
		relock_bios_845g_865g();
		break;
	default:
		close_bios();
		exit(2);
	}

	close_bios();

	printf("Patch complete.\n");
	return 0;
}
