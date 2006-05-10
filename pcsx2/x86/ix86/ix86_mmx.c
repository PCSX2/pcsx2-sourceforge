
#include "ix86.h"

/********************/
/* MMX instructions */
/********************/

// r64 = mm

/* movq m64 to r64 */
void MOVQMtoR( x86MMXRegType to, u32 from )
{
	write16( 0x6F0F );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* movq r64 to m64 */
void MOVQRtoM( u32 to, x86MMXRegType from ) 
{
	write16( 0x7F0F );
	ModRM( 0, from, DISP32 );
	write32(MEMADDR(to, 4)); 
}

/* pand r64 to r64 */
void PANDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xDB0F );
	ModRM( 3, to, from ); 
}

/* por r64 to r64 */
void PORRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xEB0F );
	ModRM( 3, to, from ); 
}

/* pxor r64 to r64 */
void PXORRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xEF0F );
	ModRM( 3, to, from ); 
}

/* psllq r64 to r64 */
void PSLLQRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xF30F );
	ModRM( 3, to, from ); 
}

/* psllq m64 to r64 */
void PSLLQMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xF30F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

/* psllq imm8 to r64 */
void PSLLQItoR( x86MMXRegType to, u8 from ) 
{
	write16( 0x730F ); 
	ModRM( 3, 6, to); 
	write8( from ); 
}

/* psrlq r64 to r64 */
void PSRLQRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xD30F ); 
	ModRM( 3, to, from ); 
}

/* psrlq m64 to r64 */
void PSRLQMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xD30F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

/* psrlq imm8 to r64 */
void PSRLQItoR( x86MMXRegType to, u8 from ) 
{
	write16( 0x730F );
	ModRM( 3, 2, to); 
	write8( from ); 
}

/* paddusb r64 to r64 */
void PADDUSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xDC0F ); 
	ModRM( 3, to, from ); 
}

/* paddusb m64 to r64 */
void PADDUSBMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xDC0F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

/* paddusw r64 to r64 */
void PADDUSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xDD0F ); 
	ModRM( 3, to, from ); 
}

/* paddusw m64 to r64 */
void PADDUSWMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xDD0F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

/* paddb r64 to r64 */
void PADDBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xFC0F ); 
	ModRM( 3, to, from ); 
}

/* paddb m64 to r64 */
void PADDBMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xFC0F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

/* paddw r64 to r64 */
void PADDWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xFD0F ); 
	ModRM( 3, to, from ); 
}

/* paddw m64 to r64 */
void PADDWMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xFD0F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

/* paddd r64 to r64 */
void PADDDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xFE0F ); 
	ModRM( 3, to, from ); 
}

/* paddd m64 to r64 */
void PADDDMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xFE0F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

/* emms */
void EMMS( void ) 
{
	write16( 0x770F );
}

void PADDSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xEC0F ); 
	ModRM( 3, to, from ); 
}

void PADDSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xED0F );
	ModRM( 3, to, from ); 
}



void PSUBSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xE80F ); 
	ModRM( 3, to, from ); 
}

void PSUBSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xE90F );
	ModRM( 3, to, from ); 
}


void PSUBBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xF80F ); 
	ModRM( 3, to, from ); 
}

void PSUBWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xF90F ); 
	ModRM( 3, to, from ); 
}

void PSUBDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xFA0F ); 
	ModRM( 3, to, from ); 
}

void PSUBUSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xD80F ); 
	ModRM( 3, to, from ); 
}

void PSUBUSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0xD90F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x740F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x750F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x760F ); 
	ModRM( 3, to, from ); 
}

void PCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x640F ); 
	ModRM( 3, to, from ); 
}

void PCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x650F ); 
	ModRM( 3, to, from ); 
}

void PCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x660F ); 
	ModRM( 3, to, from ); 
}

void PSRLWItoR( x86MMXRegType to, u8 from )
{
	write16( 0x710F );
	ModRM( 3, 2 , to ); 
	write8( from );
}

void PSRLDItoR( x86MMXRegType to, u8 from )
{
	write16( 0x720F );
	ModRM( 3, 2 , to ); 
	write8( from );
}

void PSLLWItoR( x86MMXRegType to, u8 from )
{
	write16( 0x710F );
	ModRM( 3, 6 , to ); 
	write8( from );
}

void PSLLDItoR( x86MMXRegType to, u8 from )
{
	write16( 0x720F );
	ModRM( 3, 6 , to ); 
	write8( from );
}

void PSRAWItoR( x86MMXRegType to, u8 from )
{
	write16( 0x710F );
	ModRM( 3, 4 , to ); 
	write8( from );
}

void PSRADItoR( x86MMXRegType to, u8 from )
{
	write16( 0x720F );
	ModRM( 3, 4 , to ); 
	write8( from );
}

/* por m64 to r64 */
void PORMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xEB0F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

/* pxor m64 to r64 */
void PXORMtoR( x86MMXRegType to, u32 from ) 
{
	write16( 0xEF0F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

/* pand m64 to r64 */
void PANDMtoR( x86MMXRegType to, u32 from ) 
{
	//u64 rip = (u64)x86Ptr + 7;
	write16( 0xDB0F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

void PANDNMtoR( x86MMXRegType to, u32 from )
{
	write16( 0xDF0F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

void PUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x6A0F );
	ModRM( 3, to, from );
}

void PUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x620F );
	ModRM( 3, to, from );
}

void MOVQ64ItoR( x86MMXRegType reg, u64 i ) 
{
	MOVQMtoR( reg, ( u32 )(x86Ptr) + 2 + 7 );
	JMP8( 8 );
	write64( i );
}

void MOVQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x6F0F );
	ModRM( 3, to, from );
}

/* movd m32 to r64 */
void MOVDMtoMMX( x86MMXRegType to, u32 from ) 
{
	write16( 0x6E0F );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* movq r64 to m32 */
void MOVDMMXtoM( u32 to, x86MMXRegType from ) 
{
	write16( 0x7E0F );
	ModRM( 0, from, DISP32 );
	write32( MEMADDR(to, 4) ); 
}

/* movd r32 to r64 */
void MOVD32MMXtoMMX( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0x6E0F );
	ModRM( 3, to, from );
}

/* movq r64 to r32 */
void MOVD64MMXtoMMX( x86MMXRegType to, x86MMXRegType from ) 
{
	write16( 0x7E0F );
	ModRM( 3, from, to );
}

// untested
void PACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	write16( 0x630F );
	ModRM( 3, to, from ); 
}

void PACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	write16( 0x6B0F );
	ModRM( 3, to, from ); 
}

void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from)
{
	write16( 0xD70F ); 
	ModRM( 3, to, from );
}
