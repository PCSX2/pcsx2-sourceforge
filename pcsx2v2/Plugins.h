/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#define PLUGINtypedefs
#define PLUGINfuncs
#include "PS2Edefs.h"

#ifdef PLUGINfuncs

// GS
_GSinit            GSinit;
_GSopen            GSopen;
_GSclose           GSclose;
_GSshutdown        GSshutdown;
_GSvsync           GSvsync;
_GSgifTransfer1    GSgifTransfer1;
_GSgifTransfer2    GSgifTransfer2;
_GSgifTransfer3    GSgifTransfer3;
_GSgetLastTag      GSgetLastTag;
_GSgifSoftReset    GSgifSoftReset;
_GSreadFIFO        GSreadFIFO;
_GSreadFIFO2       GSreadFIFO2;

_GSkeyEvent        GSkeyEvent;
_GSchangeSaveState GSchangeSaveState;
_GSmakeSnapshot	   GSmakeSnapshot;
_GSmakeSnapshot2   GSmakeSnapshot2;
_GSirqCallback 	   GSirqCallback;
_GSprintf      	   GSprintf;
_GSsetBaseMem 	   GSsetBaseMem;
_GSsetGameCRC		GSsetGameCRC;
_GSsetFrameSkip	   GSsetFrameSkip;
_GSsetupRecording GSsetupRecording;
_GSreset		   GSreset;
_GSwriteCSR		   GSwriteCSR;
_GSgetDriverInfo   GSgetDriverInfo;
#ifdef _WIN32
_GSsetWindowInfo   GSsetWindowInfo;
#endif
_GSfreeze          GSfreeze;
_GSconfigure       GSconfigure;
_GStest            GStest;
_GSabout           GSabout;

// PAD1
_PADinit           PAD1init;
_PADopen           PAD1open;
_PADclose          PAD1close;
_PADshutdown       PAD1shutdown;
_PADkeyEvent       PAD1keyEvent;
_PADstartPoll      PAD1startPoll;
_PADpoll           PAD1poll;
_PADquery          PAD1query;
_PADupdate         PAD1update;

_PADgsDriverInfo   PAD1gsDriverInfo;
_PADconfigure      PAD1configure;
_PADtest           PAD1test;
_PADabout          PAD1about;

// PAD2
_PADinit           PAD2init;
_PADopen           PAD2open;
_PADclose          PAD2close;
_PADshutdown       PAD2shutdown;
_PADkeyEvent       PAD2keyEvent;
_PADstartPoll      PAD2startPoll;
_PADpoll           PAD2poll;
_PADquery          PAD2query;
_PADupdate         PAD2update;

_PADgsDriverInfo   PAD2gsDriverInfo;
_PADconfigure      PAD2configure;
_PADtest           PAD2test;
_PADabout          PAD2about;

// SIO[2]
_SIOinit           SIOinit[2][9];
_SIOopen           SIOopen[2][9];
_SIOclose          SIOclose[2][9];
_SIOshutdown       SIOshutdown[2][9];
_SIOstartPoll      SIOstartPoll[2][9];
_SIOpoll           SIOpoll[2][9];
_SIOquery          SIOquery[2][9];

_SIOconfigure      SIOconfigure[2][9];
_SIOtest           SIOtest[2][9];
_SIOabout          SIOabout[2][9];

// SPU2
_SPU2init          SPU2init;
_SPU2open          SPU2open;
_SPU2close         SPU2close;
_SPU2shutdown      SPU2shutdown;
_SPU2write         SPU2write;
_SPU2read          SPU2read;
_SPU2readDMA4Mem   SPU2readDMA4Mem;
_SPU2writeDMA4Mem  SPU2writeDMA4Mem;
_SPU2interruptDMA4 SPU2interruptDMA4;
_SPU2readDMA7Mem   SPU2readDMA7Mem;
_SPU2writeDMA7Mem  SPU2writeDMA7Mem;
_SPU2setDMABaseAddr SPU2setDMABaseAddr;
_SPU2interruptDMA7 SPU2interruptDMA7;
_SPU2ReadMemAddr   SPU2ReadMemAddr;
_SPU2setupRecording SPU2setupRecording;
_SPU2WriteMemAddr   SPU2WriteMemAddr;
_SPU2irqCallback   SPU2irqCallback;

_SPU2setClockPtr   SPU2setClockPtr;

_SPU2async         SPU2async;
_SPU2freeze        SPU2freeze;
_SPU2configure     SPU2configure;
_SPU2test          SPU2test;
_SPU2about         SPU2about;

// CDVD
_CDVDinit          CDVDinit;
_CDVDopen          CDVDopen;
_CDVDclose         CDVDclose;
_CDVDshutdown      CDVDshutdown;
_CDVDreadTrack     CDVDreadTrack;
_CDVDgetBuffer     CDVDgetBuffer;
_CDVDreadSubQ      CDVDreadSubQ;
_CDVDgetTN         CDVDgetTN;
_CDVDgetTD         CDVDgetTD;
_CDVDgetTOC        CDVDgetTOC;
_CDVDgetDiskType   CDVDgetDiskType;
_CDVDgetTrayStatus CDVDgetTrayStatus;
_CDVDctrlTrayOpen  CDVDctrlTrayOpen;
_CDVDctrlTrayClose CDVDctrlTrayClose;

_CDVDconfigure     CDVDconfigure;
_CDVDtest          CDVDtest;
_CDVDabout         CDVDabout;
_CDVDnewDiskCB     CDVDnewDiskCB;

// DEV9
_DEV9init          DEV9init;
_DEV9open          DEV9open;
_DEV9close         DEV9close;
_DEV9shutdown      DEV9shutdown;
_DEV9read8         DEV9read8;
_DEV9read16        DEV9read16;
_DEV9read32        DEV9read32;
_DEV9write8        DEV9write8;
_DEV9write16       DEV9write16;
_DEV9write32       DEV9write32;
_DEV9readDMA8Mem   DEV9readDMA8Mem;
_DEV9writeDMA8Mem  DEV9writeDMA8Mem;
_DEV9irqCallback   DEV9irqCallback;
_DEV9irqHandler    DEV9irqHandler;

_DEV9configure     DEV9configure;
_DEV9freeze        DEV9freeze;
_DEV9test          DEV9test;
_DEV9about         DEV9about;

// USB
_USBinit           USBinit;
_USBopen           USBopen;
_USBclose          USBclose;
_USBshutdown       USBshutdown;
_USBread8          USBread8;
_USBread16         USBread16;
_USBread32         USBread32;
_USBwrite8         USBwrite8;
_USBwrite16        USBwrite16;
_USBwrite32        USBwrite32;
_USBasync          USBasync;

_USBirqCallback    USBirqCallback;
_USBirqHandler     USBirqHandler;
_USBsetRAM         USBsetRAM;

_USBconfigure      USBconfigure;
_USBfreeze         USBfreeze;
_USBtest           USBtest;
_USBabout          USBabout;

// FW
_FWinit            FWinit;
_FWopen            FWopen;
_FWclose           FWclose;
_FWshutdown        FWshutdown;
_FWread32          FWread32;
_FWwrite32         FWwrite32;
_FWirqCallback     FWirqCallback;

_FWconfigure       FWconfigure;
_FWfreeze          FWfreeze;
_FWtest            FWtest;
_FWabout           FWabout;
#endif

int  LoadPlugins();
void ReleasePlugins();
int  OpenPlugins(const char* pTitleFilename);
void ClosePlugins();
void ResetPlugins();

#endif /* __PLUGINS_H__ */
