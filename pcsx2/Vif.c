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

#include <math.h>
#include <string.h>

#include "Common.h"
#include "ix86/ix86.h"
#include "Vif.h"
#include "VUmicro.h"

#include <assert.h>

VIFregisters *_vifRegs;
u32* _vifMaskRegs = NULL;
PCSX2_ALIGNED16(u32 g_vifRow0[4]);
PCSX2_ALIGNED16(u32 g_vifCol0[4]);
PCSX2_ALIGNED16(u32 g_vifRow1[4]);
PCSX2_ALIGNED16(u32 g_vifCol1[4]);
u32* _vifRow = NULL;

vifStruct *_vif;

static int n;
static int i;

__inline static int _limit( int a, int max ) 
{
	return ( a > max ? max : a );
}

#define _UNPACKpart( offnum, func )                         \
	if ( ( size > 0 ) && ( _vifRegs->offset == offnum ) ) { \
		func;                                               \
		size--;                                             \
		_vifRegs->offset++;                                  \
	}

#define _UNPACKpart_nosize( offnum, func )                \
	if ( ( _vifRegs->offset == offnum ) ) { \
		func;                                               \
		_vifRegs->offset++;                                  \
	}

static void _writeX( u32 *dest, u32 data )
{
	//int n;

	switch ( _vif->cl ) {
		case 0:  n =  0; break;
		case 1:  n =  8; break;
		case 2:  n = 16; break;
		default: n = 24; break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("_writeX %x = %x (writecycle=%d; mask %x; mode %d)\n", (u32)dest-(u32)VU1.Mem, data, _vif->cl, ( _vifRegs->mask >> n ) & 0x3,_vifRegs->mode);
#endif*/
	switch ( ( _vifRegs->mask >> n ) & 0x3 ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				//SysPrintf("Phew X!\n");
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r0;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r0 = data + _vifRegs->r0;
				*dest = _vifRegs->r0;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r0; break;
		case 2: 
			switch ( _vif->cl ) {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}

/*#ifdef VIF_LOG
	VIF_LOG("_writeX-done : Data %x : Row %x\n", *dest, _vifRegs->r0);
#endif*/
}

static void _writeY( u32 *dest, u32 data )
{
	//int n;
	switch ( _vif->cl ) {
		case 0:  n =  2; break;
		case 1:  n = 10; break;
		case 2:  n = 18; break;
		default: n = 26; break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("_writeY %x = %x (writecycle=%d; mask %x; mode %d)\n", (u32)dest-(u32)VU1.Mem, data, _vif->cl, ( _vifRegs->mask >> n ) & 0x3,_vifRegs->mode);
#endif*/
	switch ( ( _vifRegs->mask >> n ) & 0x3 ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				//SysPrintf("Phew Y!\n");
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r1;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r1 = data + _vifRegs->r1;
				*dest = _vifRegs->r1;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r1; break;
		case 2: 
			switch ( _vif->cl )
         {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}

/*#ifdef VIF_LOG
	VIF_LOG("_writeY-done : Data %x : Row %x\n", *dest, _vifRegs->r1);
#endif*/
}

static void _writeZ( u32 *dest, u32 data )
{
	//int n;
	switch ( _vif->cl ) {
		case 0:  n =  4; break;
		case 1:  n = 12; break;
		case 2:  n = 20; break;
		default: n = 28; break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("_writeZ %x = %x (writecycle=%d; mask %x; mode %d)\n", (u32)dest-(u32)VU1.Mem, data, _vif->cl, ( _vifRegs->mask >> n ) & 0x3,_vifRegs->mode);
#endif*/
	switch ( ( _vifRegs->mask >> n ) & 0x3 ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				//SysPrintf("Phew Z!\n");
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r2;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r2 = data + _vifRegs->r2;
				*dest = _vifRegs->r2;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r2; break;
		case 2: 
			switch ( _vif->cl )
         {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}
}

static void _writeW( u32 *dest, u32 data )
{
	//int n;
	switch ( _vif->cl ) {
		case 0:  n =  6; break;
		case 1:  n = 14; break;
		case 2:  n = 22; break;
		default: n = 30; break;
	}
/*#ifdef VIF_LOG
	VIF_LOG("_writeW %x = %x (writecycle=%d; mask %x; mode %d)\n", (u32)dest-(u32)VU1.Mem, data, _vif->cl, ( _vifRegs->mask >> n ) & 0x3,_vifRegs->mode);
#endif*/
	switch ( ( _vifRegs->mask >> n ) & 0x3 ) {
		case 0:
			if((_vif->cmd & 0x6F) == 0x6f) {
				//SysPrintf("Phew W!\n");
				*dest = data;
				break;
			}
			if (_vifRegs->mode == 1) {
				*dest = data + _vifRegs->r3;
			} else 
			if (_vifRegs->mode == 2) {
				_vifRegs->r3 = data + _vifRegs->r3;
				*dest = _vifRegs->r3;
			} else {
				*dest = data;
			}
			break;
		case 1: *dest = _vifRegs->r3; break;
		case 2: 
			switch ( _vif->cl ) {
				case 0:  *dest = _vifRegs->c0; break;
				case 1:  *dest = _vifRegs->c1; break;
				case 2:  *dest = _vifRegs->c2; break;
				default: *dest = _vifRegs->c3; break;
			}
			break;
	}
}

static void writeX( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { _writeX(dest, data); return; }
	if((_vif->cmd & 0x6F) == 0x6f) {
		//SysPrintf("Phew X!\n");
		*dest = data;
		return;
	}
	if (_vifRegs->mode == 1) {
		*dest = data + _vifRegs->r0;
	} else 
	if (_vifRegs->mode == 2) {
		_vifRegs->r0 = data + _vifRegs->r0;
		*dest = _vifRegs->r0;
	} else {
		*dest = data;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeX %8.8x : Mode %d, r0 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r0,data);
#endif*/
}

static void writeY( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { _writeY(dest, data); return; }
	if((_vif->cmd & 0x6F) == 0x6f) {
		//SysPrintf("Phew Y!\n");
		*dest = data;
		return;
	}
	if (_vifRegs->mode == 1) {
		*dest = data + _vifRegs->r1;
	} else 
	if (_vifRegs->mode == 2) {
		_vifRegs->r1 = data + _vifRegs->r1;
		*dest = _vifRegs->r1;
	} else {
		*dest = data;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeY %8.8x : Mode %d, r1 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r1,data);
#endif*/
}

static void writeZ( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { _writeZ(dest, data); return; }
	if((_vif->cmd & 0x6F) == 0x6f) {
		//SysPrintf("Phew Z!\n");
		*dest = data;
		return;
	}
	if (_vifRegs->mode == 1) {
		*dest = data + _vifRegs->r2;
	} else 
	if (_vifRegs->mode == 2) {
		_vifRegs->r2 = data + _vifRegs->r2;
		*dest = _vifRegs->r2;
	} else {
		*dest = data;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeZ %8.8x : Mode %d, r2 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r2,data);
#endif*/
}

static void writeW( u32 *dest, u32 data ) {
	if (_vifRegs->code & 0x10000000) { _writeW(dest, data); return; }
	if((_vif->cmd & 0x6F) == 0x6f) {
		//SysPrintf("Phew X!\n");
		*dest = data;
		return;
	}
	if (_vifRegs->mode == 1) {
		*dest = data + _vifRegs->r3;
	} else 
	if (_vifRegs->mode == 2) {
		_vifRegs->r3 = data + _vifRegs->r3;
		*dest = _vifRegs->r3;
	} else {
		*dest = data;
	}
/*#ifdef VIF_LOG
	VIF_LOG("writeW %8.8x : Mode %d, r3 = %x, data %8.8x\n", *dest,_vifRegs->mode,_vifRegs->r3,data);
#endif*/
}

void UNPACK_S_32(u32 *dest, u32 *data) {
		writeX(dest++, *data);
		writeY(dest++, *data);
		writeZ(dest++, *data);
		writeW(dest++, *data++);
}

int  UNPACK_S_32part(u32 *dest, u32 *data, int size) {
	u32 *_data = data;
	while (size > 0) {
		_UNPACKpart(0, writeX(dest++, *data) );
		_UNPACKpart(1, writeY(dest++, *data) );
		_UNPACKpart(2, writeZ(dest++, *data) );
		_UNPACKpart(3, writeW(dest++, *data++) );
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;
	}

	return (u32)data - (u32)_data;
}

#define _UNPACK_S_16(format) \
	format *sdata = (format*)data; \
	 \
 \
		writeX(dest++, *sdata); \
		writeY(dest++, *sdata); \
		writeZ(dest++, *sdata); \
		writeW(dest++, *sdata++);

void UNPACK_S_16s( u32 *dest, u32 *data ) {
	_UNPACK_S_16( s16 );
}

void UNPACK_S_16u( u32 *dest, u32 *data ) {
	_UNPACK_S_16( u16 );
}

#define _UNPACK_S_16part(format) \
	format *sdata = (format*)data; \
	while (size > 0) { \
		_UNPACKpart(0, writeX(dest++, *sdata) ); \
		_UNPACKpart(1, writeY(dest++, *sdata) ); \
		_UNPACKpart(2, writeZ(dest++, *sdata) ); \
		_UNPACKpart(3, writeW(dest++, *sdata++) ); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0; \
	} \
	return (u32)sdata - (u32)data;

int  UNPACK_S_16spart(u32 *dest, u32 *data, int size) {
	_UNPACK_S_16part(s16);
}

int  UNPACK_S_16upart(u32 *dest, u32 *data, int size) {
	_UNPACK_S_16part(u16);
}

#define _UNPACK_S_8(format) \
	format *cdata = (format*)data; \
	 \
 \
		writeX(dest++, *cdata); \
		writeY(dest++, *cdata); \
		writeZ(dest++, *cdata); \
		writeW(dest++, *cdata++);

void UNPACK_S_8s(u32 *dest, u32 *data) {
	_UNPACK_S_8(s8);
}

void UNPACK_S_8u(u32 *dest, u32 *data) {
	_UNPACK_S_8(u8);
}

#define _UNPACK_S_8part(format) \
	format *cdata = (format*)data; \
	while (size > 0) { \
		_UNPACKpart(0, writeX(dest++, *cdata) ); \
		_UNPACKpart(1, writeY(dest++, *cdata) ); \
		_UNPACKpart(2, writeZ(dest++, *cdata) ); \
		_UNPACKpart(3, writeW(dest++, *cdata++) ); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0; \
	} \
	return (u32)cdata - (u32)data;

int  UNPACK_S_8spart(u32 *dest, u32 *data, int size) {
	_UNPACK_S_8part(s8);
}

int  UNPACK_S_8upart(u32 *dest, u32 *data, int size) {
	_UNPACK_S_8part(u8);
}

void UNPACK_V2_32( u32 *dest, u32 *data ) {
		writeX(dest++, *data++);
		writeY(dest++, *data);
		writeZ(dest++, *data);
		writeW(dest++, *data++);
	
}

int  UNPACK_V2_32part( u32 *dest, u32 *data, int size ) {
	u32 *_data = data;
	while (size > 0) { 
		_UNPACKpart(0, writeX(dest++, *data++));
		_UNPACKpart(1, writeY(dest++, *data++));
		_UNPACKpart_nosize(2, writeZ(dest++, 0));
		_UNPACKpart_nosize(3, writeW(dest++, 0));
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;
	}
	return (u32)data - (u32)_data;
}

#define _UNPACK_V2_16(format) \
	format *sdata = (format*)data; \
	 \
 \
		writeX(dest++, *sdata++); \
		writeY(dest++, *sdata); \
		writeZ(dest++, *sdata); \
		writeW(dest++, *sdata++); \
	

void UNPACK_V2_16s(u32 *dest, u32 *data) {
	_UNPACK_V2_16(s16);
}

void UNPACK_V2_16u(u32 *dest, u32 *data) {
	_UNPACK_V2_16(u16);
}

#define _UNPACK_V2_16part(format) \
	format *sdata = (format*)data; \
	\
	 while(size > 0) {	\
		_UNPACKpart(0, writeX(dest++, *sdata++)); \
		_UNPACKpart(1, writeY(dest++, *sdata++)); \
		_UNPACKpart_nosize(2,writeZ(dest++, 0)); \
		_UNPACKpart_nosize(3,writeW(dest++, 0)); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;	\
	 }	\
	return (u32)sdata - (u32)data;

int  UNPACK_V2_16spart(u32 *dest, u32 *data, int size) {
	_UNPACK_V2_16part(s16);
}

int  UNPACK_V2_16upart(u32 *dest, u32 *data, int size) {
	_UNPACK_V2_16part(u16);
}

#define _UNPACK_V2_8(format) \
	format *cdata = (format*)data; \
	 \
 \
		writeX(dest++, *cdata++); \
		writeY(dest++, *cdata); \
		writeZ(dest++, *cdata); \
		writeW(dest++, *cdata++); 

void UNPACK_V2_8s(u32 *dest, u32 *data) {
	_UNPACK_V2_8(s8);
}

void UNPACK_V2_8u(u32 *dest, u32 *data) {
	_UNPACK_V2_8(u8);
}

#define _UNPACK_V2_8part(format) \
	format *cdata = (format*)data; \
	 while(size > 0) {	\
		_UNPACKpart(0, writeX(dest++, *cdata++)); \
		_UNPACKpart(1, writeY(dest++, *cdata++)); \
		_UNPACKpart_nosize(2,writeZ(dest++, 0)); \
		_UNPACKpart_nosize(3,writeW(dest++, 0)); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;	\
	 }	\
	return (u32)cdata - (u32)data;

int  UNPACK_V2_8spart(u32 *dest, u32 *data, int size) {
	_UNPACK_V2_8part(s8);
}

int  UNPACK_V2_8upart(u32 *dest, u32 *data, int size) {
	_UNPACK_V2_8part(u8);
}

void UNPACK_V3_32(u32 *dest, u32 *data) {
		writeX(dest++, *data++);
		writeY(dest++, *data++);
		writeZ(dest++, *data);
		writeW(dest++, *data++);
}

int  UNPACK_V3_32part(u32 *dest, u32 *data, int size) {
	u32 *_data = data;
	while (size > 0) {
		_UNPACKpart(0, writeX(dest++, *data++); );
		_UNPACKpart(1, writeY(dest++, *data++); );
		_UNPACKpart(2, writeZ(dest++, *data++); );
		_UNPACKpart_nosize(3, writeW(dest++, 0); );
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;
	}
	return (u32)data - (u32)_data;
}

#define _UNPACK_V3_16(format) \
	format *sdata = (format*)data; \
	 \
 \
		writeX(dest++, *sdata++); \
		writeY(dest++, *sdata++); \
		writeZ(dest++, *sdata); \
		writeW(dest++, *sdata++); 

void UNPACK_V3_16s(u32 *dest, u32 *data) {
	_UNPACK_V3_16(s16);
}

void UNPACK_V3_16u(u32 *dest, u32 *data) {
	_UNPACK_V3_16(u16);
}

#define _UNPACK_V3_16part(format) \
	format *sdata = (format*)data; \
	 \
	 while(size > 0) {	\
		_UNPACKpart(0, writeX(dest++, *sdata++)); \
		_UNPACKpart(1, writeY(dest++, *sdata++)); \
		_UNPACKpart(2, writeZ(dest++, *sdata++)); \
		_UNPACKpart_nosize(3,writeW(dest++, 0)); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;	\
	 }	\
	return (u32)sdata - (u32)data;

int  UNPACK_V3_16spart(u32 *dest, u32 *data, int size) {
	_UNPACK_V3_16part(s16);
}

int  UNPACK_V3_16upart(u32 *dest, u32 *data, int size) {
	_UNPACK_V3_16part(u16);
}

#define _UNPACK_V3_8(format) \
	format *cdata = (format*)data; \
	 \
 \
		writeX(dest++, *cdata++); \
		writeY(dest++, *cdata++); \
		writeZ(dest++, *cdata); \
		writeW(dest++, *cdata++); 

void UNPACK_V3_8s(u32 *dest, u32 *data) {
	_UNPACK_V3_8(s8);
}

void UNPACK_V3_8u(u32 *dest, u32 *data) {
	_UNPACK_V3_8(u8);
}

#define _UNPACK_V3_8part(format) \
	format *cdata = (format*)data; \
	 while(size > 0) {	\
		_UNPACKpart(0, writeX(dest++, *cdata++)); \
		_UNPACKpart(1, writeY(dest++, *cdata++)); \
		_UNPACKpart(2, writeZ(dest++, *cdata++)); \
		_UNPACKpart_nosize(3,writeW(dest++, 0)); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;	\
	 }	\
	return (u32)cdata - (u32)data;

int  UNPACK_V3_8spart(u32 *dest, u32 *data, int size) {
	_UNPACK_V3_8part(s8);
}

int  UNPACK_V3_8upart(u32 *dest, u32 *data, int size) {
	_UNPACK_V3_8part(u8);
}

void UNPACK_V4_32( u32 *dest, u32 *data ) {
		writeX(dest++, *data++);
		writeY(dest++, *data++);
		writeZ(dest++, *data++);
		writeW(dest++, *data++);
}

int  UNPACK_V4_32part(u32 *dest, u32 *data, int size) {
	u32 *_data = data;
	while (size > 0) {
		_UNPACKpart(0, writeX(dest++, *data++) );
		_UNPACKpart(1, writeY(dest++, *data++) );
		_UNPACKpart(2, writeZ(dest++, *data++) );
		_UNPACKpart(3, writeW(dest++, *data++) );
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;
	}
	return (u32)data - (u32)_data;
}

#define _UNPACK_V4_16(format) \
	format *sdata = (format*)data; \
	 \
  \
		writeX(dest++, *sdata++); \
		writeY(dest++, *sdata++); \
		writeZ(dest++, *sdata++); \
		writeW(dest++, *sdata++);

void UNPACK_V4_16s(u32 *dest, u32 *data) {
	_UNPACK_V4_16(s16);
}

void UNPACK_V4_16u(u32 *dest, u32 *data) {
	_UNPACK_V4_16(u16);
}

#define _UNPACK_V4_16part(format) \
	format *sdata = (format*)data; \
	while (size > 0) { \
		_UNPACKpart(0, writeX(dest++, *sdata++) ); \
		_UNPACKpart(1, writeY(dest++, *sdata++) ); \
		_UNPACKpart(2, writeZ(dest++, *sdata++) ); \
		_UNPACKpart(3, writeW(dest++, *sdata++) ); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0; \
	} \
	return (u32)sdata - (u32)data;

int  UNPACK_V4_16spart(u32 *dest, u32 *data, int size) {
	_UNPACK_V4_16part(s16);
}

int  UNPACK_V4_16upart(u32 *dest, u32 *data, int size) {
	_UNPACK_V4_16part(u16);
}

#define _UNPACK_V4_8(format) \
	format *cdata = (format*)data; \
	 \
 \
		writeX(dest++, *cdata++); \
		writeY(dest++, *cdata++); \
		writeZ(dest++, *cdata++); \
		writeW(dest++, *cdata++);

void UNPACK_V4_8s(u32 *dest, u32 *data) {
	_UNPACK_V4_8(s8);
}

void UNPACK_V4_8u(u32 *dest, u32 *data) {
	_UNPACK_V4_8(u8);
}

#define _UNPACK_V4_8part(format) \
	format *cdata = (format*)data; \
	while (size > 0) { \
		_UNPACKpart(0, writeX(dest++, *cdata++) ); \
		_UNPACKpart(1, writeY(dest++, *cdata++) ); \
		_UNPACKpart(2, writeZ(dest++, *cdata++) ); \
		_UNPACKpart(3, writeW(dest++, *cdata++) ); \
		if (_vifRegs->offset == 4) _vifRegs->offset = 0; \
	} \
	return (u32)cdata - (u32)data;

int  UNPACK_V4_8spart(u32 *dest, u32 *data, int size) {
	_UNPACK_V4_8part(s8);
}

int  UNPACK_V4_8upart(u32 *dest, u32 *data, int size) {
	_UNPACK_V4_8part(u8);
}

void UNPACK_V4_5(u32 *dest, u32 *data) {
	u16 *sdata = (u16*)data;
	u32 rgba;

	rgba = *sdata++;
	writeX(dest++, (rgba & 0x001f) << 3);
	writeY(dest++, (rgba & 0x03e0) >> 2);
	writeZ(dest++, (rgba & 0x7c00) >> 7);
	writeW(dest++, (rgba & 0x8000) >> 8);
}

int  UNPACK_V4_5part(u32 *dest, u32 *data, int size) {
	u16 *sdata = (u16*)data;
	u32 rgba;

	while (size > 0) {
		rgba = *sdata++;
		_UNPACKpart(0, writeX(dest++, (rgba & 0x001f) << 3); );
		_UNPACKpart(1, writeY(dest++, (rgba & 0x03e0) >> 2); );
		_UNPACKpart(2, writeZ(dest++, (rgba & 0x7c00) >> 7); );
		_UNPACKpart(3, writeW(dest++, (rgba & 0x8000) >> 8); );
		if (_vifRegs->offset == 4) _vifRegs->offset = 0;
	}

	return (u32)sdata - (u32)data;
}

#if (defined(__i386__) || defined(__x86_64__))

// sse2 highly optimized vif (~200 separate functions are built) zerofrog(@gmail.com)
#include <xmmintrin.h>
#include <emmintrin.h>

PCSX2_ALIGNED16(u32 g_vif1Masks[64]);
PCSX2_ALIGNED16(g_vif0Masks[64]);
u32 g_vif1HasMask3[4] = {0}, g_vif0HasMask3[4] = {0};

//static const u32 writearr[4] = { 0xffffffff, 0, 0, 0 };
//static const u32 rowarr[4] = { 0, 0xffffffff, 0, 0 };
//static const u32 colarr[4] = { 0, 0, 0xffffffff, 0 };
//static const u32 updatearr[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0 };

// arranged in writearr, rowarr, colarr, updatearr
static PCSX2_ALIGNED16(s_maskarr[16][4]) = {
	0xffffffff, 0x00000000, 0x00000000, 0xffffffff,
	0xffff0000, 0x0000ffff, 0x00000000, 0xffffffff,
	0xffff0000, 0x00000000, 0x0000ffff, 0xffffffff,
	0xffff0000, 0x00000000, 0x00000000, 0xffff0000,
	0x0000ffff, 0xffff0000, 0x00000000, 0xffffffff,
	0x00000000, 0xffffffff, 0x00000000, 0xffffffff,
	0x00000000, 0xffff0000, 0x0000ffff, 0xffffffff,
	0x00000000, 0xffff0000, 0x00000000, 0xffff0000,
	0x0000ffff, 0x00000000, 0xffff0000, 0xffffffff,
	0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff,
	0x00000000, 0x00000000, 0xffffffff, 0xffffffff,
	0x00000000, 0x00000000, 0xffff0000, 0xffff0000,
	0x0000ffff, 0x00000000, 0x00000000, 0x0000ffff,
	0x00000000, 0x0000ffff, 0x00000000, 0x0000ffff,
	0x00000000, 0x00000000, 0x0000ffff, 0x0000ffff,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};

u8 s_maskwrite[256];
void SetNewMask(u32* vif1masks, u32* hasmask, u32 mask, u32 oldmask)
{
	u32 prev = 0;
	if( !cpucaps.hasStreamingSIMD2Extensions ) return;
	FreezeXMMRegs(1);

	for(i = 0; i < 4; ++i, mask >>= 8, oldmask >>= 8, vif1masks += 16) {

		prev |= s_maskwrite[mask&0xff];//((mask&3)==3)||((mask&0xc)==0xc)||((mask&0x30)==0x30)||((mask&0xc0)==0xc0);
		hasmask[i] = prev;

		if( (mask&0xff) != (oldmask&0xff) ) {
			__m128i r0, r1, r2, r3;
			r0 = _mm_load_si128((__m128i*)&s_maskarr[mask&15][0]);
			r2 = _mm_unpackhi_epi16(r0, r0);
			r0 = _mm_unpacklo_epi16(r0, r0);

			r1 = _mm_load_si128((__m128i*)&s_maskarr[(mask>>4)&15][0]);
			r3 = _mm_unpackhi_epi16(r1, r1);
			r1 = _mm_unpacklo_epi16(r1, r1);

			_mm_storel_pi((__m64*)&vif1masks[0], *(__m128*)&r0);
			_mm_storel_pi((__m64*)&vif1masks[2], *(__m128*)&r1);
			_mm_storeh_pi((__m64*)&vif1masks[4], *(__m128*)&r0);
			_mm_storeh_pi((__m64*)&vif1masks[6], *(__m128*)&r1);

			_mm_storel_pi((__m64*)&vif1masks[8], *(__m128*)&r2);
			_mm_storel_pi((__m64*)&vif1masks[10], *(__m128*)&r3);
			_mm_storeh_pi((__m64*)&vif1masks[12], *(__m128*)&r2);
			_mm_storeh_pi((__m64*)&vif1masks[14], *(__m128*)&r3);
		}
	}
}

// msvc++
#define VIF_SRC	ecx
#define VIF_INC	edx
#define VIF_DST edi

// writing masks
#define UNPACK_Write0_Regular(r0, CL, DEST_OFFSET, MOVDQA) \
{ \
	__asm MOVDQA qword ptr [VIF_DST+(DEST_OFFSET)], r0 \
} \

#define UNPACK_Write1_Regular(r0, CL, DEST_OFFSET, MOVDQA) \
{ \
	__asm MOVDQA qword ptr [VIF_DST], r0 \
	__asm add VIF_DST, VIF_INC \
} \

#define UNPACK_Write0_Mask UNPACK_Write0_Regular
#define UNPACK_Write1_Mask UNPACK_Write1_Regular

#define UNPACK_Write0_WriteMask(r0, CL, DEST_OFFSET, MOVDQA) \
{ \
	/* masked write (dest needs to be in edi) */ \
	__asm movdqa XMM_WRITEMASK, qword ptr [eax + 64*(CL) + 48] \
	/*__asm maskmovdqu r0, XMM_WRITEMASK*/ \
	__asm pand r0, XMM_WRITEMASK \
	__asm pandn XMM_WRITEMASK, qword ptr [VIF_DST] \
	__asm por r0, XMM_WRITEMASK \
	__asm MOVDQA qword ptr [VIF_DST], r0 \
	__asm add VIF_DST, 16 \
} \

#define UNPACK_Write1_WriteMask(r0, CL, DEST_OFFSET, MOVDQA) \
{ \
	__asm movdqa XMM_WRITEMASK, qword ptr [eax + 64*(0) + 48] \
	/* masked write (dest needs to be in edi) */ \
	/*__asm maskmovdqu r0, XMM_WRITEMASK*/ \
	__asm pand r0, XMM_WRITEMASK \
	__asm pandn XMM_WRITEMASK, qword ptr [VIF_DST] \
	__asm por r0, XMM_WRITEMASK \
	__asm MOVDQA qword ptr [VIF_DST], r0 \
	__asm add VIF_DST, VIF_INC \
} \

#define UNPACK_Mask_SSE_0(r0) \
{ \
	__asm pand r0, XMM_WRITEMASK \
	__asm por r0, XMM_ROWCOLMASK \
} \

// once a qword is uncomprssed, applies masks and saves
// note: modifying XMM_WRITEMASK
#define UNPACK_Mask_SSE_1(r0) \
{ \
	/* dest = row + write (only when mask=0), otherwise write */ \
	__asm pand r0, XMM_WRITEMASK \
	__asm por r0, XMM_ROWCOLMASK \
	__asm pand XMM_WRITEMASK, XMM_ROW \
	__asm paddd r0, XMM_WRITEMASK \
} \

#define UNPACK_Mask_SSE_2(r0) \
{ \
	/* dest = row + write (only when mask=0), otherwise write \
		row = row + write (only when mask = 0), otherwise row */ \
	__asm pand r0, XMM_WRITEMASK \
	__asm pand XMM_WRITEMASK, XMM_ROW \
	__asm paddd XMM_ROW, r0 \
	__asm por r0, XMM_ROWCOLMASK \
	__asm paddd r0, XMM_WRITEMASK \
} \

#define UNPACK_WriteMask_SSE_0 UNPACK_Mask_SSE_0
#define UNPACK_WriteMask_SSE_1 UNPACK_Mask_SSE_1
#define UNPACK_WriteMask_SSE_2 UNPACK_Mask_SSE_2

#define UNPACK_Regular_SSE_0(r0)

#define UNPACK_Regular_SSE_1(r0) \
{ \
	__asm paddd r0, XMM_ROW \
} \

#define UNPACK_Regular_SSE_2(r0) \
{ \
	__asm paddd r0, XMM_ROW \
	__asm movdqa XMM_ROW, r0 \
} \

// setting up masks
#define UNPACK_Setup_Mask_SSE(CL) \
{ \
	__asm mov eax, _vifMaskRegs \
	__asm movdqa XMM_ROWMASK, qword ptr [eax + 64*(CL) + 16] \
	__asm movdqa XMM_ROWCOLMASK, qword ptr [eax + 64*(CL) + 32] \
	__asm movdqa XMM_WRITEMASK, qword ptr [eax + 64*(CL)] \
	__asm pand XMM_ROWMASK, XMM_ROW \
	__asm pand XMM_ROWCOLMASK, XMM_COL \
	__asm por XMM_ROWCOLMASK, XMM_ROWMASK \
} \

#define UNPACK_Start_Setup_Mask_SSE_0(CL) UNPACK_Setup_Mask_SSE(CL);
#define UNPACK_Start_Setup_Mask_SSE_1(CL) \
{ \
	__asm mov eax, _vifMaskRegs \
	__asm movdqa XMM_ROWMASK, qword ptr [eax + 64*(CL) + 16] \
	__asm movdqa XMM_ROWCOLMASK, qword ptr [eax + 64*(CL) + 32] \
	__asm pand XMM_ROWMASK, XMM_ROW \
	__asm pand XMM_ROWCOLMASK, XMM_COL \
	__asm por XMM_ROWCOLMASK, XMM_ROWMASK \
} \

#define UNPACK_Start_Setup_Mask_SSE_2(CL)

#define UNPACK_Setup_Mask_SSE_0_1(CL) 
#define UNPACK_Setup_Mask_SSE_1_1(CL) \
{ \
	__asm mov eax, _vifMaskRegs \
	__asm movdqa XMM_WRITEMASK, qword ptr [eax + 64*(0)] \
} \

#define UNPACK_Setup_Mask_SSE_2_1(CL) { \
	/* ignore CL, since vif.cycle.wl == 1 */ \
	__asm mov eax, _vifMaskRegs \
	__asm movdqa XMM_ROWMASK, qword ptr [eax + 64*(0) + 16] \
	__asm movdqa XMM_ROWCOLMASK, qword ptr [eax + 64*(0) + 32] \
	__asm movdqa XMM_WRITEMASK, qword ptr [eax + 64*(0)] \
	__asm pand XMM_ROWMASK, XMM_ROW \
	__asm pand XMM_ROWCOLMASK, XMM_COL \
	__asm por XMM_ROWCOLMASK, XMM_ROWMASK \
} \

#define UNPACK_Setup_Mask_SSE_0_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_Mask_SSE_1_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_Mask_SSE_2_0(CL) UNPACK_Setup_Mask_SSE(CL)

// write mask always destroys XMM_WRITEMASK, so 0_0 = 1_0
#define UNPACK_Setup_WriteMask_SSE_0_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_WriteMask_SSE_1_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_WriteMask_SSE_2_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_WriteMask_SSE_0_1(CL) UNPACK_Setup_Mask_SSE_1_1(CL)
#define UNPACK_Setup_WriteMask_SSE_1_1(CL) UNPACK_Setup_Mask_SSE_1_1(CL)
#define UNPACK_Setup_WriteMask_SSE_2_1(CL) UNPACK_Setup_Mask_SSE_2_1(CL)

#define UNPACK_Start_Setup_WriteMask_SSE_0(CL) UNPACK_Start_Setup_Mask_SSE_1(CL)
#define UNPACK_Start_Setup_WriteMask_SSE_1(CL) UNPACK_Start_Setup_Mask_SSE_1(CL)
#define UNPACK_Start_Setup_WriteMask_SSE_2(CL) UNPACK_Start_Setup_Mask_SSE_2(CL)

#define UNPACK_Start_Setup_Regular_SSE_0(CL)
#define UNPACK_Start_Setup_Regular_SSE_1(CL)
#define UNPACK_Start_Setup_Regular_SSE_2(CL)
#define UNPACK_Setup_Regular_SSE_0_0(CL)
#define UNPACK_Setup_Regular_SSE_1_0(CL)
#define UNPACK_Setup_Regular_SSE_2_0(CL)
#define UNPACK_Setup_Regular_SSE_0_1(CL)
#define UNPACK_Setup_Regular_SSE_1_1(CL)
#define UNPACK_Setup_Regular_SSE_2_1(CL)

#define UNPACK_INC_DST_0_Regular(qw) __asm add VIF_DST, (16*qw)
#define UNPACK_INC_DST_1_Regular(qw)
#define UNPACK_INC_DST_0_Mask(qw) __asm add VIF_DST, (16*qw)
#define UNPACK_INC_DST_1_Mask(qw)
#define UNPACK_INC_DST_0_WriteMask(qw)
#define UNPACK_INC_DST_1_WriteMask(qw)

// unpacks for 1,2,3,4 elements (V3 uses this directly)
#define UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType) { \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+0); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+3); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R3); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R3, CL+3, 48, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(4) \
} \

// V3 uses this directly
#define UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType) { \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(3) \
} \

#define UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType) { \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(2) \
} \

#define UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType) { \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(1) \
} \

// S-32
// only when cl==1
#define UNPACK_S_32SSE_4x(CL, TOTALCL, MaskType, ModeType, MOVDQA) { \
	{ \
		__asm MOVDQA XMM_R3, qword ptr [VIF_SRC] \
		\
		__asm pshufd XMM_R0, XMM_R3, 0 \
		__asm pshufd XMM_R1, XMM_R3, 0x55 \
		__asm pshufd XMM_R2, XMM_R3, 0xaa \
		__asm pshufd XMM_R3, XMM_R3, 0xff \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

#define UNPACK_S_32SSE_4A(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_S_32SSE_4(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_S_32SSE_3x(CL, TOTALCL, MaskType, ModeType, MOVDQA) { \
	{ \
		__asm MOVDQA XMM_R2, qword ptr [VIF_SRC] \
		\
		__asm pshufd XMM_R0, XMM_R2, 0 \
		__asm pshufd XMM_R1, XMM_R2, 0x55 \
		__asm pshufd XMM_R2, XMM_R2, 0xaa \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_S_32SSE_3A(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_S_32SSE_3(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_S_32SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R1, qword ptr [VIF_SRC] \
		\
		__asm pshufd XMM_R0, XMM_R1, 0 \
		__asm pshufd XMM_R1, XMM_R1, 0x55 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_S_32SSE_2A UNPACK_S_32SSE_2

#define UNPACK_S_32SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm pshufd XMM_R0, XMM_R0, 0 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

#define UNPACK_S_32SSE_1A UNPACK_S_32SSE_1

// S-16
#define UNPACK_S_16SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R3, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R3, XMM_R3 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 16 \
		\
		__asm pshufd XMM_R0, XMM_R3, 0 \
		__asm pshufd XMM_R1, XMM_R3, 0x55 \
		__asm pshufd XMM_R2, XMM_R3, 0xaa \
		__asm pshufd XMM_R3, XMM_R3, 0xff \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
}

#define UNPACK_S_16SSE_4A UNPACK_S_16SSE_4

#define UNPACK_S_16SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R2, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R2, XMM_R2 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
		\
		__asm pshufd XMM_R0, XMM_R2, 0 \
		__asm pshufd XMM_R1, XMM_R2, 0x55 \
		__asm pshufd XMM_R2, XMM_R2, 0xaa \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
	__asm add VIF_SRC, 6 \
	} \
} \

#define UNPACK_S_16SSE_3A UNPACK_S_16SSE_3

#define UNPACK_S_16SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R1, dword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R1, XMM_R1 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		\
		__asm pshufd XMM_R0, XMM_R1, 0 \
		__asm pshufd XMM_R1, XMM_R1, 0x55 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

#define UNPACK_S_16SSE_2A UNPACK_S_16SSE_2

#define UNPACK_S_16SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm pshufd XMM_R0, XMM_R0, 0 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 2 \
	} \
} \

#define UNPACK_S_16SSE_1A UNPACK_S_16SSE_1

// S-8
#define UNPACK_S_8SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R3, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R3, XMM_R3 \
		__asm punpcklwd XMM_R3, XMM_R3 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 24 \
		\
		__asm pshufd XMM_R0, XMM_R3, 0 \
		__asm pshufd XMM_R1, XMM_R3, 0x55 \
		__asm pshufd XMM_R2, XMM_R3, 0xaa \
		__asm pshufd XMM_R3, XMM_R3, 0xff \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
}

#define UNPACK_S_8SSE_4A UNPACK_S_8SSE_4

#define UNPACK_S_8SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R2, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R2, XMM_R2 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
		\
		__asm pshufd XMM_R0, XMM_R2, 0 \
		__asm pshufd XMM_R1, XMM_R2, 0x55 \
		__asm pshufd XMM_R2, XMM_R2, 0xaa \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 3 \
	} \
} \

#define UNPACK_S_8SSE_3A UNPACK_S_8SSE_3

#define UNPACK_S_8SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R1, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R1, XMM_R1 \
		__asm punpcklwd XMM_R1, XMM_R1 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		\
		__asm pshufd XMM_R0, XMM_R1, 0 \
		__asm pshufd XMM_R1, XMM_R1, 0x55 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 2 \
	} \
} \

#define UNPACK_S_8SSE_2A UNPACK_S_8SSE_2

#define UNPACK_S_8SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm pshufd XMM_R0, XMM_R0, 0 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm inc VIF_SRC \
	} \
} \

#define UNPACK_S_8SSE_1A UNPACK_S_8SSE_1

// V2-32
#define UNPACK_V2_32SSE_4A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm MOVDQA XMM_R0, qword ptr [VIF_SRC] \
		__asm MOVDQA XMM_R2, qword ptr [VIF_SRC+16] \
		\
		__asm pshufd XMM_R1, XMM_R0, 0xee \
		__asm pshufd XMM_R3, XMM_R2, 0xee \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 32 \
	} \
}

#define UNPACK_V2_32SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+8] \
		__asm movq XMM_R2, qword ptr [VIF_SRC+16] \
		__asm movq XMM_R3, qword ptr [VIF_SRC+24] \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 32 \
	} \
}

#define UNPACK_V2_32SSE_3A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm MOVDQA XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R2, qword ptr [VIF_SRC+16] \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 24 \
	} \
} \

#define UNPACK_V2_32SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+8] \
		__asm movq XMM_R2, qword ptr [VIF_SRC+16] \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 24 \
	} \
} \

#define UNPACK_V2_32SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+8] \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
} \

#define UNPACK_V2_32SSE_2A UNPACK_V2_32SSE_2

#define UNPACK_V2_32SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_V2_32SSE_1A UNPACK_V2_32SSE_1

// V2-16
#define UNPACK_V2_16SSE_4A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhwd XMM_R2, qword ptr [VIF_SRC] \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
		__asm pshufd XMM_R3, XMM_R2, 0xee \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

#define UNPACK_V2_16SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpckhwd XMM_R2, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
		__asm pshufd XMM_R3, XMM_R2, 0xee \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

#define UNPACK_V2_16SSE_3A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhwd XMM_R2, qword ptr [VIF_SRC] \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_V2_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpckhwd XMM_R2, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_V2_16SSE_2A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_V2_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_V2_16SSE_1A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm punpcklwd XMM_R0, dword ptr [VIF_SRC] \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

#define UNPACK_V2_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

// V2-8
#define UNPACK_V2_8SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpckhwd XMM_R2, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
		__asm pshufd XMM_R3, XMM_R2, 0xee \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
}

#define UNPACK_V2_8SSE_4A UNPACK_V2_8SSE_4

#define UNPACK_V2_8SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpckhwd XMM_R2, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 6 \
	} \
} \

#define UNPACK_V2_8SSE_3A UNPACK_V2_8SSE_3

#define UNPACK_V2_8SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		\
		/* move the lower 64 bits down*/ \
		__asm pshufd XMM_R1, XMM_R0, 0xee \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

#define UNPACK_V2_8SSE_2A UNPACK_V2_8SSE_2

#define UNPACK_V2_8SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 2 \
	} \
} \

#define UNPACK_V2_8SSE_1A UNPACK_V2_8SSE_1

// V3-32
#define UNPACK_V3_32SSE_4x(CL, TOTALCL, MaskType, ModeType, MOVDQA) { \
	{ \
		__asm MOVDQA XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R1, qword ptr [VIF_SRC+12] \
	} \
	{ \
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+0); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
		\
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	} \
	{ \
		__asm movdqu XMM_R2, qword ptr [VIF_SRC+24] \
		__asm movdqu XMM_R3, qword ptr [VIF_SRC+36] \
	} \
	{ \
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
		\
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+3); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R3); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R3, CL+3, 48, movdqa); \
		\
		UNPACK_INC_DST_##TOTALCL##_##MaskType##(4) \
	} \
	{ \
		__asm add VIF_SRC, 48 \
	} \
}

#define UNPACK_V3_32SSE_4A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_4(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_V3_32SSE_3x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
{ \
	{ \
		__asm MOVDQA XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R1, qword ptr [VIF_SRC+12] \
	} \
	{ \
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
		\
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	} \
	{ \
		__asm movdqu XMM_R2, qword ptr [VIF_SRC+24] \
	} \
	{ \
		UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
		UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
		UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
		\
		UNPACK_INC_DST_##TOTALCL##_##MaskType##(3) \
	} \
	{ \
		__asm add VIF_SRC, 36 \
	} \
} \

#define UNPACK_V3_32SSE_3A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_3(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_V3_32SSE_2x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
{ \
	{ \
		__asm MOVDQA XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R1, qword ptr [VIF_SRC+12] \
	} \
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 24 \
	} \
} \

#define UNPACK_V3_32SSE_2A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_2x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_2(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_2x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_V3_32SSE_1x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
{ \
	{ \
		__asm MOVDQA XMM_R0, qword ptr [VIF_SRC] \
	} \
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_V3_32SSE_1A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_1x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_1(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_1x(CL, TOTALCL, MaskType, ModeType, movdqu)

// V3-16
#define UNPACK_V3_16SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+6] \
		\
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm movq XMM_R2, qword ptr [VIF_SRC+12] \
		__asm punpcklwd XMM_R1, XMM_R1 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm movq XMM_R3, qword ptr [VIF_SRC+18] \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		__asm punpcklwd XMM_R3, XMM_R3 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 16 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 24 \
	} \
}

#define UNPACK_V3_16SSE_4A UNPACK_V3_16SSE_4

#define UNPACK_V3_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+6] \
		\
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm movq XMM_R2, qword ptr [VIF_SRC+12] \
		__asm punpcklwd XMM_R1, XMM_R1 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 18 \
	} \
} \

#define UNPACK_V3_16SSE_3A UNPACK_V3_16SSE_3

#define UNPACK_V3_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+6] \
		\
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R1, XMM_R1 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_V3_16SSE_2A UNPACK_V3_16SSE_2

#define UNPACK_V3_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 6 \
	} \
} \

#define UNPACK_V3_16SSE_1A UNPACK_V3_16SSE_1

// V3-8
#define UNPACK_V3_8SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R1, qword ptr [VIF_SRC] \
		__asm movq XMM_R3, qword ptr [VIF_SRC+6] \
		\
		__asm punpcklbw XMM_R1, XMM_R1 \
		__asm punpcklbw XMM_R3, XMM_R3 \
		__asm punpcklwd XMM_R0, XMM_R1 \
		__asm psrldq XMM_R1, 6 \
		__asm punpcklwd XMM_R2, XMM_R3 \
		__asm psrldq XMM_R3, 6 \
		__asm punpcklwd XMM_R1, XMM_R1 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm punpcklwd XMM_R3, XMM_R3 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 24 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
}

#define UNPACK_V3_8SSE_4A UNPACK_V3_8SSE_4

#define UNPACK_V3_8SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, word ptr [VIF_SRC] \
		__asm movd XMM_R1, dword ptr [VIF_SRC+3] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm movd XMM_R2, dword ptr [VIF_SRC+6] \
		__asm punpcklbw XMM_R1, XMM_R1 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklbw XMM_R2, XMM_R2 \
		\
		__asm punpcklwd XMM_R1, XMM_R1 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 9 \
	} \
} \

#define UNPACK_V3_8SSE_3A UNPACK_V3_8SSE_3

#define UNPACK_V3_8SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm movd XMM_R1, dword ptr [VIF_SRC+3] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklbw XMM_R1, XMM_R1 \
		\
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R1, XMM_R1 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 6 \
	} \
} \

#define UNPACK_V3_8SSE_2A UNPACK_V3_8SSE_2

#define UNPACK_V3_8SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 3 \
	} \
} \

#define UNPACK_V3_8SSE_1A UNPACK_V3_8SSE_1

// V4-32
#define UNPACK_V4_32SSE_4A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movdqa XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqa XMM_R1, qword ptr [VIF_SRC+16] \
		__asm movdqa XMM_R2, qword ptr [VIF_SRC+32] \
		__asm movdqa XMM_R3, qword ptr [VIF_SRC+48] \
	} \
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 64 \
	} \
}

#define UNPACK_V4_32SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R1, qword ptr [VIF_SRC+16] \
		__asm movdqu XMM_R2, qword ptr [VIF_SRC+32] \
		__asm movdqu XMM_R3, qword ptr [VIF_SRC+48] \
	} \
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 64 \
	} \
}

#define UNPACK_V4_32SSE_3A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqa XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqa XMM_R1, qword ptr [VIF_SRC+16] \
		__asm movdqa XMM_R2, qword ptr [VIF_SRC+32] \
	} \
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 48 \
	} \
}

#define UNPACK_V4_32SSE_3(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R1, qword ptr [VIF_SRC+16] \
		__asm movdqu XMM_R2, qword ptr [VIF_SRC+32] \
	} \
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 48 \
	} \
}

#define UNPACK_V4_32SSE_2A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqa XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqa XMM_R1, qword ptr [VIF_SRC+16] \
	} \
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 32 \
	} \
}

#define UNPACK_V4_32SSE_2(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R1, qword ptr [VIF_SRC+16] \
	} \
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 32 \
	} \
}

#define UNPACK_V4_32SSE_1A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqa XMM_R0, qword ptr [VIF_SRC] \
	} \
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

#define UNPACK_V4_32SSE_1(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
	} \
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

// V4-16
#define UNPACK_V4_16SSE_4A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhwd XMM_R1, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R2, qword ptr [VIF_SRC+16] \
		__asm punpckhwd XMM_R3, qword ptr [VIF_SRC+16] \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 32 \
	} \
}

#define UNPACK_V4_16SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		__asm movdqu XMM_R2, qword ptr [VIF_SRC+16] \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpckhwd XMM_R3, XMM_R2 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 32 \
	} \
}

#define UNPACK_V4_16SSE_3A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhwd XMM_R1, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R2, qword ptr [VIF_SRC+16] \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 24 \
	} \
} \

#define UNPACK_V4_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R2, qword ptr [VIF_SRC+16] \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 16 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 24 \
	} \
} \

#define UNPACK_V4_16SSE_2A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhwd XMM_R1, qword ptr [VIF_SRC] \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
} \

#define UNPACK_V4_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movq XMM_R1, qword ptr [VIF_SRC+8] \
		\
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R1, XMM_R1 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 16 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
} \

#define UNPACK_V4_16SSE_1A(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm punpcklwd XMM_R0, qword ptr [VIF_SRC] \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_V4_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
{ \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 16 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

// V4-8
#define UNPACK_V4_8SSE_4A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm punpcklbw XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhbw XMM_R2, qword ptr [VIF_SRC] \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpckhwd XMM_R3, XMM_R2 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R3, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

#define UNPACK_V4_8SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movdqu XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpckhbw XMM_R2, XMM_R0 \
		__asm punpcklbw XMM_R0, XMM_R0 \
		\
		__asm punpckhwd XMM_R3, XMM_R2 \
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R3, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 16 \
	} \
}

#define UNPACK_V4_8SSE_3A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm punpcklbw XMM_R0, qword ptr [VIF_SRC] \
		__asm punpckhbw XMM_R2, qword ptr [VIF_SRC] \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_V4_8SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		__asm movd XMM_R2, dword ptr [VIF_SRC+8] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklbw XMM_R2, XMM_R2 \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R2, 24 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 12 \
	} \
} \

#define UNPACK_V4_8SSE_2A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm punpcklbw XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_V4_8SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movq XMM_R0, qword ptr [VIF_SRC] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm UNPACK_RIGHTSHIFT XMM_R1, 24 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
} \

#define UNPACK_V4_8SSE_1A(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm punpcklbw XMM_R0, qword ptr [VIF_SRC] \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

#define UNPACK_V4_8SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm movd XMM_R0, dword ptr [VIF_SRC] \
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm UNPACK_RIGHTSHIFT XMM_R0, 24 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

// V4-5
static PCSX2_ALIGNED16(u32 s_TempDecompress[4]) = {0};

#define DECOMPRESS_RGBA(OFFSET) { \
	/* R */ \
	__asm mov bl, al \
	__asm shl bl, 3 \
	__asm mov byte ptr [s_TempDecompress+OFFSET], bl \
	/* G */ \
	__asm mov bx, ax \
	__asm shr bx, 2 \
	__asm and bx, 0xf8 \
	__asm mov byte ptr [s_TempDecompress+OFFSET+1], bl \
	/* B */ \
	__asm mov bx, ax \
	__asm shr bx, 7 \
	__asm and bx, 0xf8 \
	__asm mov byte ptr [s_TempDecompress+OFFSET+2], bl \
	__asm mov bx, ax \
	__asm shr bx, 8 \
	__asm and bx, 0x80 \
	__asm mov byte ptr [s_TempDecompress+OFFSET+3], bl \
} \

#define UNPACK_V4_5SSE_4(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm mov eax, dword ptr [VIF_SRC] \
	} \
	DECOMPRESS_RGBA(0); \
	{ \
		__asm shr eax, 16 \
	} \
	DECOMPRESS_RGBA(4); \
	{ \
		__asm mov ax, word ptr [VIF_SRC+4] \
	} \
	DECOMPRESS_RGBA(8); \
	{ \
		__asm mov ax, word ptr [VIF_SRC+6] \
	} \
	DECOMPRESS_RGBA(12); \
	{ \
		__asm movdqa XMM_R0, qword ptr [s_TempDecompress] \
		\
		__asm punpckhbw XMM_R2, XMM_R0 \
		__asm punpcklbw XMM_R0, XMM_R0 \
		\
		__asm punpckhwd XMM_R3, XMM_R2 \
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm psrld XMM_R0, 24 \
		__asm psrld XMM_R1, 24 \
		__asm psrld XMM_R2, 24 \
		__asm psrld XMM_R3, 24 \
	} \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 8 \
	} \
}

#define UNPACK_V4_5SSE_4A UNPACK_V4_5SSE_4

#define UNPACK_V4_5SSE_3(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm mov eax, dword ptr [VIF_SRC] \
	} \
	DECOMPRESS_RGBA(0); \
	{ \
		__asm shr eax, 16 \
	} \
	DECOMPRESS_RGBA(4); \
	{ \
		__asm mov ax, word ptr [VIF_SRC+4] \
	} \
	DECOMPRESS_RGBA(8); \
	{ \
		__asm movdqa XMM_R0, qword ptr [s_TempDecompress] \
		\
		__asm punpckhbw XMM_R2, XMM_R0 \
		__asm punpcklbw XMM_R0, XMM_R0 \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R2, XMM_R2 \
		\
		__asm psrld XMM_R0, 24 \
		__asm psrld XMM_R1, 24 \
		__asm psrld XMM_R2, 24 \
	} \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 6 \
	} \
} \

#define UNPACK_V4_5SSE_3A UNPACK_V4_5SSE_3

#define UNPACK_V4_5SSE_2(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm mov eax, dword ptr [VIF_SRC] \
	} \
	DECOMPRESS_RGBA(0); \
	{ \
		__asm shr eax, 16 \
	} \
	DECOMPRESS_RGBA(4); \
	{ \
		__asm movq XMM_R0, qword ptr [s_TempDecompress] \
		\
		__asm punpcklbw XMM_R0, XMM_R0 \
		\
		__asm punpckhwd XMM_R1, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm psrld XMM_R0, 24 \
		__asm psrld XMM_R1, 24 \
	} \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 4 \
	} \
} \

#define UNPACK_V4_5SSE_2A UNPACK_V4_5SSE_2

#define UNPACK_V4_5SSE_1(CL, TOTALCL, MaskType, ModeType) { \
	{ \
		__asm mov ax, word ptr [VIF_SRC] \
	} \
	DECOMPRESS_RGBA(0); \
	{ \
		__asm movd XMM_R0, dword ptr [s_TempDecompress] \
		__asm punpcklbw XMM_R0, XMM_R0 \
		__asm punpcklwd XMM_R0, XMM_R0 \
		\
		__asm psrld XMM_R0, 24 \
	} \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	{ \
		__asm add VIF_SRC, 2 \
	} \
} \

#define UNPACK_V4_5SSE_1A UNPACK_V4_5SSE_1

#pragma warning(disable:4731)

//#ifdef _DEBUG
#define PUSHESI __asm mov s_saveesi, esi
#define POPESI __asm mov esi, s_saveesi
//#else
//#define PUSHESI
//#define POPESI
//#endif

#define PUSHEDI
#define POPEDI
#define PUSHEBP //__asm mov dword ptr [esp-4], ebp
#define POPEBP //__asm mov ebp, dword ptr [esp-4]

//__asm mov eax, pr0 \
///* load row */ \
//__asm movss XMM_ROW, dword ptr [eax] \
//__asm movss XMM_R1, dword ptr [eax+4] \
//__asm punpckldq XMM_ROW, XMM_R1 \
//__asm movss XMM_R0, dword ptr [eax+8] \
//__asm movss XMM_R1, dword ptr [eax+12] \
//__asm punpckldq XMM_R0, XMM_R1 \
//__asm punpcklqdq XMM_ROW, XMM_R0 \
//\
//__asm mov eax, pc0 \
//__asm movss XMM_R3, dword ptr [eax] \
//__asm movss XMM_R1, dword ptr [eax+4] \
//__asm punpckldq XMM_R3, XMM_R1 \
//__asm movss XMM_R0, dword ptr [eax+8] \
//__asm movss XMM_R1, dword ptr [eax+12] \
//__asm punpckldq XMM_R0, XMM_R1 \
//__asm punpcklqdq XMM_R3, XMM_R0 \

#define SAVE_ROW_REG_BASE { \
	{ \
		/* save the row reg */ \
		__asm mov eax, _vifRow \
		__asm movdqa qword ptr [eax], XMM_ROW \
		__asm mov eax, _vifRegs \
		__asm movss dword ptr [eax+0x100], XMM_ROW \
		__asm psrldq XMM_ROW, 4 \
		__asm movss dword ptr [eax+0x110], XMM_ROW \
		__asm psrldq XMM_ROW, 4 \
		__asm movss dword ptr [eax+0x120], XMM_ROW \
		__asm psrldq XMM_ROW, 4 \
		__asm movss dword ptr [eax+0x130], XMM_ROW \
	} \
} \

#define SAVE_NO_REG

extern int g_nCounters[4];

static int tempcl = 0, incdest;
static int s_saveesi, s_saveinc;

// qsize - bytes of compressed size of 1 decompressed qword
#define defUNPACK_SkippingWrite(name, MaskType, ModeType, qsize, sign, SAVE_ROW_REG) \
int UNPACK_SkippingWrite_##name##_##sign##_##MaskType##_##ModeType##(u32* dest, u32* data, int dmasize) \
{ \
	incdest = ((_vifRegs->cycle.cl - _vifRegs->cycle.wl)<<4); \
	\
	switch( _vifRegs->cycle.wl ) { \
		case 1: \
			{ \
				/*__asm inc dword ptr [g_nCounters] \
				__asm mov eax, dmasize \
				__asm add dword ptr [g_nCounters+4], eax*/ \
				PUSHESI \
				PUSHEDI \
				__asm mov esi, dmasize \
			} \
			UNPACK_Start_Setup_##MaskType##_SSE_##ModeType##(0); \
			{ \
				__asm cmp esi, qsize \
				__asm jl C1_Done3 \
				\
				/* move source and dest */ \
				__asm mov VIF_DST, dest \
				__asm mov VIF_SRC, data \
				__asm mov VIF_INC, incdest \
				__asm add VIF_INC, 16 \
			} \
			\
			/* first align VIF_SRC to 16 bytes */ \
C1_Align16: \
			{ \
				__asm test VIF_SRC, 15 \
				__asm jz C1_UnpackAligned \
			} \
			UNPACK_##name##SSE_1(0, 1, MaskType, ModeType); \
			{ \
				__asm cmp esi, (2*qsize) \
				__asm jl C1_DoneWithDec \
				__asm sub esi, qsize \
				__asm jmp C1_Align16 \
			} \
C1_UnpackAligned: \
			{ \
				__asm cmp esi, (2*qsize) \
				__asm jl C1_Unpack1 \
				__asm cmp esi, (3*qsize) \
				__asm jl C1_Unpack2 \
				__asm cmp esi, (4*qsize) \
				__asm jl C1_Unpack3 \
				__asm prefetchnta [eax + 192] \
			} \
C1_Unpack4: \
			UNPACK_##name##SSE_4A(0, 1, MaskType, ModeType); \
			{ \
				__asm cmp esi, (8*qsize) \
				__asm jl C1_DoneUnpack4 \
				__asm sub esi, (4*qsize) \
				__asm jmp C1_Unpack4 \
			} \
C1_DoneUnpack4: \
			{ \
				__asm sub esi, (4*qsize) \
				__asm cmp esi, qsize \
				__asm jl C1_Done3 \
				__asm cmp esi, (2*qsize) \
				__asm jl C1_Unpack1 \
				__asm cmp esi, (3*qsize) \
				__asm jl C1_Unpack2 \
				/* fall through */ \
			} \
C1_Unpack3: \
			UNPACK_##name##SSE_3A(0, 1, MaskType, ModeType); \
			{ \
				__asm sub esi, (3*qsize) \
				__asm jmp C1_Done3 \
			} \
C1_Unpack2: \
			UNPACK_##name##SSE_2A(0, 1, MaskType, ModeType); \
			{ \
				__asm sub esi, (2*qsize) \
				__asm jmp C1_Done3 \
			} \
C1_Unpack1: \
			UNPACK_##name##SSE_1A(0, 1, MaskType, ModeType); \
C1_DoneWithDec: \
			{ \
				__asm sub esi, qsize \
			} \
C1_Done3: \
			{ \
				POPEDI \
				__asm mov dmasize, esi \
				POPESI \
			} \
			SAVE_ROW_REG; \
			return dmasize; \
		case 2: \
			{ \
				/*__asm inc dword ptr [g_nCounters+4]*/ \
				PUSHESI \
				PUSHEDI \
				__asm mov VIF_INC, incdest \
				__asm mov esi, dmasize \
				__asm cmp esi, (2*qsize) \
				/* move source and dest */ \
				__asm mov VIF_DST, dest \
				__asm mov VIF_SRC, data \
				__asm jl C2_Done3 \
			} \
C2_Unpack: \
			UNPACK_##name##SSE_2(0, 0, MaskType, ModeType); \
 \
			{ \
				__asm add VIF_DST, VIF_INC /* take into account wl */ \
				__asm cmp esi, (4*qsize) \
				__asm jl C2_Done2 \
				__asm sub esi, (2*qsize) \
				__asm jmp C2_Unpack /* unpack next */ \
			} \
C2_Done2: \
			{ \
				__asm sub esi, (2*qsize) \
			} \
C2_Done3: \
			{ \
				__asm cmp esi, qsize \
				__asm jl C2_Done4 \
			} \
			/* execute left over qw */ \
			UNPACK_##name##SSE_1(0, 0, MaskType, ModeType); \
			{ \
				__asm sub esi, qsize \
			} \
C2_Done4: \
			{ \
				POPEDI \
				__asm mov dmasize, esi \
				POPESI \
			} \
			SAVE_ROW_REG; \
			return dmasize; \
 \
		case 3: \
			{ \
				/*__asm inc dword ptr [g_nCounters+8]*/ \
				PUSHESI \
				PUSHEDI \
				__asm mov VIF_INC, incdest \
				__asm mov esi, dmasize \
				__asm cmp esi, (3*qsize) \
				/* move source and dest */ \
				__asm mov VIF_DST, dest \
				__asm mov VIF_SRC, data \
				__asm jl C3_Done5 \
			} \
C3_Unpack: \
			UNPACK_##name##SSE_3(0, 0, MaskType, ModeType); \
 \
			{ \
				__asm add VIF_DST, VIF_INC /* take into account wl */ \
				__asm cmp esi, (6*qsize) \
				__asm jl C3_Done2 \
				__asm sub esi, (3*qsize) \
				__asm jmp C3_Unpack /* unpack next */ \
			} \
C3_Done2: \
			{ \
				__asm sub esi, (3*qsize) \
			} \
C3_Done5: \
			{ \
				__asm cmp esi, qsize \
				__asm jl C3_Done4 \
				/* execute left over qw */ \
				__asm cmp esi, (2*qsize) \
				__asm jl C3_Done3 \
			} \
			\
			/* process 2 qws */ \
			UNPACK_##name##SSE_2(0, 0, MaskType, ModeType); \
			{ \
				__asm sub esi, (2*qsize) \
				__asm jmp C3_Done4 \
			} \
 \
C3_Done3: \
			/* process 1 qw */ \
			{ \
				__asm sub esi, qsize \
			} \
			UNPACK_##name##SSE_1(0, 0, MaskType, ModeType); \
C3_Done4: \
			{ \
				POPEDI \
				__asm mov dmasize, esi \
				POPESI \
			} \
			SAVE_ROW_REG; \
			return dmasize; \
 \
		default: /* >= 4 */ \
			tempcl = _vifRegs->cycle.wl-3; \
			{ \
				/*__asm inc dword ptr [g_nCounters+12]*/ \
				PUSHESI \
				PUSHEDI \
				__asm mov VIF_INC, tempcl \
				__asm mov s_saveinc, VIF_INC \
				__asm mov esi, dmasize \
				__asm cmp esi, qsize \
				__asm jl C4_Done \
				/* move source and dest */ \
				__asm mov VIF_DST, dest \
				__asm mov VIF_SRC, data \
			} \
C4_Unpack: \
			{ \
				__asm cmp esi, (3*qsize) \
				__asm jge C4_Unpack3 \
				__asm cmp esi, (2*qsize) \
				__asm jge C4_Unpack2 \
			} \
			UNPACK_##name##SSE_1(0, 0, MaskType, ModeType); \
			/* not enough data left */ \
			{ \
				__asm sub esi, qsize \
				__asm jmp C4_Done \
			} \
C4_Unpack2: \
			UNPACK_##name##SSE_2(0, 0, MaskType, ModeType); \
			/* not enough data left */ \
			{ \
				__asm sub esi, (2*qsize) \
				__asm jmp C4_Done \
			} \
C4_Unpack3: \
			UNPACK_##name##SSE_3(0, 0, MaskType, ModeType); \
			{ \
				__asm sub esi, (3*qsize) \
				/* more data left, process 1qw at a time */ \
				__asm mov VIF_INC, s_saveinc \
			} \
C4_UnpackX: \
			{ \
				/* check if any data left */ \
				__asm cmp esi, qsize \
				__asm jl C4_Done \
				\
			} \
			UNPACK_##name##SSE_1(3, 0, MaskType, ModeType); \
			{ \
				__asm sub esi, qsize \
				__asm cmp VIF_INC, 1 \
				__asm je C4_DoneLoop \
				__asm sub VIF_INC, 1 \
				__asm jmp C4_UnpackX \
			} \
C4_DoneLoop: \
			{ \
				__asm add VIF_DST, incdest /* take into account wl */ \
				__asm cmp esi, qsize \
				__asm jl C4_Done \
				__asm jmp C4_Unpack /* unpack next */ \
			} \
C4_Done: \
			{ \
				POPEDI \
				__asm mov dmasize, esi \
				POPESI \
			} \
			SAVE_ROW_REG; \
			return dmasize; \
	} \
 \
	return dmasize; \
} \

//{ \
//				/*__asm inc dword ptr [g_nCounters] \
//				__asm mov eax, dmasize \
//				__asm add dword ptr [g_nCounters+4], eax*/ \
//				PUSHESI \
//				PUSHEDI \
//				__asm mov esi, dmasize \
//			} \
//			UNPACK_Start_Setup_##MaskType##_SSE_##ModeType##(0); \
//			{ \
//				__asm cmp esi, qsize \
//				__asm jl C1_Done3 \
//				\
//				/* move source and dest */ \
//				__asm mov VIF_DST, dest \
//				__asm mov VIF_SRC, data \
//				__asm mov VIF_INC, incdest \
//				__asm cmp esi, (2*qsize) \
//				__asm jl C1_Unpack1 \
//				__asm cmp esi, (3*qsize) \
//				__asm jl C1_Unpack2 \
//				__asm imul VIF_INC, 3 \
//				__asm prefetchnta [eax + 192] \
//			} \
//C1_Unpack3: \
//			UNPACK_##name##SSE_3(0, 1, MaskType, ModeType); \
//			{ \
//				__asm add VIF_DST, VIF_INC /* take into account wl */ \
//				__asm cmp esi, (6*qsize) \
//				__asm jl C1_DoneUnpack3 \
//				__asm sub esi, (3*qsize) \
//				__asm jmp C1_Unpack3 \
//			} \
//C1_DoneUnpack3: \
//			{ \
//				__asm sub esi, (3*qsize) \
//				__asm mov VIF_INC, dword ptr [esp] /* restore old ptr */ \
//				__asm cmp esi, (2*qsize) \
//				__asm jl C1_Unpack1 \
//				/* fall through */ \
//			} \
//C1_Unpack2: \
//			UNPACK_##name##SSE_2(0, 1, MaskType, ModeType); \
//			{ \
//				__asm add VIF_DST, VIF_INC \
//				__asm add VIF_DST, VIF_INC \
//				__asm sub esi, (2*qsize) \
//				__asm jmp C1_Done3 \
//			} \
//C1_Unpack1: \
//			UNPACK_##name##SSE_1(0, 1, MaskType, ModeType); \
//			{ \
//				__asm add VIF_DST, VIF_INC /* take into account wl */ \
//				__asm sub esi, qsize \
//			} \
//C1_Done3: \
//			{ \
//				POPEDI \
//				__asm mov dmasize, esi \
//				POPESI \
//			} \
//			SAVE_ROW_REG; \

//while(size >= qsize) {
//	funcP( dest, (u32*)cdata, chans);
//	cdata += ft->qsize;
//	size -= ft->qsize;
//
//	if (vif->cl >= wl) {
//		dest += incdest;
//		vif->cl = 0;
//	}
//	else {
//		dest += 4;
//		vif->cl++;
//	}
//}

#define UNPACK_RIGHTSHIFT psrld
#define defUNPACK_SkippingWrite2(name, qsize) \
	defUNPACK_SkippingWrite(name, Regular, 0, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Regular, 1, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Regular, 2, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Mask, 0, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Mask, 1, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Mask, 2, qsize, u, SAVE_ROW_REG_BASE); \
	defUNPACK_SkippingWrite(name, WriteMask, 0, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, WriteMask, 1, qsize, u, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, WriteMask, 2, qsize, u, SAVE_ROW_REG_BASE); \

defUNPACK_SkippingWrite2(S_32, 4);
defUNPACK_SkippingWrite2(S_16, 2);
defUNPACK_SkippingWrite2(S_8, 1);
defUNPACK_SkippingWrite2(V2_32, 8);
defUNPACK_SkippingWrite2(V2_16, 4);
defUNPACK_SkippingWrite2(V2_8, 2);
defUNPACK_SkippingWrite2(V3_32, 12);
defUNPACK_SkippingWrite2(V3_16, 6);
defUNPACK_SkippingWrite2(V3_8, 3);
defUNPACK_SkippingWrite2(V4_32, 16);
defUNPACK_SkippingWrite2(V4_16, 8);
defUNPACK_SkippingWrite2(V4_8, 4);
defUNPACK_SkippingWrite2(V4_5, 2);

#undef UNPACK_RIGHTSHIFT
#undef defUNPACK_SkippingWrite2

#define UNPACK_RIGHTSHIFT psrad
#define defUNPACK_SkippingWrite2(name, qsize) \
	defUNPACK_SkippingWrite(name, Mask, 0, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Regular, 0, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Regular, 1, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Regular, 2, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Mask, 1, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, Mask, 2, qsize, s, SAVE_ROW_REG_BASE); \
	defUNPACK_SkippingWrite(name, WriteMask, 0, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, WriteMask, 1, qsize, s, SAVE_NO_REG); \
	defUNPACK_SkippingWrite(name, WriteMask, 2, qsize, s, SAVE_ROW_REG_BASE); \

defUNPACK_SkippingWrite2(S_16, 2);
defUNPACK_SkippingWrite2(S_8, 1);
defUNPACK_SkippingWrite2(V2_16, 4);
defUNPACK_SkippingWrite2(V2_8, 2);
defUNPACK_SkippingWrite2(V3_16, 6);
defUNPACK_SkippingWrite2(V3_8, 3);
defUNPACK_SkippingWrite2(V4_16, 8);
defUNPACK_SkippingWrite2(V4_8, 4);

#undef UNPACK_RIGHTSHIFT
#undef defUNPACK_SkippingWrite2

#endif

static int cycles;
extern int g_vifCycles;
static int vifqwc = 0;
int mfifoVIF1rbTransfer() {
	u32 maddr = psHu32(DMAC_RBOR);
	int msize = psHu32(DMAC_RBSR)+16, ret;
	u32 *src;

	/* Check if the transfer should wrap around the ring buffer */
	if ((vif1ch->madr+(vif1ch->qwc << 4)) >= (maddr+msize)) {
		int s1 = (maddr+msize) - vif1ch->madr;
		int s2 = (vif1ch->qwc << 4) - s1;

		/* it does, so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return -1;
		ret = VIF1transfer(src, s1/4, 0); 
		assert(ret == 0 ); // vif stall code not implemented
		
		/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
		vif1ch->madr = maddr;
		src = (u32*)PSM(maddr);
		if (src == NULL) return -1;
		ret = VIF1transfer(src, s2/4, 0); 
		assert(ret == 0 ); // vif stall code not implemented
	} else {
		/* it doesn't, so just transfer 'qwc*4' words 
		   from 'vif1ch->madr' to VIF1 */
		src = (u32*)PSM(vif1ch->madr);
		if (src == NULL) return -1;
		ret = VIF1transfer(src, vif1ch->qwc << 2, 0); 
		assert(ret == 0 ); // vif stall code not implemented
	}

	//vif1ch->madr+= (vif1ch->qwc << 4);
	

	return 0;
}

int mfifoVIF1chain() {
	u32 maddr = psHu32(DMAC_RBOR);
	int msize = psHu32(DMAC_RBSR)+16, ret;
	u32 *pMem;
	int mfifoqwc = vif1ch->qwc;

	/* Is QWC = 0? if so there is nothing to transfer */
	if (vif1ch->qwc == 0) return 0;

	if (vif1ch->madr >= maddr &&
		vif1ch->madr <= (maddr+msize)) {
		if (mfifoVIF1rbTransfer() == -1) return -1;
	} else {
		pMem = (u32*)dmaGetAddr(vif1ch->madr);
		if (pMem == NULL) return -1;
		ret = VIF1transfer(pMem, vif1ch->qwc << 2, 0); 
		assert(ret == 0 ); // vif stall code not implemented

		//vif1ch->madr+= (vif1ch->qwc << 4);
	}

	cycles+= (mfifoqwc) * BIAS; /* guessing */
	mfifoqwc = 0;
	//vif1ch->qwc = 0;
	if(vif1.vifstalled == 1) SysPrintf("Vif1 MFIFO stalls not implemented\n");
	vif1ch->madr = psHu32(DMAC_RBOR) + (vif1ch->madr & psHu32(DMAC_RBSR));
	return 0;
}

#define spr0 ((DMACh*)&PS2MEM_HW[0xD000])
static int tempqwc = 0;

void mfifoVIF1transfer(int qwc) {
	u32 *ptag;
	int id;
	int done = 0, ret;
	u32 temp = 0;
	cycles = 0;
	vifqwc += qwc;
	g_vifCycles = 0;
	if(vifqwc == 0) {
	//#ifdef PCSX2_DEVBUILD
				/*if( vifqwc > 1 )
					SysPrintf("vif mfifo tadr==madr but qwc = %d\n", vifqwc);*/
	//#endif
		
				//INT(10,50);
				return;
			}
	/*if ((psHu32(DMAC_CTRL) & 0xC0)) { 
			SysPrintf("DMA Stall Control %x\n",(psHu32(DMAC_CTRL) & 0xC0));
			}*/
#ifdef VIF_LOG
	VIF_LOG("mfifoVIF1transfer %x madr %x, tadr %x\n", vif1ch->chcr, vif1ch->madr, vif1ch->tadr);
#endif
	
 if((vif1ch->chcr & 0x100) == 0)SysPrintf("MFIFO VIF1 not ready!\n");
	//while (qwc > 0 && done == 0) {
	 if(vif1ch->qwc == 0){
			if(vif1ch->tadr == spr0->madr) {
	#ifdef PCSX2_DEVBUILD
				/*if( vifqwc > 1 )
					SysPrintf("vif mfifo tadr==madr but qwc = %d\n", vifqwc);*/
	#endif
				//hwDmacIrq(14);
				return;
			}
			vif1ch->tadr = psHu32(DMAC_RBOR) + (vif1ch->tadr & psHu32(DMAC_RBSR));
			ptag = (u32*)dmaGetAddr(vif1ch->tadr);
			
			id        = (ptag[0] >> 28) & 0x7;
			vif1ch->qwc  = (ptag[0] & 0xffff);
			vif1ch->madr = ptag[1];
			cycles += 2;
			
			
			if(vif1ch->chcr & 0x40) {
				ret = VIF1transfer(ptag+2, 2, 1);
				assert(ret == 0 ); // vif stall code not implemented
			}
			
			vif1ch->chcr = ( vif1ch->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 );
			
	#ifdef VIF_LOG
			VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x\n",
					ptag[1], ptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr, vifqwc, spr0->madr);
	#endif
			
			switch (id) {
				case 0: // Refe - Transfer Packet According to ADDR field
					if(vifqwc < vif1ch->qwc && (vif1ch->madr & ~psHu32(DMAC_RBSR)) == psHu32(DMAC_RBOR)) {
						return;
					}
					vif1ch->tadr += 16;
					vif1.done = 1;										//End Transfer
					break;

				case 1: // CNT - Transfer QWC following the tag.
					if(vifqwc < vif1ch->qwc && ((vif1ch->tadr + 16) & ~psHu32(DMAC_RBSR)) == psHu32(DMAC_RBOR)) {
						return;
					}
					vif1ch->madr = vif1ch->tadr + 16;						//Set MADR to QW after Tag            
					vif1ch->tadr = vif1ch->madr + (vif1ch->qwc << 4);			//Set TADR to QW following the data
					vif1.done = 0;
					break;

				case 2: // Next - Transfer QWC following tag. TADR = ADDR
					if(vifqwc < vif1ch->qwc && ((vif1ch->tadr + 16) & ~psHu32(DMAC_RBSR)) == psHu32(DMAC_RBOR)) {
						return;
					}
					temp = vif1ch->madr;								//Temporarily Store ADDR
					vif1ch->madr = vif1ch->tadr + 16; 					  //Set MADR to QW following the tag
					vif1ch->tadr = temp;								//Copy temporarily stored ADDR to Tag
					vif1.done = 0;
					break;

				case 3: // Ref - Transfer QWC from ADDR field
				case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
					if(vifqwc < vif1ch->qwc && (vif1ch->madr & ~psHu32(DMAC_RBSR)) == psHu32(DMAC_RBOR)) {
						return;
					}
					vif1ch->tadr += 16;									//Set TADR to next tag
					vif1.done = 0;
					break;

				case 7: // End - Transfer QWC following the tag
					if(vifqwc < vif1ch->qwc && ((vif1ch->tadr + 16) & ~psHu32(DMAC_RBSR)) == psHu32(DMAC_RBOR)) {
						return;
					}
					vif1ch->madr = vif1ch->tadr + 16;						//Set MADR to data following the tag
					vif1ch->tadr = vif1ch->madr + (vif1ch->qwc << 4);			//Set TADR to QW following the data
					vif1.done = 1;										//End Transfer
					break;
			}
			vifqwc--;
			
			//SysPrintf("VIF1 MFIFO qwc %d vif1 qwc %d, madr = %x, tadr = %x\n", qwc, vif1ch->qwc, vif1ch->madr, vif1ch->tadr);
			vif1ch->tadr = psHu32(DMAC_RBOR) + (vif1ch->tadr & psHu32(DMAC_RBSR));
			if((vif1ch->madr & ~psHu32(DMAC_RBSR)) == psHu32(DMAC_RBOR)){
				vif1ch->madr = psHu32(DMAC_RBOR) + (vif1ch->madr & psHu32(DMAC_RBSR));
				vifqwc -= vif1ch->qwc;
			}
	 }

		if (mfifoVIF1chain() == -1) {
			SysPrintf("dmaChain error %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
					ptag[1], ptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr);
			vif1.done = 1;
			INT(10,cycles+g_vifCycles);
		}
		
		if ((vif1ch->chcr & 0x80) && (ptag[0] >> 31)) {
#ifdef VIF_LOG
			VIF_LOG("dmaIrq Set\n");
#endif
			//SysPrintf("mfifoVIF1transfer: dmaIrq Set\n");
			//vifqwc = 0;
			vif1.done = 1;
		}

//		if( (cpuRegs.interrupt & (1<<1)) && qwc > 0) {
//			SysPrintf("vif1 mfifo interrupt %d\n", qwc);
//		}
	//}

	/*if(vif1.done == 1) {
		vifqwc = 0;
	}*/
	INT(10,cycles+g_vifCycles);
	if(vifqwc == 0 && tempqwc > 0) hwDmacIrq(14);
	
	//hwDmacIrq(1);
#ifdef SPR_LOG
	SPR_LOG("mfifoVIF1transfer end %x madr %x, tadr %x\n", vif1ch->chcr, vif1ch->madr, vif1ch->tadr);
#endif
}

int vifMFIFOInterrupt()
{
	
	if(!(vif1ch->chcr & 0x100)) return 1;
	else if(vifqwc == 0 && vif1.done == 0) return 1;

	if(vif1.done == 0 && vifqwc != 0) {
		mfifoVIF1transfer(0);
		if(vif1ch->qwc > 0) return 1;
		else return 0;
	}
	if(vif1.done == 0 || vif1ch->qwc > 0) {
		SysPrintf("Shouldnt go here\n");
		return 1;
	}
	vif1.done = 0;
	vif1ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF1);
//	vif1ch->chcr &= ~0x100;
//	vif1Regs->stat&= ~0x1F000000; // FQC=0
//	hwDmacIrq(DMAC_VIF1);
//
//	if (vif1.irq > 0) {
//		vif1.irq--;
//		hwIntcIrq(5); // VIF1 Intc
//	}
	return 1;
}
