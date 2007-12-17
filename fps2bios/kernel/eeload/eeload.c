
#include <tamtypes.h>
#include <stdio.h>

#include "eeload.h"
#include "eeinit.h"
#include "eedebug.h"


void _start() __attribute__((noreturn));
void _start() {
	void (*entry)();

	__puts("EELOAD start\n");

	__asm__ ( "la $sp, 0x80070000" );

	memorySize = 32*1024*1024;
	machineType = 0;

	__printf("TlbInit\n");
	TlbInit();
	__printf("InitPgifHandler\n");
	InitPgifHandler();
	__printf("Initialize\n");
	Initialize();
//	__load_module_EENULL();

	__asm__ (
		"li   $26, 0x70030C10\n"
		"mtc0 $26, $12\n"

		"li   $26, 0x73003\n"
		"mtc0 $26, $16\n"
	);

//	ee_deci2_manager_init();
	SifDmaInit();
//	__init_unk(0x81FE0);
	SetVSyncFlag(0, 0);

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

void saveContext() {
	__asm__ (
		"lui   $26, %hi(SavedRegs)\n"
		"sq    $2,  %lo(SavedRegs+0x000)($26)\n"
		"sq    $3,  %lo(SavedRegs+0x010)($26)\n"
		"sq    $4,  %lo(SavedRegs+0x020)($26)\n"
		"sq    $5,  %lo(SavedRegs+0x030)($26)\n"
		"sq    $6,  %lo(SavedRegs+0x040)($26)\n"
		"sq    $7,  %lo(SavedRegs+0x050)($26)\n"
		"sq    $8,  %lo(SavedRegs+0x060)($26)\n"
		"sq    $9,  %lo(SavedRegs+0x070)($26)\n"
		"sq    $10, %lo(SavedRegs+0x080)($26)\n"
		"sq    $11, %lo(SavedRegs+0x090)($26)\n"
		"sq    $12, %lo(SavedRegs+0x0A0)($26)\n"
		"sq    $13, %lo(SavedRegs+0x0B0)($26)\n"
		"sq    $14, %lo(SavedRegs+0x0C0)($26)\n"
		"sq    $15, %lo(SavedRegs+0x0D0)($26)\n"
		"sq    $16, %lo(SavedRegs+0x0E0)($26)\n"
		"sq    $17, %lo(SavedRegs+0x0F0)($26)\n"
		"sq    $18, %lo(SavedRegs+0x100)($26)\n"
		"sq    $19, %lo(SavedRegs+0x110)($26)\n"
		"sq    $20, %lo(SavedRegs+0x120)($26)\n"
		"sq    $21, %lo(SavedRegs+0x130)($26)\n"
		"sq    $22, %lo(SavedRegs+0x140)($26)\n"
		"sq    $23, %lo(SavedRegs+0x150)($26)\n"
		"sq    $24, %lo(SavedRegs+0x160)($26)\n"
		"sq    $25, %lo(SavedRegs+0x170)($26)\n"
		"sq    $gp, %lo(SavedRegs+0x180)($26)\n"
		"sq    $fp, %lo(SavedRegs+0x190)($26)\n"

		"mfhi  $27\n"
		"sd    $27, %lo(SavedRegs+0x1A0)($26)\n"
		"mfhi1 $27\n"
		"sd    $27, %lo(SavedRegs+0x1A8)($26)\n"

		"mflo  $27\n"
		"sd    $27, %lo(SavedRegs+0x1B0)($26)\n"
		"mflo1 $27\n"
		"sd    $27, %lo(SavedRegs+0x1B8)($26)\n"

		"mfsa  $27\n"
		"sw    $27, %lo(SavedRegs+0x1C0)($26)\n"
	);
}

void restoreContext() {
	__asm__ (
		"lui   $26, %hi(SavedRegs)\n"
		"lq    $2,  %lo(SavedRegs+0x000)($26)\n"
		"lq    $3,  %lo(SavedRegs+0x010)($26)\n"
		"lq    $4,  %lo(SavedRegs+0x020)($26)\n"
		"lq    $5,  %lo(SavedRegs+0x030)($26)\n"
		"lq    $6,  %lo(SavedRegs+0x040)($26)\n"
		"lq    $7,  %lo(SavedRegs+0x050)($26)\n"
		"lq    $8,  %lo(SavedRegs+0x060)($26)\n"
		"lq    $9,  %lo(SavedRegs+0x070)($26)\n"
		"lq    $10, %lo(SavedRegs+0x080)($26)\n"
		"lq    $11, %lo(SavedRegs+0x090)($26)\n"
		"lq    $12, %lo(SavedRegs+0x0A0)($26)\n"
		"lq    $13, %lo(SavedRegs+0x0B0)($26)\n"
		"lq    $14, %lo(SavedRegs+0x0C0)($26)\n"
		"lq    $15, %lo(SavedRegs+0x0D0)($26)\n"
		"lq    $16, %lo(SavedRegs+0x0E0)($26)\n"
		"lq    $17, %lo(SavedRegs+0x0F0)($26)\n"
		"lq    $18, %lo(SavedRegs+0x100)($26)\n"
		"lq    $19, %lo(SavedRegs+0x110)($26)\n"
		"lq    $20, %lo(SavedRegs+0x120)($26)\n"
		"lq    $21, %lo(SavedRegs+0x130)($26)\n"
		"lq    $22, %lo(SavedRegs+0x140)($26)\n"
		"lq    $23, %lo(SavedRegs+0x150)($26)\n"
		"lq    $24, %lo(SavedRegs+0x160)($26)\n"
		"lq    $25, %lo(SavedRegs+0x170)($26)\n"
		"lq    $gp, %lo(SavedRegs+0x180)($26)\n"
		"lq    $fp, %lo(SavedRegs+0x190)($26)\n"

		"ld    $2,  %lo(SavedRegs+0x1A0)($26)\n"
		"mthi  $2\n"
		"ld    $2,  %lo(SavedRegs+0x1A8)($26)\n"
		"mthi1 $2\n"

		"ld    $2,  %lo(SavedRegs+0x1B0)($26)\n"
		"mtlo  $2\n"
		"ld    $2,  %lo(SavedRegs+0x1B8)($26)\n"
		"mtlo1 $2\n"

		"lw    $2,  %lo(SavedRegs+0x1C0)($26)\n"
		"mtsa  $2\n"

		"lq    $2,  %lo(SavedRegs+0x000)($26)\n"
	);
}

void saveThreadContext() {
	__asm__ (
		"lui   $26, %hi(SavedSP)\n"
		"lq    $26, %lo(SavedSP)($26)\n"
	);
	__asm__ (
		"addiu $26, %0\n"
		: : "i"(-sizeof(struct threadCtx))
	);
	__asm__ (
		".set noat\n"
		"lui   $1,  %hi(SavedAT)\n"
		"lq    $1,  %lo(SavedAT)($1)\n"
		"sq    $1,  0x000($26)\n"
		"sq    $2,  0x010($26)\n"
		"sq    $3,  0x020($26)\n"
		"sq    $4,  0x030($26)\n"
		"sq    $5,  0x040($26)\n"
		"sq    $6,  0x050($26)\n"
		"sq    $7,  0x060($26)\n"
		"sq    $8,  0x070($26)\n"
		"sq    $9,  0x080($26)\n"
		"sq    $10, 0x090($26)\n"
		"sq    $11, 0x0A0($26)\n"
		"sq    $12, 0x0B0($26)\n"
		"sq    $13, 0x0C0($26)\n"
		"sq    $14, 0x0D0($26)\n"
		"sq    $15, 0x0E0($26)\n"
		"sq    $16, 0x0F0($26)\n"
		"sq    $17, 0x100($26)\n"
		"sq    $18, 0x110($26)\n"
		"sq    $19, 0x120($26)\n"
		"sq    $20, 0x130($26)\n"
		"sq    $21, 0x140($26)\n"
		"sq    $22, 0x150($26)\n"
		"sq    $23, 0x160($26)\n"
		"sq    $24, 0x170($26)\n"
		"sq    $25, 0x180($26)\n"
		"sq    $gp, 0x190($26)\n"
		"lui   $1,  %hi(SavedSP)\n"
		"lq    $sp, %lo(SavedSP)($1)\n"
		"sq    $sp, 0x1A0($26)\n"
		"sq    $fp, 0x1B0($26)\n"
		"lui   $1,  %hi(SavedRA)\n"
		"lq    $ra, %lo(SavedRA)($1)\n"
		"sq    $ra, 0x1C0($26)\n"

		"mfhi  $1\n"
		"sd    $1, 0x1D0($26)\n"
		"mfhi1 $1\n"
		"sd    $1, 0x1D8($26)\n"

		"mflo  $1\n"
		"sd    $1, 0x1E0($26)\n"
		"mflo1 $1\n"
		"sd    $1, 0x1E8($26)\n"

		"mfsa  $1\n"
		"sw    $1, 0x1F0($26)\n"

		"cfc1  $1, $31\n"
		"sw    $1, 0x1F4($26)\n"

//		"cfc1  $1, $31\n"
//		"sw    $1, 0x1F8($26)\n"

		"swc1  $0,  0x200($26)\n"
		"swc1  $1,  0x204($26)\n"
		"swc1  $2,  0x208($26)\n"
		"swc1  $3,  0x20C($26)\n"
		"swc1  $4,  0x210($26)\n"
		"swc1  $5,  0x214($26)\n"
		"swc1  $6,  0x218($26)\n"
		"swc1  $7,  0x21C($26)\n"
		"swc1  $8,  0x220($26)\n"
		"swc1  $9,  0x224($26)\n"
		"swc1  $10, 0x228($26)\n"
		"swc1  $11, 0x22C($26)\n"
		"swc1  $12, 0x230($26)\n"
		"swc1  $13, 0x234($26)\n"
		"swc1  $14, 0x238($26)\n"
		"swc1  $15, 0x23C($26)\n"
		"swc1  $16, 0x240($26)\n"
		"swc1  $17, 0x244($26)\n"
		"swc1  $18, 0x248($26)\n"
		"swc1  $19, 0x24C($26)\n"
		"swc1  $20, 0x250($26)\n"
		"swc1  $21, 0x254($26)\n"
		"swc1  $22, 0x258($26)\n"
		"swc1  $23, 0x25C($26)\n"
		"swc1  $24, 0x260($26)\n"
		"swc1  $25, 0x264($26)\n"
		"swc1  $26, 0x268($26)\n"
		"swc1  $27, 0x26C($26)\n"
		"swc1  $28, 0x270($26)\n"
		"swc1  $29, 0x274($26)\n"
		"swc1  $30, 0x278($26)\n"
		"swc1  $31, 0x27C($26)\n"

		"lui   $1,  %hi(SavedSP)\n"
		"sq    $26, %lo(SavedSP)($1)\n"
		".set at\n"
	);
}

void restoreThreadContext() {
	__asm__ (
		".set noat\n"
		"lq    $1,  0x000($sp)\n"
		"lq    $2,  0x010($sp)\n"
		"lq    $3,  0x020($sp)\n"
		"lq    $4,  0x030($sp)\n"
		"lq    $5,  0x040($sp)\n"
		"lq    $6,  0x050($sp)\n"
		"lq    $7,  0x060($sp)\n"
		"lq    $8,  0x070($sp)\n"
		"lq    $9,  0x080($sp)\n"
		"lq    $10, 0x090($sp)\n"
		"lq    $11, 0x0A0($sp)\n"
		"lq    $12, 0x0B0($sp)\n"
		"lq    $13, 0x0C0($sp)\n"
		"lq    $14, 0x0D0($sp)\n"
		"lq    $15, 0x0E0($sp)\n"
		"lq    $16, 0x0F0($sp)\n"
		"lq    $17, 0x100($sp)\n"
		"lq    $18, 0x110($sp)\n"
		"lq    $19, 0x120($sp)\n"
		"lq    $20, 0x130($sp)\n"
		"lq    $21, 0x140($sp)\n"
		"lq    $22, 0x150($sp)\n"
		"lq    $23, 0x160($sp)\n"
		"lq    $24, 0x170($sp)\n"
		"lq    $gp, 0x180($sp)\n"
		"lq    $fp, 0x190($sp)\n"

		"ld    $2,  0x1A0($sp)\n"
		"mthi  $2\n"
		"ld    $2,  0x1A8($sp)\n"
		"mthi1 $2\n"

		"ld    $2,  0x1B0($sp)\n"
		"mtlo  $2\n"
		"ld    $2,  0x1B8($sp)\n"
		"mtlo1 $2\n"

		"lw    $2,  0x1C0($sp)\n"
		"mtsa  $2\n"

		"lq    $2,  0x000($sp)\n"
		".set at\n"
	);
}

struct ll* LL_unlink(struct ll *l){
	struct ll *p;

	if ((l==l->next) && (l==l->prev))
		return 0;
	p=l->prev;
	p->prev->next=p->next;
	p->next->prev=p->prev;
	return p;
}

struct ll* LL_rotate(struct ll *l){
	struct ll *p;

	if (p=LL_unlink(l)){
		p->prev=l;
		p->next=l->next;
		l->next->prev=p;
		l->next=p;
		return l->prev;
	}
	return NULL;
}

struct ll *LL_unlinkthis(struct ll *l){
	l->prev->next=l->next;
	l->next->prev=l->prev;
	return l->next;
}

void LL_add(struct ll *l, struct ll *new){
	new->prev=l;
	new->next=l->next;
	l->next->prev=new;
	l->next=new;
}

struct ll *_AddHandler() {
	struct ll *l;

	l = LL_unlink(&handler_ll_free);
	if (l == NULL) return NULL;

	_HandlersCount++;
	return l;
}

void _RemoveHandler(int n) {
	LL_add(&handler_ll_free, (struct ll*)&pgifhandlers_array[n]);
	_HandlersCount--;
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

////////////////////////////////////////////////////////////////////
//80001564		SYSCALL 000 RFU000_FullReset
//80001564		SYSCALL 003 RFU003 (also)
//80001564		SYSCALL 008 RFU008 (too)
////////////////////////////////////////////////////////////////////
void _RFU___() {
	u32 num;

	__asm__ ("move %0, $3" : "=r"(num) : );
	__printf("# Syscall: undefined (%d)\n", num);
}

int  __Exit() {
	__printf("%s\n", __FUNCTION__);
}

void _RFU005() {
	__asm__ (
		".set noat\n"

		"mfc0 $26, $12\n"
		"ori  $1,  $0, 0xFFE4\n"
		"and  $26, $1\n"
		"mtc0 $26, $12\n"
		"sync\n"

		"lw   $ra, excepRA\n"
		"lw   $sp, excepSP\n"
		"jr   $ra\n"

		".set at\n"
	);
}

void _LoadPS2Exe(const char *filename, int argc, char **argv) {
	__printf("%s\n", __FUNCTION__);
}

int  _ExecPS2(void *entry, void *gp, int argc, char **argv) {
	__printf("%s\n", __FUNCTION__);
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

int  __AddSbusIntcHandler(int cause, void (*handler)(int ca)) {
	if (cause >= 32) return -1;

	if (sbus_handlers[cause] != 0) return -1;

	sbus_handlers[cause] = handler;
	return cause;
}

int  _AddSbusIntcHandler(int cause, void (*handler)(int ca)) {
	if (cause < 16) return __AddSbusIntcHandler(cause, handler);

	return -1;
}

int  __RemoveSbusIntcHandler(int cause) {
	if (cause >= 32) return -1;

	sbus_handlers[cause] = 0;
	return cause;
}

int  _RemoveSbusIntcHandler(int cause) {
	if (cause < 16) return __RemoveSbusIntcHandler(cause);

	return -1;
}

int __Interrupt2Iop(int cause) {
	if (cause >= 32) {
		return -1;
	}

	SBUS_MSFLG = 1 << cause;

	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100;
	SBUS_F240 = 0x100; // eight times

	SBUS_F240 = 0x40100;

	return cause;
}

int _Interrupt2Iop(int cause) {
	if (cause < 16) {
		return _Interrupt2Iop(cause);
	}
	return -1;
}

void *_SetVTLBRefillHandler(int cause, void (*handler)()) {
	if ((cause-1) >= 3) return 0;

	VCRTable[cause] = handler;

	return handler;
}

void *_SetVCommonHandler(int cause, void (*handler)()) {
	if ((cause-4) >= 10) return 0;

	VCRTable[cause] = handler;

	return handler;
}

void *_SetVInterruptHandler(int cause, void (*handler)()) {
	if (cause >= 8) return 0;

	VIntTable[cause] = handler;
	return handler;
}

////////////////////////////////////////////////////////////////////
//800018B0
////////////////////////////////////////////////////////////////////
int __AddIntcHandler(int cause, int (*handler)(int), int next, void *arg, int flag) {
	struct IDhandl *idh;

	if ((flag != 0) && (cause == INTC_SBUS)) return -1;
	if (cause >= 15) return -1;

	idh = (struct IDhandl *)_AddHandler();
	if (idh == 0) return -1;

	idh->handler = handler;
	__asm__ ("sw $gp, %0\n" : "=m"(idh->gp) : );
	idh->arg     = arg;
	idh->flag    = flag;

	if (next==-1)				//register_last
		LL_add(&ihandlers_last[cause*12], (struct ll*)idh);
	else if (next==0)			//register_first
		LL_add(&ihandlers_first[cause*12], (struct ll*)idh);
	else{
		if (next>128) return -1;
		if (pgifhandlers_array[next].flag==3)	return -1;
		LL_add((struct ll*)&pgifhandlers_array[next], (struct ll*)idh);
	}

	intcs_array[cause].count++;
	return (((u32)idh-(u32)&pgifhandlers_array) * 0xAAAAAAAB) / 8;
}

int  _AddIntcHandler(int cause, int (*handler)(int), int next, void *arg) {
	__AddIntcHandler(cause, handler, next, arg, 2);
}

int  _RemoveIntcHandler(int cause, int hid) {
	if (hid >= 128) return -1;

	if (pgifhandlers_array[hid].flag == 3) return -1;
	pgifhandlers_array[hid].flag    = 3;
	pgifhandlers_array[hid].handler = 0;

	LL_unlinkthis((struct ll*)&pgifhandlers_array[hid]);
	_RemoveHandler(hid);

	intcs_array[cause].count--;
}

int __AddDmacHandler(int cause, int (*handler)(int), int next, void *arg, int flag) {
	struct IDhandl *idh;
	register int temp;

	if (cause >= 16) return -1;

	idh = (struct IDhandl *)_AddHandler();
	if (idh == 0) return -1;

	idh->handler = handler;
	__asm__ ("sw $gp, %0\n" : "=m"(idh->gp) : );
	idh->arg     = arg;
	idh->flag    = flag;

	if (next==-1)				//register_last
		LL_add(&dhandlers_last[cause*12], (struct ll*)idh);
	else if (next==0)			//register_first
		LL_add(&dhandlers_first[cause*12], (struct ll*)idh);
	else{
		if (next>128) return -1;
		if (pgifhandlers_array[next].flag==3)	return -1;
		LL_add((struct ll*)&pgifhandlers_array[next], (struct ll*)idh);
	}

	dmacs_array[cause].count++;
	return (((u32)idh-(u32)&pgifhandlers_array) * 0xAAAAAAAB) / 8;
}

int  _AddDmacHandler(int cause, int (*handler)(int), int next, void *arg) {
	__AddDmacHandler(cause, handler, next, arg, 2);
}

int  _RemoveDmacHandler(int cause, int hid) {
	if (hid >= 128) return -1;

	if (pgifhandlers_array[hid].flag == 3) return -1;
	pgifhandlers_array[hid].flag    = 3;
	pgifhandlers_array[hid].handler = 0;

	LL_unlinkthis((struct ll*)&pgifhandlers_array[hid]);
	_RemoveHandler(hid);

	dmacs_array[cause].count--;
}

int  __EnableIntc(int ch) {
	int intbit;

	intbit = 0x1 << ch;
	if ((INTC_MASK & intbit) != 0) return 0;

	INTC_MASK = intbit;

	return 1;
}

int  __DisableIntc(int ch) {
	int intbit;

	intbit = 0x1 << ch;
	if ((INTC_MASK & intbit) == 0) return 0;

	INTC_MASK = intbit;

	return 1;
}

int  __EnableDmac(int ch) {
	int dmabit;

	dmabit = 0x10000 << ch;
	if ((DMAC_STAT & dmabit) != 0) return 0;

	DMAC_STAT = dmabit;

	return 1;
}

int  __DisableDmac(int ch) {
	int dmabit;

	dmabit = 0x10000 << ch;
	if ((DMAC_STAT & dmabit) == 0) return 0;

	DMAC_STAT = dmabit;

	return 1;
}

void __SetAlarm() {
	__printf("%s\n", __FUNCTION__);
}

void __ReleaseAlarm() {
	__printf("%s\n", __FUNCTION__);
}

int  _CreateThread(struct ThreadParam *param) {
	struct TCB *th;
	struct threadCtx *thctx;
	int index;
	int *ptr;

	th = (struct TCB *)LL_unlink((struct ll*)&thread_ll_free);
	if (th == NULL) {
		__printf("%s: failed to get free thread\n", __FUNCTION__);
		return -1;
	}

	threads_count++;
	index=((th-threads_array) * 0x286BCA1B)/4;

	th->entry           = param->entry;
	th->stack_res       = param->stack + param->stackSize - STACK_RES;
	th->status          = THS_DORMANT;
	th->gpReg           = param->gpReg;
	th->initPriority    = param->initPriority;
	th->argstring       = 0;
	th->wakeupCount     = 0;
	th->semaId          = 0;
	th->stack           = param->stack;
	th->argc            = 0;
	th->entry_          = param->entry;
	th->heap_base       = threads_array[threadId].heap_base;
	th->stackSize       = param->stackSize;
	th->currentPriority = param->initPriority;
//	th->waitType        = 0;
	th->root            = threads_array[threadId].root;

	thctx = th->stack_res;
	thctx->gp = (u32)param->gpReg;
	thctx->sp = (u32)&thctx[1];
	thctx->fp = (u32)&thctx[1];
	thctx->ra = (u32)threads_array[threadId].root;

	return index;
}

int  _DeleteThread(int tid) {
/*	int ret;
	int tmp;

	saveContext();

	ret = _DeleteThread(tid);
	if (ret < 0) {
		__asm { mfc0 tmp, EPC }
		_sub_3940(tmp, SavedSP);

		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}
	} else {
		__asm {
			lw $sp, SavedSP
			sd ret, 0x20($sp)
		}
	}

	restoreContext();

	__asm {
		mfc0 $k0, SR
		ori  $k0, 0x13
		mtc0 $k0, SR
		sync
		eret
	}*/
}

int  _StartThread(int tid, void *arg) {
	__printf("%s\n", __FUNCTION__);
}

int  _ExitThread() {
	__printf("%s\n", __FUNCTION__);
}

int  _ExitDeleteThread() {
	__printf("%s\n", __FUNCTION__);
}

void _TerminateThread() {
	__printf("%s\n", __FUNCTION__);
}

void _iTerminateThread() {
	__printf("%s\n", __FUNCTION__);
}

void _DisableDispatchThread() {
	__printf("%s\n", __FUNCTION__);
}

void _EnableDispatchThread() {
	__printf("%s\n", __FUNCTION__);
}

int  _ChangeThreadPriority(int tid, int prio) {
	__printf("%s\n", __FUNCTION__);
}

int  _iChangeThreadPriority(int tid, int prio) {
	__printf("%s\n", __FUNCTION__);
}

void _RotateThreadReadyQueue() {
	__printf("%s\n", __FUNCTION__);
}

int  _iRotateThreadReadyQueue(int prio) {
	__printf("%s\n", __FUNCTION__);
}

void _ReleaseWaitThread() {
	__printf("%s\n", __FUNCTION__);
}

void _iReleaseWaitThread() {
	__printf("%s\n", __FUNCTION__);
}

int  _GetThreadId() {
	__printf("%s\n", __FUNCTION__);
}

int  _ReferThreadStatus(int tid, struct ThreadParam *info) {
	__printf("%s\n", __FUNCTION__);
}

int  _SleepThread() {
	__printf("%s\n", __FUNCTION__);
}

int  _WakeupThread(int tid) {
	__printf("%s\n", __FUNCTION__);
}

int  _iWakeupThread(int tid) {
	__printf("%s\n", __FUNCTION__);
}

int  _CancelWakeupThread(int tid) {
	__printf("%s\n", __FUNCTION__);
}

int  _SuspendThread(int thid) {
	__printf("%s\n", __FUNCTION__);
}

void _ResumeThread() {
	__printf("%s\n", __FUNCTION__);
}

int  _iResumeThread(int tid) {
	__printf("%s\n", __FUNCTION__);
}

void _JoinThread() {
	__printf("%s\n", __FUNCTION__);
}

////////////////////////////////////////////////////////////////////
// makes the args from argc & argstring; args is in bss of program
////////////////////////////////////////////////////////////////////
void _InitArgs(char *argstring, ARGS *args, int argc) {
	int i;
	char *p = args->args;

	args->argc = argc;
	for (i=0; i<argc; i++) {
		args->argv[i] = p;	//copy string pointer
		while (*argstring)	//copy the string itself
			*p++ = *argstring++;
		*p++ = *argstring++;	//copy the '\0'
	}
}

char *_InitializeMainThread(int gp, void *stack, int stack_size, 
						char *args, int root) {
	struct TCB *th;
	struct threadCtx *ctx;

	if ((int)stack == -1)
		stack = (void*)((GetMemorySize() - 4*1024) - stack_size);

	ctx = stack + stack_size;
	ctx-= STACK_RES/4;
/*	ctx->gp   = gp;			//+1C0
	ctx->ra   = root;		//+1F0
	ctx->fp   = &ctx->field_280;	//+1E0 <- &280
	ctx->sp   = &ctx->field_280;	//+1D0 <- &280
*/
	th = &threads_array[threadId];
	th->gpReg	= gp;
	th->stackSize   = stack_size;
	th->stack_res   = ctx;
	th->stack       = stack;
	th->root        = root;
	_InitArgs(th->argstring, args, th->argc);
	th->argstring   = args;

	return ctx;
}

void *_InitializeHeapArea(void *heap_base, int heap_size) {
	void *ret;

	if (heap_size < 0) {
		ret = threads_array[threadId].stack;
	} else {
		ret = heap_base + heap_size;
	}

	threads_array[threadId].heap_base = ret;
	return ret;
}

void *_EndOfHeap() {
	return threads_array[threadId].heap_base;
}

int  _CreateSema(struct SemaParam *sema) {
	struct kSema *crt=semas_last;

	if ((crt==NULL) || (sema->init_count<0))	return -1;

	crt->wait_prev	=&crt->wait_next;
	semas_count++;
	crt->count	=sema->init_count;
	crt->wait_next	=&crt->wait_next;
	semas_last	=crt->free;
	crt->max_count	=sema->max_count;
	crt->free	=NULL;
	crt->attr	=sema->attr;
	crt->wait_threads=0;
	crt->option	=sema->option;

	return (crt-semas_array);	//sizeof(kSema)==32
}

void _DeleteSema() {
	__printf("%s\n", __FUNCTION__);
}

void _SignalSema() {
	__printf("%s\n", __FUNCTION__);
}

int  _iSignalSema(int sid) {
	__printf("%s\n", __FUNCTION__);
}

int  _WaitSema(int sid) {
	__printf("%s\n", __FUNCTION__);
}

int  _PollSema(int sid) {
	__printf("%s\n", __FUNCTION__);
}

int  _ReferSemaStatus(int sid, struct SemaParam *sema) {
	__printf("%s\n", __FUNCTION__);
}

int  _RFU073(int sid) {
	__printf("%s\n", __FUNCTION__);
}

void _GetOsdConfigParam(int *result){
	*result= (*result & 0xFFFFFFFE) | (osdConfigParam & 1);
	*result= (*result & 0xFFFFFFF9) | (osdConfigParam & 6);
	*result= (*result & 0xFFFFFFF7) | (osdConfigParam & 8);
	*result= (*result & 0xFFFFFFEF) | (osdConfigParam & 0x10);
	*result=((*result & 0xFFFFE01F) | (osdConfigParam & 0x1FE0)) & 0xFFFF1FFF;
	((u16*)result)[1]=((u16*)&osdConfigParam)[1];
}

void _SetOsdConfigParam(int *param){
	osdConfigParam = (osdConfigParam & 0xFFFFFFFE) | (*param & 1);
	osdConfigParam = (osdConfigParam & 0xFFFFFFF9) | (*param & 6);
	osdConfigParam = (osdConfigParam & 0xFFFFFFF7) | (*param & 8);
	osdConfigParam = (osdConfigParam & 0xFFFFFFEF) | (*param & 0x10);
	osdConfigParam =((osdConfigParam & 0xFFFFE01F) | (*param & 0x1FE0)) & 0xFFFF1FFF;
	((u16*)&osdConfigParam)[1]=((u16*)param)[1];
}

void _GetGsHParam(int *p0, int *p1, int *p2, int *p3) {
	unsigned long _hvParam = (unsigned long)hvParam;

	*p0 = _hvParam >> 12;
	*p1 = _hvParam >> 24;
	*p2 = _hvParam >> 18;
	*p3 = _hvParam >> 28;
}

int  _GetGsVParam() {
	return hvParam & 0x3;
}

void _SetGsVParam(int VParam) {
	hvParam&= ~0x1;
	hvParam|= VParam & 0x1;
}

void _SetGsHParam() {
	__printf("%s\n", __FUNCTION__);
}

int  _CreateEventFlag() {
	__printf("%s\n", __FUNCTION__);
}

int  _DeleteEventFlag() {
	__printf("%s\n", __FUNCTION__);
}

void _SetEventFlag() {
	__printf("%s\n", __FUNCTION__);
}

void _iSetEventFlag() {
	__printf("%s\n", __FUNCTION__);
}

void _EnableIntcHandler() {
	__printf("%s\n", __FUNCTION__);
}

void _DisableIntcHandler() {
	__printf("%s\n", __FUNCTION__);
}

void _EnableDmacHandler() {
	__printf("%s\n", __FUNCTION__);
}

void _DisableDmacHandler() {
	__printf("%s\n", __FUNCTION__);
}

void _KSeg0() {
	__printf("%s\n", __FUNCTION__);
}

void _EnableCache() {
	__printf("%s\n", __FUNCTION__);
}

void _DisableCache() {
	__printf("%s\n", __FUNCTION__);
}

int  _GetCop0(int reg) {
	__printf("%s\n", __FUNCTION__);
}

void _FlushCache() {
	__printf("%s\n", __FUNCTION__);
}

void _105() {
	__printf("%s\n", __FUNCTION__);
}

void _CpuConfig() {
	__printf("%s\n", __FUNCTION__);
}

void _SifStopDma() {
	int chcr;

	D5_CHCR = 0;
	D5_QWC	= 0;
	chcr	= D5_CHCR;
}

void _SetCPUTimerHandler() {
	__printf("%s\n", __FUNCTION__);
}

void _SetCPUTimer() {
	__printf("%s\n", __FUNCTION__);
}

u64  _GsGetIMR() {
	return gsIMR;
}

void _GsPutIMR(u64 val) {
	GS_IMR = val;
	gsIMR = val;
}

void _SetPgifHandler() {
	__printf("%s\n", __FUNCTION__);
}

void _SetVSyncFlag(int flag0, int flag1) {
	__printf("%s\n", __FUNCTION__);
	VSyncFlag0 = flag0;
	VSyncFlag1 = flag1;
}

void _SetSYSCALL(int num, int address) {
	__printf("%s\n", __FUNCTION__);
}

void _print() {
	__printf("%s\n", __FUNCTION__);
}

void _SifDmaStat() {
	__printf("%s\n", __FUNCTION__);
}

void _SifSetDma(SifDmaTransfer_t *sdd, int len) {
	__printf("%s\n", __FUNCTION__);
}

void _SifSetDChain() {
	int chcr;

	D5_CHCR	= 0;
	D5_QWC	= 0;
	D5_CHCR	= 0x184;
	chcr	= D5_CHCR;
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
	__printf("%s: reg=%x\n", __FUNCTION__, reg);

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

void _ExecOSD() {
	__printf("%s\n", __FUNCTION__);
}

void _PSMode() {
	machineType|= 0x8000;
}

u32  _MachineType() {
	return machineType;
}

long _SetMemorySize(long size) {
	memorySize = size;
	return size;
}

long _GetMemorySize() {
	__printf("%s\n", __FUNCTION__);
	return memorySize;
}

static void _SetGsCrt2() {
	u32 tmp;

	tmp = *(int*)0x8001F344;
	if (tmp == 0x40 || tmp == 0x60 || tmp == 0x61) {
		*(char*)0xbf803218 = 0;
		*(char*)0xbf803218 = 2;
		return;
	}

	*(short*)0xbf801470 = 0;
	*(short*)0xbf801472 = 0;
	*(short*)0xbf801472 = 1;
}

////////////////////////////////////////////////////////////////////
// SYSCALL 002 SetGsCrt
////////////////////////////////////////////////////////////////////
void _SetGsCrt(short arg0, short arg1, short arg2) {
	u64 val, val2;
	u64 tmp;
	int count;

	if (arg1 == 0) {
		tmp = (hvParam >> 3) & 0x07;
		tmp ^= 0x02;
		if (tmp == 0) arg1 = 3;
		else arg1 = 2;
	}

	for (count=0x270f; count >= 0; count--) {
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\n");
	}

	*(int*)0x8001F344 = 0;

	if (arg1 == 2) {
		if (arg0 != 0) {
			val  = 0x740834504;
			val2 = 0x740814504;

			tmp = (hvParam & 0x1) << 25;
			val  |= tmp;
			val2 |= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5B61F06F040;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01801;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		} else {
			val = 0x740834504;
			val2 = 0x740814504;

			tmp = (hvParam & 0x2) << 35;
			val |= tmp;
			val2|= tmp;

			tmp = (hvParam & 0x1) << 25;
			val |= tmp;
			val2|= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5B61F06F040;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01802;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		}
		_SetGsCrt2(); return;
	}

	if (arg1 == 3) {
		if (arg0 != 0) {
			val = 0x740836504;
			val2 = 0x740816504;

			tmp = (hvParam & 0x1) << 25;
			val |= tmp;
			val2|= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5C21FC83030;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101401;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		} else {
			val = 0x740836504;
			val2 = 0x740816504;

			tmp = (hvParam & 0x2) << 35;
			val |= tmp;
			val2|= tmp;

			tmp = (hvParam & 0x1) << 25;
			val |= tmp;
			val2|= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5C21F683030;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101404;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		}
		_SetGsCrt2(); return;
	}

	if (arg1 == 0x72) {
		if (arg0 != 0) {
			val = 0x740814504;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5B61F06F040;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01801;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		} else {
			val = 0x740814504;

			val|= (hvParam & 0x2) << 35;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5B61F06F040;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01802;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		}
		return;
	}

	if (arg1 == 0x73) {
		if (arg0 != 0) {
			val = 0x740816504;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5C21FC83030;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101401;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		} else {
			val = 0x740816504;

			val|= (hvParam & 0x2) << 35;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5C21FC83030;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101404;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		}
		return;
	}
/*
	if ((arg1 - 26) >= 56) {
		_SetGsCrt3(arg0, arg1, arg2); return;
	}

	if (arg1 == 0x52) {
		_SetGsCrt3(arg0, arg1, arg2); return;
	}

	_SetGsCrt4(arg0, arg1, arg2);
*/
}


#if 0


////////////////////////////////////////////////////////////////////
//80000D80		SYSCALL 002 Exit
////////////////////////////////////////////////////////////////////
int Exit(){
	return _RFU004_Exit();
}

////////////////////////////////////////////////////////////////////
//80001A50
////////////////////////////////////////////////////////////////////
int sub_80001A50(int cause, int (*handler)(int), int next, void *arg) {
	_AddIntcHandler(cause, handler, next, arg, 0);
}

////////////////////////////////////////////////////////////////////
//80001C98
////////////////////////////////////////////////////////////////////
int sub_80001C98(int cause, int (*handler)(int), int next, void *arg) {
	_AddDmacHandler(cause, handler, next, arg, 0);
}


////////////////////////////////////////////////////////////////////
//80002050
////////////////////////////////////////////////////////////////////
int _SetAlarm(short a0, int a1, int a2) {
	int mode = _alarm_unk & 0x1;
	int i;

	i = 0;
	while (mode) {
		mode = (_alarm_unk >> i++) & 0x1;
		if (i >= 64) {
			return -1;
		}
	}

	_alarm_unk|= mode << i;	

	__asm { move i, $gp }
	dword_80016A80 = i;
	dword_80016A78 = a1;
	dword_80016A7C = a2;
	dword_80016A84 = RCNT3_MODE;
	i = RCNT3_MODE + a0;
	if (i < -1)
		i&= 0xffff;
	dword_80016A88 = i;

	if (RCNT3_MODE < i) {
		if (rcnt3Count <= 0) {
			
		}
	}
}

////////////////////////////////////////////////////////////////////
//80002650
////////////////////////////////////////////////////////////////////
void rcnt3Handler() {
	unsigned int i;
	char  *ptr;
	short *ptrs;
	long  *ptrl;
	int tmp;

	if (rcnt3Count < 2) {
		RCNT3_MODE) = 0x483;
	} else {
		RCNT3_MODE) = 0x583;
		RCNT3_TARGET) = rcnt3TargetTable[rcnt3TargetNum * 0x14];
	}

	for (;;) {
		if (--rcnt3Count >= 0) {
			ptr = /*rcnt3CountTable;*/0x80016F78;
			for (i=0; i<rcnt3Count; i++) {
				ptr[0] = ptr[1];
				ptr++;
			}
		}

		*(__int64*)80016A68&= ~(1 << *(char*)0x80016F78);
		tmp = rcnt3TargetTable[*(char*)0x80016F78 * 0x14];
		__asm { 
			move $s0, $gp 
			move $gp, tmp
		}
		ptrl = 0x80016A80 + *(char*)0x80016F78;
		ptrs = rcnt3TargetTable + *(char*)0x80016F78);
		_excepRet(*(int*)0x800124E4, *(ptr1-2), *(char*)0x80016F78, *ptrs, *(ptrl-1));

		__asm { $gp = $s0 }
		if (rcnt3Count >= 0) {
			if (*ptrs != rcnt3TargetTable[*(char*)0x80016F78 * 0x14]) {
				break;
			}
		} else break;
	}
}

////////////////////////////////////////////////////////////////////
//800027F8
////////////////////////////////////////////////////////////////////
void _InitRCNT3() {
	int i;

	rcnt3Count = 0;
	_alarm_unk = 0;
	for (i=0; i<0x40; i++) rcnt3TargetNum[i] = 0;

	_EnableINTC(INTC_TIM3);
}

////////////////////////////////////////////////////////////////////
//80002880		SYSCALL 005 RFU005
////////////////////////////////////////////////////////////////////
void RFU005() {
	int tmp;

	__asm { mfc0 tmp, SR } // Status Register
	tmp&= ~0x1B;
	__asm { mtc0 SR, tmp } // Status Register
	__asm { sync }

	tmp = 0x80016FD0;
	__asm { lw $sp, 0(tmp) }
	*(int*)0x80016FC0();
}

////////////////////////////////////////////////////////////////////
//80002C40		SYSCALL 099 GetCop0
////////////////////////////////////////////////////////////////////
int GetCop0(int reg) {
	return table_GetCop0[reg](reg);
}

////////////////////////////////////////////////////////////////////
//80002C58-80002D50	calls for table_GetCop0
////////////////////////////////////////////////////////////////////
int GetCop0_Index(int reg) 	{ __asm { mfc0 $v0, Index } }
int GetCop0_Random(int reg) 	{ __asm { mfc0 $v0, Random } }
int GetCop0_EntryLo0(int reg) 	{ __asm { mfc0 $v0, EntryLo0 } }
int GetCop0_EntryLo1(int reg) 	{ __asm { mfc0 $v0, EntryLo1 } }

int GetCop0_Context(int reg) 	{ __asm { mfc0 $v0, Context } }
int GetCop0_PageMask(int reg) 	{ __asm { mfc0 $v0, PageMask } }
int GetCop0_Wired(int reg) 	{ __asm { mfc0 $v0, Wired } }
int GetCop0_Reg7(int reg) 	{ return; }

int GetCop0_BadVAddr(int reg) 	{ __asm { mfc0 $v0, BadVAddr } }
int GetCop0_Count(int reg) 	{ __asm { mfc0 $v0, Count } }
int GetCop0_EntryHi(int reg) 	{ __asm { mfc0 $v0, EntryHi } }
int GetCop0_Compare(int reg) 	{ __asm { mfc0 $v0, Compare } }

int GetCop0_Status(int reg) 	{ __asm { mfc0 $v0, Status } }
int GetCop0_Cause(int reg) 	{ __asm { mfc0 $v0, Cause } }
int GetCop0_ExceptPC(int reg) 	{ __asm { mfc0 $v0, ExceptPC } }
int GetCop0_PRevID(int reg) 	{ __asm { mfc0 $v0, PRevID } }

int GetCop0_Config(int reg) 	{ __asm { mfc0 $v0, Config } }
int GetCop0_Reg17(int reg) 	{ return; }
int GetCop0_Reg18(int reg) 	{ return; }
int GetCop0_Reg19(int reg) 	{ return; }

int GetCop0_Reg20(int reg) 	{ return; }
int GetCop0_Reg21(int reg) 	{ return; }
int GetCop0_Reg22(int reg) 	{ return; }
int GetCop0_Reg23(int reg) 	{ __asm { mfc0 $v0, $23 } }

int GetCop0_DebugReg24(int reg)	{ __asm { mfc0 $v0, Debug } }
int GetCop0_Perf(int reg) 	{ __asm { mfc0 $v0, Perf } }
int GetCop0_Reg26(int reg) 	{ return; }
int GetCop0_Reg27(int reg) 	{ return; }

int GetCop0_TagLo(int reg) 	{ __asm { mfc0 $v0, TagLo } }
int GetCop0_TagHi(int reg) 	{ __asm { mfc0 $v0, TagHi } }
int GetCop0_ErrorPC(int reg) 	{ __asm { mfc0 $v0, ErrorPC } }
int GetCop0_Reg31(int reg) 	{ return; }

////////////////////////////////////////////////////////////////////
//80002D80
////////////////////////////////////////////////////////////////////
int SetCop0(int reg, int val) {
	return table_SetCop0[reg]();
}

////////////////////////////////////////////////////////////////////
//80002D98-80002F74	calls for table_SetCop0
////////////////////////////////////////////////////////////////////
int SetCop0_Index(int reg, int val) 	{ __asm { mfc0 $v0, Index; mtc0 val, Index; sync } }
int SetCop0_Random(int reg, int val) 	{ return -1; }
int SetCop0_EntryLo0(int reg, int val) 	{ __asm { mfc0 $v0, EntryLo0; mtc0 val, EntryLo0; sync } }
int SetCop0_EntryLo1(int reg, int val) 	{ __asm { mfc0 $v0, EntryLo1; mtc0 val, EntryLo1; sync } }

int SetCop0_Context(int reg, int val) 	{ __asm { mfc0 $v0, Context; mtc0 val, Context; sync } }
int SetCop0_PageMask(int reg, int val) 	{ __asm { mfc0 $v0, PageMask; mtc0 val, PageMask; sync } }
int SetCop0_Wired(int reg, int val) 	{ __asm { mfc0 $v0, Wired; mtc0 val, Wired; sync } }
int SetCop0_Reg7(int reg, int val) 	{ return -1; }

int SetCop0_BadVAddr(int reg, int val) 	{ return -1; }
int SetCop0_Count(int reg, int val) 	{ __asm { mfc0 $v0, Count; mtc0 val, Count; sync } }
int SetCop0_EntryHi(int reg, int val) 	{ __asm { mfc0 $v0, EntryHi; mtc0 val, EntryHi; sync } }
int SetCop0_Compare(int reg, int val) 	{ __asm { mfc0 $v0, Compare; mtc0 val, Compare; sync } }

int SetCop0_Status(int reg, int val) 	{ __asm { mfc0 $v0, Status; mtc0 val, Status; sync } }
int SetCop0_Cause(int reg, int val) 	{ return -1; }
int SetCop0_ExceptPC(int reg, int val) 	{ __asm { mfc0 $v0, ExceptPC; mtc0 val, ExceptPC; sync } }
int SetCop0_PRevID(int reg, int val) 	{ return -1; }

int SetCop0_Config(int reg, int val) 	{ __asm { mfc0 $v0, Config; sync; mtc0 val, Config; sync } }
int SetCop0_Reg17(int reg, int val) 	{ return -1; }
int SetCop0_Reg18(int reg, int val) 	{ return -1; }
int SetCop0_Reg19(int reg, int val) 	{ return -1; }

int SetCop0_Reg20(int reg, int val) 	{ return -1; }
int SetCop0_Reg21(int reg, int val) 	{ return -1; }
int SetCop0_Reg22(int reg, int val) 	{ return -1; }
int SetCop0_Reg23(int reg, int val) 	{ __asm { mfc0 $v0, $23; mtc0 val, $23; sync } }

int SetCop0_DebugReg24(int reg, int val){ __asm { mfc0 $v0, Debug; mtc0 val, Debug; sync } }
int SetCop0_Perf(int reg, int val) 	{ __asm { mfc0 $v0, Perf; mtc0 val, Perf; sync } }
int SetCop0_Reg26(int reg, int val) 	{ return -1; }
int SetCop0_Reg27(int reg, int val) 	{ return -1; }

int SetCop0_TagLo(int reg, int val) 	{ __asm { mfc0 $v0, TagLo; mtc0 val, TagLo; sync } }
int SetCop0_TagHi(int reg, int val) 	{ __asm { mfc0 $v0, TagHi; mtc0 val, TagHi; sync } }
int SetCop0_ErrorPC(int reg, int val) 	{ __asm { mfc0 $v0, ErrorPC; mtc0 val, ErrorPC; sync } }
int SetCop0_Reg31(int reg, int val) 	{ return -1; }

////////////////////////////////////////////////////////////////////
//80002F80		SYSCALL 007 ExecPS2
////////////////////////////////////////////////////////////////////
int ExecPS2(void *, void *, int, char **) {
	int ret;

	saveContext();

	ret = _ExecPS2();
	__asm {
		mtc0 ret, EPC
		lw $sp, SavedSP
		sd ret, 0x20($sp)
	}

	restoreContext();

	__asm {
		mfc0 $v0, SR
		ori  $v0, 0x13
		mtc0 $v0, SR
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//80002FC0		SYSCALL 033 DeleteThread
////////////////////////////////////////////////////////////////////
int DeleteThread(int tid) {
	int ret;
	int tmp;

	saveContext();

	ret = _DeleteThread(tid);
	if (ret < 0) {
		__asm { mfc0 tmp, EPC }
		_sub_3940(tmp, SavedSP);

		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}
	} else {
		__asm {
			lw $sp, SavedSP
			sd ret, 0x20($sp)
		}
	}

	restoreContext();

	__asm {
		mfc0 $k0, SR
		ori  $k0, 0x13
		mtc0 $k0, SR
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//80003040		SYSCALL 034 StartThread
////////////////////////////////////////////////////////////////////
int StartThread(int tid, void *arg) {
	int ret;
	int tmp;

	saveContext();

	ret = _StartThread(tid, arg);
	if (ret >= 0) {
		__asm { mfc0 tmp, EPC }
		_ThreadHandler(tmp, SavedSP);

		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}
	} else {
		__asm {
			lw $sp, SavedSP
			sd ret, 0x20($sp)
		}
	}

	restoreContext();

	__asm {
		mfc0 $k0, SR
		ori  $k0, 0x13
		mtc0 $k0, SR
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//800030C0		SYSCALL 035 ExitThread
////////////////////////////////////////////////////////////////////
int ExitThread() {
	int ret;

	saveContext();

	ret = _ExitThread();
	__asm {
		mtc0 EPC, ret
		move $sp, $v1
	}

	restoreContext();

	__asm {
		mfc0 $v0, SR
		ori  $v0, 0x13
		mtc0 SR, $v0
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//80003100		SYSCALL 036 ExitDeleteThread
////////////////////////////////////////////////////////////////////
int ExitDeleteThread() {
	int ret;

	saveContext();

	ret = _ExitDeleteThread();
	__asm {
		mtc0 ret, EPC
		move $sp, $v1
	}

	restoreContext();

	__asm {
		mfc0 $v0, SR
		ori  $v0, 0x13
		mtc0 SR, $v0
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//800032C0		SYSCALL 052 SleepThread
////////////////////////////////////////////////////////////////////
int SleepThread() {
	int ret;

	__asm {
		lw $k0, threadId
		move $v0, $k0
	} 
	saveContext();

	ret = _SleepThread();
	if (ret <= 0) {
		int epc;

		__asm {
			mfc0 epc, EPC
		}
		_ChangeThread(epc, SavedSP, 1);

		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}

		restoreContext();
	} else {
		__asm {
			lw $sp, SavedSP
			sd $v0, 0x20($sp)
		}
		restoreContext();
	}

	__asm {
		mfc0 $v0, SR
		ori  $v0, 0x13
		mtc0 SR, $v0
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//80003340		SYSCALL 051 WakeupThread
////////////////////////////////////////////////////////////////////
int WakeupThread(int tid){
	register int ret, tmp;

	ret = tid;	//hm
	saveContext();
	ret = iWakeupThread(tid);
	if (ret>=0){
		__asm { mfc0 tmp, EPC }
		_ThreadHandler(tmp, SavedSP);

		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}
	}else {
		__asm { lw $sp, SavedSP }
		((struct stackregs*)SavedSP)->v0[0] = ret;	//useless
	}
	restoreContext();
	
	__asm {
		mfc0 $k0, SR
		ori  $k0, 0x13
		mtc0 $k0, SR
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//80003440		SYSCALL 068 WaitSema
////////////////////////////////////////////////////////////////////
int WaitSema(int sid) {
	int tmp;

	__asm { move $v0, sid }
	saveContext();

	ret = iWaitSema(sid);
	if (ret == -2) {
		__asm { mfc0 tmp, EPC }
		_ChangeThread(tmp, SavedSP, 2);
		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}
	} else {
		__asm { lw $sp, SavedSP }
	}
	restoreContext();

	__asm {
		mfc0 $k0, SR
		ori  $k0, 0x13
		mtc0 $k0, SR
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//800035C0		SYSCALL 041 ChangeThreadPriority
////////////////////////////////////////////////////////////////////
int ChangeThreadPriority(int tid, int prio){
	register int ret, tmp;

	saveContext();
	ret = iChangeThreadPriority(tid, prio);
	((struct stackregs*)(SavedSP))->v0[0] = ret;
	if (ret>=0){
		__asm { mfc0 tmp, EPC }
		_ThreadHandler(tmp, SavedSP);

		__asm {
			mtc0 $v0, EPC
			sync
			move $sp, $v1
		}
	}else {
		__asm { lw $sp, SavedSP }
		((struct stackregs*)SavedSP)->v0[0] = ret;	//useless
	}
	restoreContext();
	
	__asm {
		mfc0 $k0, SR
		ori  $k0, 0x13
		mtc0 $k0, SR
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//8000363C
////////////////////////////////////////////////////////////////////
void __ThreadHandler() {
	int epc;

	saveContext();
	__asm { mfc0 epc, EPC }
	_ThreadHandler(epc, SavedSP);
	__asm {
		mtc0 EPC, $v0
		sync
		move $sp, $v1
	}

	restoreContext();

	__asm {
		mfc0 $v0, SR
		ori  $v0, 0x13
		mtc0 SR, $v0
		sync
		eret
	}
}

////////////////////////////////////////////////////////////////////
//80003940
////////////////////////////////////////////////////////////////////
__int128 _ThreadHandler(unsigned long epc, unsigned long stack) {
	register int tid;

	threads_array[threadId].entry		=epc;
	threads_array[threadId].status		=THS_READY;
	threads_array[threadId].stacksave	=stack;

	for ( ; threadPrio < 129; threadPrio++)
		if ((thread_ll_priorities[threadPrio]->next !=
		     thread_ll_priorities[threadPrio]) ||
		    (thread_ll_priorities[threadPrio]->prev !=
		     thread_ll_priorities[threadPrio])){
			tid=threadId=(( thread_ll_priorities[threadPrio]->prev -
					threads_array)*0x286BCA1B)>>2;
			break;
		}

	if (threadPrio>=129){
		__printf("# <Thread> No active threads\n");
		Exit(1);
		tid=0;
	}

	threads_array[tid].status=THS_RUN;

	if (threads_array[tid].waitType){
		threads_array[tid].waitType=0;
		((struct stackregs*)threads_array[tid].stacksave)->v0[0]=-1;
	}

	__asm{
		mov $v0, threads_array[tid].entry
		mov $v1, threads_array[tid].stacksave
	}
}

////////////////////////////////////////////////////////////////////
//80003A78
////////////////////////////////////////////////////////////////////
__int128 _ChangeThread(unsigned long entry, unsigned long stack_res, int waitType) {
	struct TCB *th;
	struct ll *l, *p;
	int prio;

	th = &threads_array[threadId];
	th->status = THS_WAIT;
	th->waitType = waitType;
	th->entry = entry;
	th->stack_res = stack_res;

	prio = threadPrio;
	for (l = &thread_ll_priorities[prio]; ; l++, prio++) {
		if (prio >= 129) {
			__printf("# <Thread> No active threads\n");
			Exit(1); l = 0; break;
		}

		if (l->next != l) { p = l->next; break; }
		if (l->prev == l) continue;
		p = l->prev; break;
	}

	if (l) {
		threadPrio = prio;
		threadId = ((p - threads_array) * 0x286BCA1B) / 4;
	}

	th = &threads_array[threadId];
	th->status = THS_RUN;
	if (th->waitType) {
		th->waitType = 0;
		((__int64*)th->stack_res)[4] = -1;
	}

	__asm {
		move $v0, th->entry
		move $v1, th->stack_res
	}
//	return ;
}

////////////////////////////////////////////////////////////////////
//80003C50		SYSCALL 032 CreateThread
////////////////////////////////////////////////////////////////////
int CreateThread(struct ThreadParam *param){
	struct TCB *th;
	int index;
	int *ptr;

	th=LL_unlink(&thread_ll_free);
	if (th==NULL)
		return -1;
	}

	threads_count++;//eg.0x4C * 0x286BCA1B / 4 == 1 :)
	index=((th-threads_array) * 0x286BCA1B)/4;

	th->entry           = param->entry;
	th->stack_res       = param->stack + param->stackSize - STACK_RES;
	th->status          = THS_DORMANT;
	th->gpReg           = param->gpReg;
	th->initPriority    = param->initPriority;	//		short
	th->argstring       = 0;
	th->wakeupCount     = 0;
	th->semaId          = 0;
	th->stack           = param->stack;
	th->argc            = 0;
	th->entry_          = param->entry;
	th->heap_base       = threads_array[threadId].heap_base;
	th->stackSize       = param->stackSize;
	th->currentPriority = param->initPriority;	//		short
	th->waitType        = 0;
	th->root            = threads_array[threadId].root;

	ptr = param->stack + param->stackSize - STACK_RES;
	ptr->gp	= param->gpReg;			//+1C0
	ptr->sp	= &ptr->field_280;		//+1D0 <- &280
	ptr->fp	= &ptr->field_280;		//+1E0 <- &280
	ptr->ra	= threads_array[threadId].root;	//+1F0

	return index;
}

////////////////////////////////////////////////////////////////////
//80003F00
////////////////////////////////////////////////////////////////////
int _DeleteThread(int tid){
	if ((tid>=256) || (tid==threadId) || (threads_array[tid].status!=THS_DORMANT))
		return -1;

	releaseTCB(tid);
	return tid;
}

////////////////////////////////////////////////////////////////////
//80003F70
////////////////////////////////////////////////////////////////////
int _StartThread(int tid, void *arg){
	if ((tid>=256) || (tid==threadId) || (threads_array[tid].status!=THS_DORMANT))
		return -1;

	threads_array[tid].argstring	             = arg;
	((int*)threads_array[tid].stack_res)[0x10] = arg;  //a0
	thread_2_ready(tid);
	return tid;
}

////////////////////////////////////////////////////////////////////
//80004288		SYSCALL 042 iChangeThreadPriority
////////////////////////////////////////////////////////////////////
int iChangeThreadPriority(int tid, int prio) {
	short oldPrio;

	if ((tid >= 256) || (prio < 0) || (prio >= 128)) return -1;

	if (tid == 0) tid = threadId;
	if (threads_array[tid].status == 0) return -1;
	if ((0 < (threads_array[tid].status ^ 0x10)) == 0) return -1;

	oldPrio = threads_array[tid].currentPriority;
	if ((tid != threadId) && (threads_array[tid].status != THS_READY)) {
		threads_array[tid].currentPriority = prio;
		return oldPrio;
	}

	if (threadPrio < prio) threadStatus = 1;

	unsetTCB(tid);
	threads_array[tid].currentPriority = prio;
	thread_2_ready(tid);

	return oldPrio;
}

////////////////////////////////////////////////////////////////////
//80004388		SYSCALL 044 iRotateThreadReadyQueue
////////////////////////////////////////////////////////////////////
int iRotateThreadReadyQueue(int prio){
	if (prio >= 128) return -1;

	LL_rotate(&thread_ll_priorities[prio])
	return prio;
}

////////////////////////////////////////////////////////////////////
//80004548		SYSCALL 047 GetThreadId
////////////////////////////////////////////////////////////////////
int GetThreadId() {
	return threadId;
}

////////////////////////////////////////////////////////////////////
//80004558		SYSCALL 048 ReferThreadStatus
////////////////////////////////////////////////////////////////////
int ReferThreadStatus(int tid, ThreadParam *info) {
	if (tid >= 256) return -1;
	if (tid == 0) tid = threadId;

	if (info != NULL) {
		info->entry           = threads_array[tid].entry;
		info->status          = threads_array[tid].status;
		info->stack           = threads_array[tid].stack;
		info->stack_size      = threads_array[tid].stack_size;
		info->gpReg           = threads_array[tid].gpReg;
		info->initPriority    = threads_array[tid].initPriority;
		info->currentPriority = threads_array[tid].currentPriority;
		info->attr            = threads_array[tid].attr;
		info->waitType        = threads_array[tid].waitType;
		info->option          = threads_array[tid].option;
		info->waitId          = threads_array[tid].semaId;
		info->wakeupCount     = threads_array[tid].wakeupCount;
	}

	return threads_array[tid].status;
}

////////////////////////////////////////////////////////////////////
//80004658
////////////////////////////////////////////////////////////////////
int _SleepThread() {
	if (theads_array[threadId].wakeupCount <= 0) {
		unsetTCB(threadId);
		return -1;
	}

	theads_array[threadId].wakeupCount--;
	return threadId;
}

////////////////////////////////////////////////////////////////////
//800046B0		SYSCALL 052 iWakeupThread
////////////////////////////////////////////////////////////////////
int iWakeupThread(int tid){
	register int prio;

	if (tid>=256) return -1;

	if (tid==0) tid = threadId;

	switch (threads_array[tid].status){
	case THS_WAIT:
		if (threads_array[tid].waitType==1){
			prio=threadPrio;
			thread_2_ready(tid);
			if (threadPrio<prio)
				threadStatus=THS_RUN;
			threads_array[tid].waitType=0;
		}else
			threads_array[tid].wakeupCount++;
		break;
	case THS_READY:
	case THS_SUSPEND:
		threads_array[tid].wakeupCount++;
		break;
	case THS_WAITSUSPENDED:
		if (threads_array[tid].waitType==1){
			threads_array[tid].status=THS_SUSPEND;
			threads_array[tid].waitType=0;
		}else
			threads_array[tid].wakeupCount++;
		break;
	default:
		return -1;
	}
	
	return tid;
}

////////////////////////////////////////////////////////////////////
//80004808		SYSCALL 055 SuspendThread
//80004808		SYSCALL 056 iSuspendThread
////////////////////////////////////////////////////////////////////
int SuspendThread(int thid){
	if (thid<256){
		if ((threads_array[thid].status==THS_READY) || 
		    (threads_array[thid].status==THS_RUN)){
			unsetTCB(thid);
			threads_array[thid].status=THS_SUSPENDED;
			return thid;
		}
		if (threads_array[thid].status==THS_WAIT){
			threads_array[thid].status=THS_WAITSUSPENDED;
			return thid;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////
//800048C0		SYSCALL 058 iResumeThread
////////////////////////////////////////////////////////////////////
int iResumeThread(int tid){
    int tmp;
	if ((tid<256) && (threadId!=tid))
		if (threads_array[tid].status==THS_SUSPEND){
			tmp=threadPrio;
			thread_2_ready(tid);
			if (threadPrio < tmp)
				threadStatus=THS_RUN;
		}else
		if (threads_array[tid].status==THS_WAITSUSPEND)
			threads_array[tid].status=THS_WAIT;
		return tid;
	}
	return -1;
}

////////////////////////////////////////////////////////////////////
//80004970		SYSCALL 053 CancelWakeupThread
//80004970		SYSCALL 054 iCancelWakeupThread
////////////////////////////////////////////////////////////////////
int CancelWakeupThread(int tid){
	register int ret;

	if (tid>=256)	return -1;
	tid = tid ? tid : threadId;
	ret=threads_array[tid].wakeupCount;
	threads_array[tid].wakeupCount=0;
	return ret;
}

////////////////////////////////////////////////////////////////////
//800049B0		SYSCALL 080 RFU080_CreateEventFlag
////////////////////////////////////////////////////////////////////
int RFU080_CreateEventFlag() {
	return threads_count; // CreateEventFlag, why!?!
}


////////////////////////////////////////////////////////////////////
//80004A48		SYSCALL 073 RFU073 (iDeleteSema)
////////////////////////////////////////////////////////////////////
int RFU073(int sid){
    register thid, thprio;
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	semas_count--;
	while (semas_array[sid].wait_threads>0){
		thid=((LL_unlink(&semas_array[sid].wait_next)-threads_array)
			* 0x286BCA1B)/4;
		LL_unlinkthis(&threads_array[thid]);
		semas_array[sid].wait_threads--;
		if (threads_array[thid].status==THS_WAIT){
			thprio=threadPrio;
			thread_2_ready(thid);
			if (threadPrio<thprio)
				threadStatus=THS_RUN;
		}else
		if (threads_array[thid].status!=THS_WAITSUSPEND)
			threads_array[thid].status=THS_SUSPEND;
	}
	semas_array[sid].count	=-1;
	semas_array[sid].free	=semas_last;
	semas_last		=&semas_array[sid];
	return sid;
}

////////////////////////////////////////////////////////////////////
//80004BC8		SYSCALL 067 iSignalSema
////////////////////////////////////////////////////////////////////
int iSignalSema(int sid) {
    register int prio, thid;
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	if (semas_array[sid].wait_threads>0){
		thid=((LL_unlink(&semas_array[sid].wait_next)-threads_array)
			*0x286BCA1B)/4;

		LL_unlinkthis(&threads_array[thid]);

		semas_array[sid].wait_threads--;

		if (threads_array[thid].status==THS_WAIT){
			prio=threadPrio;
			thread_2_ready(thid);
			if (threadPrio < prio)
				threadStatus=THS_RUN;
			threads_array[thid].waitType=0;	//just a guess:P
		}else
		if (threads_array[thid].status==THS_WAITSUSPEND){
			threads_array[thid].status =THS_SUSPEND;
			threads_array[thid].waitType=0;
		}
	}else
		semas_array[sid].count++;
	return sid;
}

////////////////////////////////////////////////////////////////////
//80004CF8
////////////////////////////////////////////////////////////////////
int iWaitSema(int sid) {
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	if (semas_array[sid].count>0){
		semas_array[sid].count--;
		return sid;
	}

	semas_array[sid].wait_threads++;

	unsetTCB(threadId);
	LL_add(&semas_array[sid].wait_next, &threads_array[threadId]);
	threads_array[threadId].semaId=sid;

	return -2;
}

////////////////////////////////////////////////////////////////////
//80004DC8	SYSCALL 069 PollSema, 070 iPollSema
////////////////////////////////////////////////////////////////////
int PollSema(int sid) {
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<=0))	return -1;

	semas_array[sid].count--;

	return sid;
}

////////////////////////////////////////////////////////////////////
//80004E00	SYSCALL 071 ReferSemaStatus, 072 iReferSemaStatus
////////////////////////////////////////////////////////////////////
int ReferSemaStatus(int sid, struct SemaParam *sema){
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	sema->count		=semas_array[sid].count;
	sema->max_count		=semas_array[sid].max_count;
	sema->wait_threads	=semas_array[sid].wait_threads;
	sema->attr		=semas_array[sid].attr;
	sema->option		=semas_array[sid].option;

	return sid;
}

////////////////////////////////////////////////////////////////////
//80004E58		SYSCALL 081 RFU081_DeleteEventFlag
////////////////////////////////////////////////////////////////////
int RFU081_DeleteEventFlag() {
	return semas_count; // DeleteEventFlag, why!?!
}

////////////////////////////////////////////////////////////////////
//80004E68
////////////////////////////////////////////////////////////////////
int _SemasInit() {
	int i;

	for (i=0; i<256; i++) {
		semas_array[i].free = &semas_array[i+1];
		semas_array[i].count = -1;
		semas_array[i].wait_threads = 0;
		semas_array[i].wait_next = &semas_array[i].wait_next;
		semas_array[i].wait_prev = &semas_array[i].wait_next;
	}
	semas_array[255].free = 0;

	semas_last  = semas_array;
	semas_count = 0;

	return 256;
}

////////////////////////////////////////////////////////////////////
//80004FB0
////////////////////////////////////////////////////////////////////
void __load_module_EENULL() {
	int i;

	thread_ll_free.prev = thread_ll_free;
	thread_ll_free.next = thread_ll_free;

	for (i=0; i<128; i++) {
		thread_ll_priorities[i].prev = thread_ll_priorities[i];
		thread_ll_priorities[i].next = thread_ll_priorities[i];
	}

	threads_count = 0;
	threadId = 0;
	threadPrio = 0;

	for (i=0; i<256; i++) {
		threads_array[i].status = 0;
		LL_add(thread_ll_free, &threads_array[i]);
	}

	_SemasInit();

	threadStatus = 0;
	__load_module("EENULL", 0x81FC0, 0x81000, 0x80); 
}

////////////////////////////////////////////////////////////////////
//80004FB0
// makes the args from argc & argstring; args is in bss of program
////////////////////////////////////////////////////////////////////
void _InitArgs(char *argstring, ARGS args, int argc) {
	int i;
	char *p = args->args;

	args->argc = argc;
	for (i=0; i<argc; i++) {
		args->argv[i] = p;	//copy string pointer
		while (*argstring)	//copy the string itself
		*p++ = *argstring++;
		*p++ = *argstring++;	//copy the '\0'
	}
}

////////////////////////////////////////////////////////////////////
//80005198		SYSCALL 060 RFU060_InitializeMainThread
////////////////////////////////////////////////////////////////////
char *RFU060_InitializeMainThread(int gp, void *stack, int stack_size, 
						char *args, int root) {
	struct TCB *th;
	int *ptr;

	if ((int)stack == -1)
		stack = (GetMemorySize() - 4*1024) - stack_size;

	ptr = stack + stack_size;
	ptr-= STACK_RES/4;
	ptr->gp   = gp;			//+1C0
	ptr->ra   = root;		//+1F0
	ptr->fp   = &ptr->field_280;	//+1E0 <- &280
	ptr->sp   = &ptr->field_280;	//+1D0 <- &280

	th = &threads_array[threadId];
	th->gpReg	= gp;
	th->stack_size  = stack_size;
	th->stack_res   = ptr;
	th->stack       = stack;
	th->root        = root;
	_InitArgs(th->argstring, args, th->argc);
	th->argstring   = args;

	return ptr;
}

////////////////////////////////////////////////////////////////////
//800052A0		SYSCALL 061 RFU061_InitializeHeapArea
////////////////////////////////////////////////////////////////////
void* RFU061_InitializeHeapArea(void *heap_base, int heap_size) {
	void *ret;

	if (heap_size < 0) {
		ret = threads_array[threadId].stack;
	} else {
		ret = heap_base + heap_size;
	}

	threads_array[threadId].heap_base = ret;
	return ret;
}

////////////////////////////////////////////////////////////////////
//800052D8		SYSCALL 062 RFU062_EndOfHeap
////////////////////////////////////////////////////////////////////
void* RFU062_EndOfHeap() {
	return threads_array[threadId].heap_base;
}

////////////////////////////////////////////////////////////////////
//80005390
////////////////////////////////////////////////////////////////////
void __load_module(char *name, void (*entry)(void*), void *stack_res, int prio) {
	struct TCB *th;
	int index;
	int info[4], dinfo[4];
	int *ptr;

	th = LL_unlink(thread_ll_free);
	if (th) {
		threads_count++;
		index = ((th-threads_array) * 0x286BCA1B)/4;
	} else {
		index = -1;
	}

	threadId = index;
	th->wakeupCount     = 0;
	th->semaId          = 0;
	th->attr            = 0;
	th->stack_res       = stack_res;
	th->option          = 0;
	th->entry           = entry;
	th->gpReg           = 0;
	th->currentPriority = prio;
	th->status          = THS_DORMANT;
	th->waitType        = 0;
	th->entry_          = entry;
	th->argc            = 0;
	th->argstring       = 0;
	th->initPriority    = prio;

	thread_2_ready(index);

	if (_LoadDirInfo(0xBFC00000, 0xBFC10000, info) == NULL) {
		__printf("# panic ! dir not found\n");
		Exit(1);
	}

	if (_ReadDir(info, name, dinfo) == NULL) {
		__printf("# panic ! '%s' not found\n", name);
		Exit(1);
	}	

	ptr = dinfo[0];
	if (ptr[3] > 0) {
		int size = ptr[3];
		int i;
		int *src = dinfo[1];
		int *dst = entry;

		for (i=0; i<size; i+=4) {
			*dst++ = *src++;
		}
	}

	__CacheFlush1();
	__CacheFlush2();

	return index;
}

////////////////////////////////////////////////////////////////////
//80005900	
////////////////////////////////////////////////////////////////////
int  _RFU004_Exit(){
     char *bb="BootBrowser";
	return LoadPS2Exe("rom0:OSDSYS", 1, bb);
}

////////////////////////////////////////////////////////////////////
//80005938
////////////////////////////////////////////////////////////////////
void releaseTCB(int tid){
	threads_count--;
	threads_array[tid].status=0;
	LL_add(&thread_ll_free, &threads_array[tid]);
}

////////////////////////////////////////////////////////////////////
//80005978
////////////////////////////////////////////////////////////////////
void unsetTCB(int tid){
	if ((threads_array[tid].status) <= THS_READY)
		LL_unlinkthis(&threads_array[tid]);
}

////////////////////////////////////////////////////////////////////
//800059B8
////////////////////////////////////////////////////////////////////
void thread_2_ready(int tid){
	threads_array[tid].status=THS_READY;
	if (threads_array[tid].initPriority_ < threadPrio)
		threadPrio=(short)threads_array[tid].initPriority_;
	LL_add( &thread_ll_priorities[threads_array[tid].initPriority_],
		&threads_array[tid] );
}

////////////////////////////////////////////////////////////////////
//80005A58
////////////////////////////////////////////////////////////////////
struct ll* LL_unlink(struct ll *l){
	struct ll *p;

	if ((l==l->next) && (l==l->prev))
		return 0;
	p=l->prev;
	p->prev->next=p->next;
	p->next->prev=p->prev;
	return p;
}

////////////////////////////////////////////////////////////////////
//80005A98
////////////////////////////////////////////////////////////////////
struct ll* LL_rotate(struct ll *l){
	struct ll *p;

	if (p=LL_unlink(l)){
		p->prev=l;
		p->next=l->next;
		l->next->prev=p;
		l->next=p;
		return l->prev;
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////
//80005AE8
////////////////////////////////////////////////////////////////////
struct ll *LL_unlinkthis(struct ll *l){
	l->prev->next=l->next;
	l->next->prev=l->prev;
	return l->next;
}

////////////////////////////////////////////////////////////////////
//80005B08
////////////////////////////////////////////////////////////////////
void LL_add(struct ll *l, struct ll *new){
	new->prev=l;
	new->next=l->next;
	l->next->prev=new;
	l->next=new;
}

////////////////////////////////////////////////////////////////////
//800062A0		SYSCALL 120 (0x78) sceSifSetDChain
////////////////////////////////////////////////////////////////////
void sceSifSetDChain(){
}

////////////////////////////////////////////////////////////////////
//800062D8		SYSCALL 107 (0x6B) sceSifStopDma
////////////////////////////////////////////////////////////////////
void _sceSifStopDma(){
	int var_10;

	KSEG1_ADDR(D5_CHCR)	= 0;
	KSEG1_ADDR(D5_QWC)	= 0;
	var_10	= KSEG1_ADDR(D5_CHCR);	// ?!?
}

////////////////////////////////////////////////////////////////////
//80006370
////////////////////////////////////////////////////////////////////
void _sceSifDmaSend(void *src, void *dest, int size, int attr, int id){
	if (((++tagindex) & 0xFF) == 31){
		tagindex=0;
		transferscount++;
	}

	qwc=(size+15)/16;	//rount up

	t1=KSEG1_ADDR(&tadrptr[tagindex]);
	if (attr & SIF_DMA_TAG){
		t1->id_qwc = (id << 16) | qwc |
			((attr & SIF_DMA_INT_I) ? 0x80000000 : 0);	//IRQ
		t1->addr=src & 0x1FFFFFFF;
		t3=KSEG1_ADDR(src);
		qwc--;
	}else{
		if (qwc >= 8){	//two transfers
			tocopy=7;
			t1->id_qwc=0x30000008;	//(id)REF | (qwc)8;
			t1->addr=&etrastorage[tagindex] | 0x1FFFFFFF;
			t3=KSEG1_ADDR(&extrastorage[tagindex]);

			if (((++tagindex) & 0xff) == 31){
				tagindex=0;
				transferscount++;
			}

			t1=KSEG1_ADDR(&tadrptr[tagindex]);
			t1->id_qwc=(id << 16) | (qwc - 7) |
				((attr & SIF_DMA_INT_I) ? 0x80000000 : 0);//IRQ
			t1->addr=(src+112) | 0x1FFFFFFF;
		}else{
			tocopy=qwc;
			t1->id_qwc=(id << 16) | (qwc+1) |
				((attr & SIF_DMA_INT_I) ? 0x80000000 : 0);//IRQ
			t1->addr=&extrastorage[tagindex] & 0x1FFFFFFF;
			t3=KSEG1_ADDR(&extrastorage[tagindex]);
		}
		memcpy(t3+16, KSEG1_ADDR(src), tocopy*16);//inline with qwords
	}
	t3[1]=qwc * 4;
	t3[0]=(dest & 0x00FFFFFF) |
		((attr & SIF_DMA_INT_O) ? 0x40000000 : 0) |
		((attr & SIF_DMA_ERT)	? 0x80000000 : 0);
}

////////////////////////////////////////////////////////////////////
//800065C8
////////////////////////////////////////////////////////////////////
int _sceSifDmaCount(){
	register int count;

	count=((KSEG1_ADDR(D6_TADR)-&tadrptr) & 0x1FFFFFFF) >> 4;

	count=count>0? count-1:30;

	if (count == tagindex)
		return (KSEG1_ADDR(D6_QWC) ? 30 : 31);

	if (count < tagindex)
		return count + 30 - tagindex;

	return count-tagindex-1;
}

////////////////////////////////////////////////////////////////////
//80006650
////////////////////////////////////////////////////////////////////
void _sceSifDmaPrepare(int count){
	register struct TAG *t0;

	if (count==31)	return;

	t0=KSEG1_ADDR(&tadrptr[tagindex]);
	if (count == 30){
		t0->id_qwc &= 0x8c000000;	//keep PCE|REFE|IRQ
		t0->id_qwc |= 0x30000000;	//enqueue REF
		t0->id_qwc |= KSEG1_ADDR(D6_QWC);
		t0->addr    = D6_MADR;
		D6_QWC  = 0;
		D6_TADR = t0 | 0x1FFFFFFF;	//KUSEG_ADDR
	}else
		t0->id_qwc |= 0x30000000;	//REF
}

////////////////////////////////////////////////////////////////////
//800066F8		SYSCALL 119 (0x77) sceSifSetDma
////////////////////////////////////////////////////////////////////
void sceSifSetDma(sceSifDmaData *sdd, int len){
	int var_80;
	int count, tmp;
	int i, c, _len = len;

	KSEG1_ADDR(DMAC_ENABLEW) = KSEG1_ADDR(DMAC_ENABLER) | 0x10000;	//suspend

	KSEG1_ADDR(D6_CHCR) = 0;	//kill any previous transfer?!?
	var_10	= KSEG1_ADDR(D6_CHCR);	// ?!?

	KSEG1_ADDR(DMAC_ENABLEW) = KSEG1_ADDR(DMAC_ENABLER) & ~0x10000;	//enable

	count = _sceSifDmaCount();

lenloop:
	i=0; c=0;
	while (_len > 0) {
		if (!(sdd[i].mode & SIF_DMA_TAG)) {
			if (sdd[i].size <= 112) c++;
			else c+=2;
		} else c++;
		_len--; i++;
	}
	if (count < c) { count = 0; goto lenloop; }

	s3=(((tagindex+1) % 31) & 0xff);
	if (s3 == 0) {
		tmp = (transferscount + 1) & 0xFFFF;
	else
		tmp = transferscount;

	_sceSifDmaPrepare(count);

	while (len > 0) {
		_sceSifDmaSend(sdd->data, sdd->addr, sdd->size. sdd->mode, 0x3000);//REF >> 16
		sdd++; len--;
	}

	_sceSifDmaSend(sdd->data, sdd->addr, sdd->size. sdd->mode, 0);//REFE >> 16

	KSEG1_ADDR(D6_CHCR)|= 0x184;
	var_10	= KSEG1_ADDR(D6_CHCR);	// ?!?
}


////////////////////////////////////////////////////////////////////
//80010F34
////////////////////////////////////////////////////////////////////
void __disableInterrupts(int arg) {
	__asm {
		mtc0 $0, Perf
		mtc0 $0, Debug
		li   $v1, 0xFFFFFFE0
		mfc0 $v0, SR
		and  $v0, $v1
		mtc0 $v0, SR
		sync
	}
}

////////////////////////////////////////////////////////////////////
//80011108
////////////////////////////////////////////////////////////////////
int __exception() {
	int epc;
	int ret;

	__asm { mfc0 epc, EPC }

	if (epc >= 0x800112C0 &&
		epc <  0x800113AC) {
		__asm { mfc0 ret, Cause }

		return (ret & 0x7C) >> 2;
	}

	kSaveContext();

	__disableInterrupts();
	__exhandler(1);
	
	kLoadContext();

	__asm { eret }
}


#endif

// Globals declared in eeload.h

u128 SavedSP;
u128 SavedRA;
u128 SavedAT;
u64  SavedT9;

eeRegs SavedRegs;

u32  excepRA;
u32  excepSP;

int	SRInitVal;
int	ConfigInitVal;

int	threadId;
int	threadPrio;
int	threadStatus;

u64 hvParam;

u32 machineType;
u64 gsIMR;
u32 memorySize;

int VSyncFlag0;
int VSyncFlag1;

int _HandlersCount;
struct ll	handler_ll_free, *ihandlers_last, *ihandlers_first;
struct HCinfo	intcs_array[14];
struct ll	*dhandlers_last, *dhandlers_first;
struct HCinfo	dmacs_array[15];
struct IDhandl	pgifhandlers_array[129];
void (*sbus_handlers[32])(int ca);

int 		rcnt3Mode;
int 		rcnt3TargetTable[0x140];
char		rcnt3TargetNum;
int		threads_count;
struct ll	thread_ll_free;
struct ll	thread_ll_priorities[128];
int		semas_count;
int		semas_last;

struct TCB	threads_array[256];
struct kSema	semas_array[256];

char		tagindex;
short		transferscount;
struct TAG tadrptr[31];
int	extrastorage[(16/4) * 8][31];

int	osdConfigParam;

u32 sifEEbuff[32];
u32 sifRegs[32];
u32 sifIOPbuff;
u32 sif1tagdata;
