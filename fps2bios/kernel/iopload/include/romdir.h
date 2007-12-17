#ifndef __ROMDIR_H__
#define __ROMDIR_H__

#include <tamtypes.h>

struct romdir {
	/*following variable must place in designed order*/
	char fileName[10];
	unsigned short extInfoSize;
	unsigned long fileSize;
} __attribute__ ((packed));

struct rominfo {
	u32 fileOffset;
	u32 fileSize;
};

struct rominfo *romdirGetFile(char *name, struct rominfo *ri);

#endif /* __ROMDIR_H__ */
