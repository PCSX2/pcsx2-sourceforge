
#include "eestart.h"
#include "iopstart.h"

__asm__ (
	".org 0\n"
	".set noat\n"

	".global _start\n"
	"_start:\n"
	"mfc0  $at, $15\n"
	"sltiu $at, 0x59\n"
	"bne   $at, $0, __iopstart\n"
	"j     eestart\n"
	"__iopstart:\n"
	"j     iopstart");


/*
void _start() __attribute__ ((noreturn));
void _start() {
	register unsigned long PRid;

	__asm__ ("mfc0 %0, $15" : "=r"(PRid) : );
	if (PRid >= 0x59) eestart();
	else iopstart();
}*/
