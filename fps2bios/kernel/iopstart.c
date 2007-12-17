
#include "iopstart.h"
#include "romdir.h"

static void Kputc(u8 c) {
	*((u8*)0x1f80380c) = c;
}

static void Kputs(u8 *s) {
	while (*s != 0) {
		Kputc(*s++);
	}
}

static void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}

static void _iopstart() {
	struct rominfo ri;
	u8 *str;

	romdirGetFile("ROMVER", &ri);
	str = (u8*)(0xbfc00000 + ri.fileOffset);
	Kputs("iopstart: fps2bios v");
	Kputc(str[1]); Kputc('.'); Kputc(str[3]); Kputc('\n');

	romdirGetFile("IOPLOAD", &ri);

	Kputs("_iopstart: loading IOPLOAD to 0x80000000\n");
	Kmemcpy((void*)0x80000000, (void*)(0xbfc00000 + ri.fileOffset), ri.fileSize);

	__asm__ (
		"li  $26, 0x80001000\n"
		"jr  $26\n");
	for (;;);
}

__asm__ (
	".global iopstart\n"
	"iopstart:\n"
	"li	$sp, 0x80010000\n"
	"j     _iopstart\n");


