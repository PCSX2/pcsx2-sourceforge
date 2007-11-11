/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2003  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef _SIO_H_
#define _SIO_H_

typedef struct {
	u16 StatReg;
	u16 ModeReg;
	u16 CtrlReg;
	u16 BaudReg;

	u8  buf[256];
	u32 bufcount;
	u32 parp;
	u32 mcdst,rdwr;
	u8  adrH,adrL;
	u32 padst;
	u32 mtapst;
	u32 packetsize;

	u8  terminator;
	u8  mode;
	u8  mc_command;
	u32 lastsector;
	u32 sector;
	u32 k;
	u32 count;
} _sio;
_sio sio;

#define MCD_SIZE	(1024 *  8  * 16)
#define MC2_SIZE	(1024 * 528 * 16)

// Status Flags
#define TX_RDY		0x0001
#define RX_RDY		0x0002
#define TX_EMPTY	0x0004
#define PARITY_ERR	0x0008
#define RX_OVERRUN	0x0010
#define FRAMING_ERR	0x0020
#define SYNC_DETECT	0x0040
#define DSR			0x0080
#define CTS			0x0100
#define IRQ			0x0200

// Control Flags
#define TX_PERM		0x0001
#define DTR			0x0002
#define RX_PERM		0x0004
#define BREAK		0x0008
#define RESET_ERR	0x0010
#define RTS			0x0020
#define SIO_RESET	0x0040

int Mcd1Size, Mcd2Size;

int  sioInit();
void sioShutdown();
void psxSIOShutdown();
unsigned char sioRead8();
void sioWrite8(unsigned char value);
void sioWriteCtrl16(unsigned short value);
int  sioInterrupt();
int  sioFreeze(gzFile f, int Mode);
void InitializeSIO(u8 value);

FILE *LoadMcd(int mcd);
void ReadMcd(int mcd, char *data, u32 adr, int size);
void SaveMcd(int mcd, char *data, u32 adr, int size);
void EraseMcd(int mcd, u32 adr);
void CreateMcd(char *mcd);

typedef struct {
	char Title[48];
	char ID[14];
	char Name[16];
	int IconCount;
	short Icon[16*16*3];
	unsigned char Flags;
} McdBlock;

#ifdef _MSC_VER
#pragma pack(1)
#endif
struct mc_command_0x26_tag{
	u8	field_151;	//+02 flags
	u16	sectorSize;	//+03 divide to it
	u16 field_2C;	//+05 divide to it
	u32	mc_size;	//+07
	u8	xor;		//+0b don't forget to recalculate it!!!
	u8	Z;			//+0c
#ifdef _MSC_VER
};
#pragma pack()
#else
} __attribute__((packed));
#endif

void GetMcdBlockInfo(int mcd, int block, McdBlock *info);

#endif
