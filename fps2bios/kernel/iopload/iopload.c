
#include <tamtypes.h>
#include <stdio.h>

#include "iopload.h"
#include "iopdebug.h"
#include "romdir.h"
#include "kloadcore.h"


void _start() {
	struct rominfo ri;
	struct init lci;
	void *(*sysmem_entry)(u32 iopmemsize);
	void (*loadcore_entry)(struct init *init);
	u32 offset=0;
	int i;

	__printf("iopload: _start\n");

	romdirGetFile("IOPLOAD", &ri); offset+= (ri.fileSize + 15) & ~0xf;

	lci.ramM     = 2;
    lci.bootinfo = 0;
    lci.btupdater = 0;
	lci.sysmem_LET      = (void*)offset;
    lci._pos = 0;
    lci._size = 0;
    lci.lines = sizeof(iopModules)/sizeof(iopModules[0]);
	lci.modules = (void*)iopModules;

	romdirGetFile("SYSMEM", &ri);
	sysmem_entry = (void *(*)(u32))loadElfFile("SYSMEM", offset);
	offset+= (ri.fileSize + 15) & ~0xf;
	sysmem_entry(lci.ramM);

	romdirGetFile("LOADCORE", &ri);
	loadcore_entry = (void (*)())loadElfFile("LOADCORE", offset);
	offset+= (ri.fileSize + 15) & ~0xf;
	__printf("executing LOADCORE entry at %p\n", loadcore_entry);
	//lci.offset = offset;
	loadcore_entry(&lci);

	__printf("loadcore finished ok\n");

	//syncing EE
	SBUS_SMFLG&= ~SBFLG_IOPSYNC;

	for (;;);
}

void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}

char *iopModules[32] = {
    "SYSMEM",
    "LOADCORE",
	"EXCEPMAN",
	"INTRMAN",
	"SSBUSC",
	"DMACMAN",
	"TIMRMAN",
	"SYSCLIB",
	"HEAPLIB",
	"THREADMAN",
	"VBLANK",
	"STDIO",
	"SIFMAN",
	"SIFCMD",
	NULL,
};


