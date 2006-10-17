#ifndef __PS2EDEFS_H__
#define __PS2EDEFS_H__

/*
 *  PS2E Definitions v0.2.8 (beta)
 *
 *  Author: linuzappz@pcsx.net
 */

/*
 Notes: 
 * Since this is still beta things may change.

 * OSflags:
	__LINUX__ (linux OS)
	__WIN32__ (win32 OS)

 * common return values (for ie. GSinit):
	 0 - success
	-1 - error

 * reserved keys:
	F1 to F10 are reserved for the emulator

 * plugins should NOT change the current 
   working directory.
   (on win32, add flag OFN_NOCHANGEDIR for
    GetOpenFileName)

*/

#include "PS2Etypes.h"

#ifdef __LINUX__
#define CALLBACK
#else
#include <windows.h>
#endif

/* common defines */

#if defined(GSdefs)  || defined(PADdefs) || \
	defined(SPU2defs)|| defined(CDVDdefs)
#define COMMONdefs
#endif

// PS2EgetLibType returns 
#define PS2E_LT_GS   0x1
#define PS2E_LT_PAD  0x2
#define PS2E_LT_SPU2 0x4
#define PS2E_LT_CDVD 0x8

#ifdef COMMONdefs

u32   CALLBACK PS2EgetLibType(void);
u32   CALLBACK PS2EgetLibVersion(void);
char* CALLBACK PS2EgetLibName(void);

#endif

// key values:
/* key values must be OS dependant:
	win32: the VK_XXX will be used (WinUser)
	linux: the XK_XXX will be used (XFree86)
*/

// event values:
#define KEYPRESS	1
#define KEYRELEASE	2

typedef struct {
	u32 key;
	u32 event;
} keyEvent;

typedef struct { // NOT bcd coded
	u8 minute;
	u8 second;
	u8 frame;
} cdvdLoc;

typedef struct {
	u8 strack;
	u8 etrack;
} cdvdTN;

/* GS plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef GSdefs

// basic funcs

s32  CALLBACK GSinit();
s32  CALLBACK GSopen(void *pDsp, char *Title);
void CALLBACK GSclose();
void CALLBACK GSshutdown();
void CALLBACK GSvsync();
void CALLBACK GSgifTransfer(u32 *pMem, u32 size);
void CALLBACK GSgifTransfer2(u32 *pMem);
void CALLBACK GSwrite32(u32 mem, u32 value);
void CALLBACK GSwrite64(u32 mem, u64 value);
u32  CALLBACK GSread32(u32 mem);
u64  CALLBACK GSread64(u32 mem);

// extended funcs

// GSkeyEvent gets called when there is a keyEvent from the PAD plugin
void CALLBACK GSkeyEvent(keyEvent *ev);
void CALLBACK GSmakeSnapshot(char *path);
void CALLBACK GSconfigure();
void CALLBACK GSabout();
s32  CALLBACK GStest();

#endif

/* PAD plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef PADdefs

// basic funcs

s32  CALLBACK PADinit(u32 flags);
s32  CALLBACK PADopen(void *pDsp);
void CALLBACK PADclose();
void CALLBACK PADshutdown();
// PADkeyEvent is called every vsync (return NULL if no event)
keyEvent* CALLBACK PADkeyEvent();
u8   CALLBACK PADstartPoll(int pad);
u8   CALLBACK PADpoll(u8 value);
// returns: 1 if supported pad1
//			2 if supported pad2
//			3 if both are supported
u32  CALLBACK PADquery();

// extended funcs

void CALLBACK PADconfigure();
void CALLBACK PADabout();
s32  CALLBACK PADtest();

#endif

/* SPU2 plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef SPU2defs

// basic funcs

s32  CALLBACK SPU2init();
s32  CALLBACK SPU2open(void *pDsp);
void CALLBACK SPU2close();
void CALLBACK SPU2shutdown();
void CALLBACK SPU2update();
void CALLBACK SPU2dma(u32 *dmaAddr, char *pRam);
void CALLBACK SPU2write(u32 mem, u16 value);
u16  CALLBACK SPU2read(u32 mem);

// extended funcs

void CALLBACK SPU2configure();
void CALLBACK SPU2about();
s32  CALLBACK SPU2test();

#endif

/* CDVD plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef CDVDdefs

// basic funcs

s32  CALLBACK CDVDinit();
s32  CALLBACK CDVDopen(void *pDsp);
void CALLBACK CDVDclose();
void CALLBACK CDVDshutdown();
s32  CALLBACK CDVDreadTrack(cdvdLoc *Time);

// return can be NULL (for async modes)
u8*  CALLBACK CDVDgetBuffer();

s32  CALLBACK CDVDgetTN(cdvdTN *Buffer);
s32  CALLBACK CDVDgetTD(u8 Track, cdvdLoc *Buffer);

// extended funcs

void CALLBACK CDVDconfigure();
void CALLBACK CDVDabout();
s32  CALLBACK CDVDtest();

#endif

// might be useful for emulators
#ifdef PLUGINtypedefs

typedef u32  (CALLBACK* _PS2EgetLibType)(void);
typedef u32  (CALLBACK* _PS2EgetLibVersion)(void);
typedef char*(CALLBACK* _PS2EgetLibName)(void);

// GS
typedef s32  (CALLBACK* _GSinit)();
typedef s32  (CALLBACK* _GSopen)(void *pDsp, char *Title);
typedef void (CALLBACK* _GSclose)();
typedef void (CALLBACK* _GSshutdown)();
typedef void (CALLBACK* _GSvsync)();
typedef void (CALLBACK* _GSwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _GSwrite64)(u32 mem, u64 value);
typedef u32  (CALLBACK* _GSread32)(u32 mem);
typedef u64  (CALLBACK* _GSread64)(u32 mem);
typedef void (CALLBACK* _GSgifTransfer)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSgifTransfer2)(u32 *pMem);
typedef void (CALLBACK* _GSkeyEvent)(keyEvent* ev);

typedef void (CALLBACK* _GSmakeSnapshot)(char *path);
typedef void (CALLBACK* _GSconfigure)();
typedef s32  (CALLBACK* _GStest)();
typedef void (CALLBACK* _GSabout)();

// PAD
typedef s32  (CALLBACK* _PADinit)(u32 flags);
typedef s32  (CALLBACK* _PADopen)(void *pDsp);
typedef void (CALLBACK* _PADclose)();
typedef void (CALLBACK* _PADshutdown)();
typedef keyEvent* (CALLBACK* _PADkeyEvent)();
typedef u8   (CALLBACK* _PADstartPoll)(int pad);
typedef u8   (CALLBACK* _PADpoll)(u8 value);
typedef u32  (CALLBACK* _PADquery)();

typedef void (CALLBACK* _PADconfigure)();
typedef s32  (CALLBACK* _PADtest)();
typedef void (CALLBACK* _PADabout)();

// SPU2
typedef s32  (CALLBACK* _SPU2init)();
typedef s32  (CALLBACK* _SPU2open)(void *pDsp);
typedef void (CALLBACK* _SPU2close)();
typedef void (CALLBACK* _SPU2shutdown)();
typedef void (CALLBACK* _SPU2update)();
typedef void (CALLBACK* _SPU2dma)(u32 *dmaAddr, char *pRam);
typedef void (CALLBACK* _SPU2write)(u32 mem, u16 value);
typedef u16  (CALLBACK* _SPU2read)(u32 mem);

typedef void (CALLBACK* _SPU2configure)();
typedef s32  (CALLBACK* _SPU2test)();
typedef void (CALLBACK* _SPU2about)();

// CDVD
typedef s32  (CALLBACK* _CDVDinit)();
typedef s32  (CALLBACK* _CDVDopen)(void *pDsp);
typedef void (CALLBACK* _CDVDclose)();
typedef void (CALLBACK* _CDVDshutdown)();
typedef s32  (CALLBACK* _CDVDreadTrack)(cdvdLoc *Time);
typedef u8*  (CALLBACK* _CDVDgetBuffer)();
typedef s32  (CALLBACK* _CDVDgetTN)(cdvdTN *Buffer);
typedef s32  (CALLBACK* _CDVDgetTD)(u8 Track, cdvdLoc *Buffer);

typedef void (CALLBACK* _CDVDconfigure)();
typedef s32  (CALLBACK* _CDVDtest)();
typedef void (CALLBACK* _CDVDabout)();

#endif

#ifdef PLUGINfuncs

// GS
_GSinit         GSinit;
_GSopen         GSopen;
_GSclose        GSclose;
_GSshutdown     GSshutdown;
_GSvsync        GSvsync;
_GSwrite32      GSwrite32;
_GSwrite64      GSwrite64;
_GSread32       GSread32;
_GSread64       GSread64;
_GSgifTransfer  GSgifTransfer;
_GSgifTransfer2 GSgifTransfer2;

_GSkeyEvent     GSkeyEvent;
_GSmakeSnapshot	GSmakeSnapshot;
_GSconfigure    GSconfigure;
_GStest         GStest;
_GSabout        GSabout;

// PAD1
_PADinit        PAD1init;
_PADopen        PAD1open;
_PADclose       PAD1close;
_PADshutdown    PAD1shutdown;
_PADkeyEvent    PAD1keyEvent;
_PADstartPoll   PAD1startPoll;
_PADpoll        PAD1poll;
_PADquery       PAD1query;

_PADconfigure   PAD1configure;
_PADtest        PAD1test;
_PADabout       PAD1about;

// PAD2
_PADinit        PAD2init;
_PADopen        PAD2open;
_PADclose       PAD2close;
_PADshutdown    PAD2shutdown;
_PADkeyEvent    PAD2keyEvent;
_PADstartPoll   PAD2startPoll;
_PADpoll        PAD2poll;
_PADquery       PAD2query;

_PADconfigure   PAD2configure;
_PADtest        PAD2test;
_PADabout       PAD2about;

// SPU2
_SPU2init       SPU2init;
_SPU2open       SPU2open;
_SPU2close      SPU2close;
_SPU2shutdown   SPU2shutdown;
_SPU2update     SPU2update;
_SPU2dma        SPU2dma;
_SPU2write      SPU2write;
_SPU2read       SPU2read;

_SPU2configure  SPU2configure;
_SPU2test       SPU2test;
_SPU2about      SPU2about;

// CDVD
_CDVDinit       CDVDinit;
_CDVDopen       CDVDopen;
_CDVDclose      CDVDclose;
_CDVDshutdown   CDVDshutdown;
_CDVDreadTrack  CDVDreadTrack;
_CDVDgetBuffer  CDVDgetBuffer;
_CDVDgetTN      CDVDgetTN;
_CDVDgetTD      CDVDgetTD;

_CDVDconfigure  CDVDconfigure;
_CDVDtest       CDVDtest;
_CDVDabout      CDVDabout;

#endif

#endif /* __PS2EDEFS_H__ */
