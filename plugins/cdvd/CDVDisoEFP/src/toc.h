/*  toc.h
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#ifndef TOC_H
#define TOC_H


// #ifndef __LINUX__
// #ifdef __linux__
// #define __LINUX__
// #endif /* __linux__ */
// #endif /* No __LINUX__ */

// #define CDVDdefs
// #include "PS2Edefs.h"

#include "isofile.h"


// #define VERBOSE_FUNCTION_TOC


#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */

struct tocTD {
  unsigned long lsn;
  unsigned char type;
#ifdef _WIN32
};
#else
} __attribute__ ((packed));
#endif /* _WIN32 */

struct tocTN {
  unsigned char strack;
  unsigned char etrack;
#ifdef _WIN32
};
#else
} __attribute__ ((packed));
#endif /* _WIN32 */

#ifdef _WIN32
#pragma pack()
#endif /* _WIN32 */


// PCSX2's .toc file format:
// 1 unsigned char - CDVD_TYPE_????
// 1 tocTN
// As many tocTDs as it takes.


extern void IsoInitTOC(struct IsoFile *isofile);
extern void IsoAddTNToTOC(struct IsoFile *isofile, struct tocTN toctn);
extern void IsoAddTDToTOC(struct IsoFile *isofile,
                          unsigned char track,
                          struct tocTD toctd);

extern int IsoLoadTOC(struct IsoFile *isofile);
extern int IsoSaveTOC(struct IsoFile *isofile);


#endif /* TOC_H */
