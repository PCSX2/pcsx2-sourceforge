
#include <tamtypes.h>
#include <stdio.h>

#include "eeload.h"
#include "eeinit.h"
#include "eedebug.h"

void __attribute__((noreturn)) eeload_start() {
	void (*entry)();
	__puts("EELOAD start\n");

	__printf("about to SifInitRpc(0)\n");
	SifInitRpc(0);
    __printf("done rpc\n");

	entry = (void (*)())loadElfFile("INTRO");
	entry();

	entry = (void (*)())loadElfFile("LOADER");
	entry();

	for (;;);
}

void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}

int _ResetSif1()
{
    sif1tagdata = 0xFFFF001E;
    //*(int*)0xa0001e330 = 0x20000000;
    //*(int*)0xa0001e334 = (u32)ptag&0x0fffffff;
    D6_QWC = 0;
    D6_TAG = (u32)&sif1tagdata&0x0fffffff;
}

void SifDmaInit()
{
    int msflg;
	memset(sifEEbuff, 0, sizeof(sifEEbuff));
	memset(sifRegs, 0, sizeof(sifRegs));

	*(u32*)0xB000F260 = 0xFF;
	D5_CHCR = 0;
	D6_CHCR = 0;

	_SifSetDChain();
	*(u32*)0xB000F200 = sifEEbuff;
	__printf("MSFLG = 0x10000\n");
	SBUS_MSFLG = 0x10000;
	msflg = SBUS_MSFLG;
    _ResetSif1();
	_SifSetReg(1, 0);

	while (!(_SifGetReg(4) & 0x10000)) { __asm__ ("nop\nnop\nnop\nnop\n"); }

	sifIOPbuff = *(u32*)0xB000F210;

	SBUS_MSFLG = 0x20000;
	SBUS_MSFLG;
}

int  _TlbWriteRandom(u32 PageMask, u32 EntryHi, u32 EntryLo0, u32 EntryLo1) {
	if ((EntryHi >> 24) != 4) return -1;
	__asm__ (
		"mfc0 $2, $1\n"
		"mtc0 $2, $0\n"
		"mtc0 $4, $5\n"
		"mtc0 $5, $10\n"
		"mtc0 $6, $2\n"
		"mtc0 $7, $3\n"
		"sync\n"
		"tlbwi\n"
		"sync\n"
	);
}

int _sifGetMSFLG() {
	u32 msflg;
	for (;;) {
        msflg = SBUS_MSFLG;
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");

        if (msflg == SBUS_MSFLG) return msflg;
	}
}

int _sifGetSMFLG() {
	u32 smflg;
	for (;;) {
        smflg = SBUS_SMFLG;
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");

        if (smflg == SBUS_SMFLG) return smflg;
	}
}

int _SifSetReg(int reg, u32 val) {
	__printf("%s: reg=%d; val=%x\n", __FUNCTION__, reg, val);

	if (reg == 1) {
		*(u32*)0xB000F200 = val;
		return *(u32*)0xB000F200;
	} else
	if (reg == 3) {
		SBUS_MSFLG = val;
		return _sifGetMSFLG();
	} else
	if (reg == 4) {
		SBUS_SMFLG = val;
		return _sifGetSMFLG();
	} else
	if (reg >= 0) {
		return 0; 
	}

	reg&= 0x7FFFFFFF;
	if (reg >= 32) return 0;

	sifRegs[reg] = val;
	return val;
}

int _SifGetReg(int reg) {
	//__printf("%s: reg=%x\n", __FUNCTION__, reg);

	if (reg == 1) {
		return *(u32*)0xB000F200;
	} else
	if (reg == 2) {
		return *(u32*)0xB000F210;
	} else
	if (reg == 3) {
		return _sifGetMSFLG();
	} else
	if (reg == 4) {
		return _sifGetSMFLG();
	} else
	if (reg >= 0) {
		return 0; 
	}

	reg&= 0x7FFFFFFF;
	if (reg >= 32) return 0;

    //__printf("ret=%x\n", sifRegs[reg]);
	return sifRegs[reg];
}
