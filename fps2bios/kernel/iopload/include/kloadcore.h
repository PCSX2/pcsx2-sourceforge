#ifndef __LOADCORE_H__
#define __LOADCORE_H__

#include <tamtypes.h>

#define LOADCORE_VER	0x101

typedef	int (*func)();

#define	VER(major, minor)	((((major) & 0xFF)<<8) + ((minor) & 0xFF))

#define MODULE_RESIDENT_END		0
#define MODULE_NO_RESIDENT_END	1

struct func_stub {
	int	jr_ra;	//jump instruction
	int	addiu0;	//addiu	zero, number
};

// defined by irx.h in the sdk
//#define IMPORT_MAGIC	0x41E00000
//#define EXPORT_MAGIC	0x41C00000

#define FLAG_EXPORT_QUEUED	1
#define FLAG_IMPORT_QUEUED	2

#define FLAG_NO_AUTO_LINK 1

#define INS_JR    2
#define INS_ADDIU 9
#define INS_JR_RA 0x03E00008
#define INS_J     0x08000000

struct import {
	u32		magic;		//0x41E00000
	struct import 	*next;
	short	version;	//mjmi (mj=major, mi=minor version numbers)
	short	flags;		//=0
	char	name[8];
	struct func_stub func[0];	//the last one is 0, 0 (i.e. nop, nop)
};

struct export {
	u32		magic_link;	//0x41C00000
	struct export	*next;
	short	version;	//mjmi (mj=major, mi=minor version numbers)
	short	flags;		//=0
	char	name[8];
	func	func[45];	//usually module entry point (allways?)
//		func1
//		func2
//		func3
//		funcs	  	// more functions: the services provided my module
};

struct imageInfo {
	struct imageInfo *next;		//+00
	char	*name;		//+04
	short	version,	//+08
		HA,		//+0A
		index,		//+0C
		HE;		//+0E
	int	entry,		//+10
		gp_value,	//+14
		p1_vaddr,	//+18
		text_size,	//+1C
		data_size,	//+20
		bss_size,	//+24
		H28,		//+28
		H2C;		//+2C
};

struct moduleInfo{
	char	*name;
	short	version;
};

struct bootmode {
	short	unk0;
	char	id;
	char	len;
	int	data[0];
};

struct init {
	u32	 memsize;		// in megs
	void *sysmem;		// address of sysmem export stub
	char **moduleslist;	// modules list to load
	u32 offset;			// offset addr for the next module
};

void	FlushIcache();				//4 (14)
void	FlushDcache();				//5 (14,21,26)
int	RegisterLibraryEntries(struct export*);	//6 (05,06,07,13,14,17,18,28)
int*	QueryBootMode(int code);		//12(11,21,25,26,28)
int	loadcore_call20_registerFunc(int (*function)(int *, int), int a1, int *result);
						//20(28)

#endif//__LOADCORE_H__
