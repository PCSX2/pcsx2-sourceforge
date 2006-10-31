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

#ifndef __IR5900SHIFT_H__
#define __IR5900SHIFT_H__

#include "Common.h"
#include "InterTables.h"

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/

void recSLL( void );
void recSRL( void );
void recSRA( void );
void recDSLL( void );
void recDSRL( void );
void recDSRA( void );
void recDSLL32( void );
void recDSRL32( void );
void recDSRA32( void );

void recSLLV( void );
void recSRLV( void );
void recSRAV( void );
void recDSLLV( void );
void recDSRLV( void );
void recDSRAV( void );

#endif
