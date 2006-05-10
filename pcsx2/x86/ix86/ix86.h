/*
 * ix86 definitions v0.6.2
 *  Authors: linuzappz <linuzappz@pcsx.net>
 *           alexey silinov
 *           goldfinger
 *           shadow < shadow@pcsx2.net >
 */

#ifndef __IX86_H__
#define __IX86_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "PS2Etypes.h"       // Basic types header

#define SIB 4
#define DISP32 5

// general types
typedef enum
{
   EAX = 0,
   EBX = 3,
   ECX = 1,
   EDX = 2,
   ESI = 6,
   EDI = 7,
   EBP = 5,
   ESP = 4,

   RAX = 0,
   RBX = 3,
   RCX = 1,
   RDX = 2,
   RSI = 6,
   RDI = 7,
   RBP = 5,
   RSP = 4,
   R8  = 8, 
   R9  = 9, 
   R10 =10, 
   R11 =11,
   R12 =12,
   R13 =13,
   R14 =14, 
   R15 =15

} x86IntRegType;

typedef enum
{
   MM0 = 0,
   MM1 = 1,
   MM2 = 2,
   MM3 = 3,
   MM4 = 4,
   MM5 = 5,
   MM6 = 6,
   MM7 = 7
} x86MMXRegType;

typedef enum
{
   XMM0 = 0,
   XMM1 = 1,
   XMM2 = 2,
   XMM3 = 3,
   XMM4 = 4,
   XMM5 = 5,
   XMM6 = 6,
   XMM7 = 7,
   XMM8 = 8,
   XMM9 = 9,
   XMM10=10,
   XMM11=11,
   XMM12=12,
   XMM13=13,
   XMM14=14,
   XMM15=15

} x86SSERegType;


void cpudetectInit( void );//this is all that needs to be called and will fill up the below structs

//cpu capabilities structure
typedef struct {
   u32 hasFloatingPointUnit;
   u32 hasVirtual8086ModeEnhancements;
   u32 hasDebuggingExtensions;
   u32 hasPageSizeExtensions;
   u32 hasTimeStampCounter;
   u32 hasModelSpecificRegisters;
   u32 hasPhysicalAddressExtension;
   u32 hasCOMPXCHG8BInstruction;
   u32 hasAdvancedProgrammableInterruptController;
   u32 hasSEPFastSystemCall;
   u32 hasMemoryTypeRangeRegisters;
   u32 hasPTEGlobalFlag;
   u32 hasMachineCheckArchitecture;
   u32 hasConditionalMoveAndCompareInstructions;
   u32 hasFGPageAttributeTable;
   u32 has36bitPageSizeExtension;
   u32 hasProcessorSerialNumber;
   u32 hasCFLUSHInstruction;
   u32 hasDebugStore;
   u32 hasACPIThermalMonitorAndClockControl;
   u32 hasMultimediaExtensions;
   u32 hasFastStreamingSIMDExtensionsSaveRestore;
   u32 hasStreamingSIMDExtensions;
   u32 hasStreamingSIMD2Extensions;
   u32 hasSelfSnoop;
   u32 hasHyperThreading;
   u32 hasThermalMonitor;
   u32 hasIntel64BitArchitecture;
   u32 hasStreamingSIMD3Extensions;
   //that is only for AMDs
   u32 hasMultimediaExtensionsExt;
   u32 hasAMD64BitArchitecture;
   u32 has3DNOWInstructionExtensionsExt;
   u32 has3DNOWInstructionExtensions;
} CAPABILITIES;

extern CAPABILITIES cpucaps;

typedef struct {
   
   u32 x86Family;	   // Processor Family
   u32 x86Model;	   // Processor Model
   u32 x86PType;	   // Processor Type
   u32 x86StepID;	   // Stepping ID
   u32 x86Flags;	   // Feature Flags
   u32 x86EFlags;	   // Extended Feature Flags
   //all the above returns hex values
   s8  x86ID[16];	   // Vendor ID  //the vendor creator (in %s)
   s8  x86Type[20];   //cpu type in char format //the cpu type (in %s)
   s8  x86Fam[50];    // family in char format //the original cpu name string (in %s)
   u32 cpuspeed;      // speed of cpu //this will give cpu speed (in %d)
} CPUINFO;

extern CPUINFO cpuinfo;

extern s8  *x86Ptr;
extern u8  *j8Ptr[32];
extern u32 *j32Ptr[32];


#ifdef __x86_64__
#define MEMADDR(addr, oplen)	((addr) - ((u64)x86Ptr + ((u64)oplen)))
#else
#define MEMADDR(addr, oplen)	(addr)
#endif



void write8( int val );
void write16( int val );
void write32( u32 val );
void write64( u64 val );


void x86SetPtr( char *ptr );
void x86Shutdown( void );

void x86SetJ8( u8 *j8 );
void x86SetJ32( u32 *j32 );
void x86Align( int bytes );
u64 GetCPUTick( void );

// General Helper functions

void Rex( int w, int r, int x, int b );
void ModRM( int mod, int rm, int reg );
void SibSB( int ss, int rm, int index );
void SET8R( int cc, int to );
u8* J8Rel( int cc, int to );
u32* J32Rel( int cc, u32 to );
void CMOV32RtoR( int cc, int to, int from );
void CMOV32MtoR( int cc, int to, u32 from );

//******************
// IX86 intructions 
//******************

//
// * scale values:
// *  0 - *1
// *  1 - *2
// *  2 - *4
// *  3 - *8
// 

void STC( void );
void CLC( void );

////////////////////////////////////
// mov instructions               //
////////////////////////////////////

// mov r64 to r64 
void MOV64RtoR( x86IntRegType to, x86IntRegType from );
// mov r64 to m64 
void MOV64RtoM( u64 to, x86IntRegType from );
// mov m64 to r64 
void MOV64MtoR( x86IntRegType to, u64 from );
// mov imm32 to m64 
void MOV64ItoM( u32 to, u32 from );

// mov r32 to r32 
void MOV32RtoR( x86IntRegType to, x86IntRegType from );
// mov r32 to m32 
void MOV32RtoM( u32 to, x86IntRegType from );
// mov m32 to r32 
void MOV32MtoR( x86IntRegType to, u32 from );
// mov [r32] to r32 
void MOV32RmtoR( x86IntRegType to, x86IntRegType from );
void MOV32RmtoROffset( x86IntRegType to, x86IntRegType from, u32 offset );
// mov [r32][r32*scale] to r32 
void MOV32RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale );
// mov r32 to [r32] 
void MOV32RtoRm( x86IntRegType to, x86IntRegType from );
// mov r32 to [r32][r32*scale]
void MOV32RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale );
// mov imm32 to r32 
void MOV32ItoR( x86IntRegType to, u32 from );
// mov imm32 to m32 
void MOV32ItoM( u32 to, u32 from );

// mov r16 to m16 
void MOV16RtoM( u32 to, x86IntRegType from );
// mov m16 to r16 
void MOV16MtoR( x86IntRegType to, u32 from );
// mov [r32] to r16
void MOV16RmtoR( x86IntRegType to, x86IntRegType from ) ;
void MOV16RmtoROffset( x86IntRegType to, x86IntRegType from, u32 offset );
// mov r16 to [r32]
void MOV16RtoRm(x86IntRegType to, x86IntRegType from);
// mov imm16 to m16 
void MOV16ItoM( u32 to, u16 from );
/* mov r16 to [r32][r32*scale] */
void MOV16RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale);

// mov r8 to m8 
void MOV8RtoM( u32 to, x86IntRegType from );
// mov m8 to r8 
void MOV8MtoR( x86IntRegType to, u32 from );
// mov [r32] to r8
void MOV8RmtoR(x86IntRegType to, x86IntRegType from);
void MOV8RmtoROffset(x86IntRegType to, x86IntRegType from, u32 offset);
// mov r8 to [r32]
void MOV8RtoRm(x86IntRegType to, x86IntRegType from);
// mov imm8 to m8 
void MOV8ItoM( u32 to, u8 from );

// movsx r8 to r32 
void MOVSX32R8toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm8toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm8toROffset( x86IntRegType to, x86IntRegType from, u32 offset );
// movsx m8 to r32 
void MOVSX32M8toR( x86IntRegType to, u32 from );
// movsx r16 to r32 
void MOVSX32R16toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm16toR( x86IntRegType to, x86IntRegType from );
void MOVSX32Rm16toROffset( x86IntRegType to, x86IntRegType from, u32 offset );
// movsx m16 to r32 
void MOVSX32M16toR( x86IntRegType to, u32 from );

// movzx r8 to r32 
void MOVZX32R8toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm8toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm8toROffset( x86IntRegType to, x86IntRegType from, u32 offset );
// movzx m8 to r32 
void MOVZX32M8toR( x86IntRegType to, u32 from );
// movzx r16 to r32 
void MOVZX32R16toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm16toR( x86IntRegType to, x86IntRegType from );
void MOVZX32Rm16toROffset( x86IntRegType to, x86IntRegType from, u32 offset );
// movzx m16 to r32 
void MOVZX32M16toR( x86IntRegType to, u32 from );

// cmovbe r32 to r32 
void CMOVBE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovbe m32 to r32
void CMOVBE32MtoR( x86IntRegType to, u32 from );
// cmovb r32 to r32 
void CMOVB32RtoR( x86IntRegType to, x86IntRegType from );
// cmovb m32 to r32
void CMOVB32MtoR( x86IntRegType to, u32 from );
// cmovae r32 to r32 
void CMOVAE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovae m32 to r32
void CMOVAE32MtoR( x86IntRegType to, u32 from );
// cmova r32 to r32 
void CMOVA32RtoR( x86IntRegType to, x86IntRegType from );
// cmova m32 to r32
void CMOVA32MtoR( x86IntRegType to, u32 from );

// cmovo r32 to r32 
void CMOVO32RtoR( x86IntRegType to, x86IntRegType from );
// cmovo m32 to r32
void CMOVO32MtoR( x86IntRegType to, u32 from );
// cmovp r32 to r32 
void CMOVP32RtoR( x86IntRegType to, x86IntRegType from );
// cmovp m32 to r32
void CMOVP32MtoR( x86IntRegType to, u32 from );
// cmovs r32 to r32 
void CMOVS32RtoR( x86IntRegType to, x86IntRegType from );
// cmovs m32 to r32
void CMOVS32MtoR( x86IntRegType to, u32 from );
// cmovno r32 to r32 
void CMOVNO32RtoR( x86IntRegType to, x86IntRegType from );
// cmovno m32 to r32
void CMOVNO32MtoR( x86IntRegType to, u32 from );
// cmovnp r32 to r32 
void CMOVNP32RtoR( x86IntRegType to, x86IntRegType from );
// cmovnp m32 to r32
void CMOVNP32MtoR( x86IntRegType to, u32 from );
// cmovns r32 to r32 
void CMOVNS32RtoR( x86IntRegType to, x86IntRegType from );
// cmovns m32 to r32
void CMOVNS32MtoR( x86IntRegType to, u32 from );

// cmovne r32 to r32 
void CMOVNE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovne m32 to r32
void CMOVNE32MtoR( x86IntRegType to, u32 from );
// cmove r32 to r32
void CMOVE32RtoR( x86IntRegType to, x86IntRegType from );
// cmove m32 to r32
void CMOVE32MtoR( x86IntRegType to, u32 from );
// cmovg r32 to r32
void CMOVG32RtoR( x86IntRegType to, x86IntRegType from );
// cmovg m32 to r32
void CMOVG32MtoR( x86IntRegType to, u32 from );
// cmovge r32 to r32
void CMOVGE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovge m32 to r32
void CMOVGE32MtoR( x86IntRegType to, u32 from );
// cmovl r32 to r32
void CMOVL32RtoR( x86IntRegType to, x86IntRegType from );
// cmovl m32 to r32
void CMOVL32MtoR( x86IntRegType to, u32 from );
// cmovle r32 to r32
void CMOVLE32RtoR( x86IntRegType to, x86IntRegType from );
// cmovle m32 to r32
void CMOVLE32MtoR( x86IntRegType to, u32 from );

////////////////////////////////////
// arithmetic instructions        //
////////////////////////////////////

// add imm32 to r64 
void ADD64ItoR( x86IntRegType to, u32 from );
// add m64 to r64 
void ADD64MtoR( x86IntRegType to, u32 from );

// add imm32 to r32 
void ADD32ItoR( x86IntRegType to, u32 from );
// add imm32 to m32 
void ADD32ItoM( u32 to, u32 from );
// add r32 to r32 
void ADD32RtoR( x86IntRegType to, x86IntRegType from );
// add r32 to m32 
void ADD32RtoM( u32 to, x86IntRegType from );
// add m32 to r32 
void ADD32MtoR( x86IntRegType to, u32 from );

// add imm16 to r16 
void ADD16ItoR( x86IntRegType to, u16 from );
// add imm16 to m16 
void ADD16ItoM( u32 to, u16 from );
// add r16 to m16 
void ADD16RtoM( u32 to, x86IntRegType from );
// add m16 to r16 
void ADD16MtoR( x86IntRegType to, u32 from );

// adc imm32 to r32 
void ADC32ItoR( x86IntRegType to, u32 from );
// adc imm32 to m32 
void ADC32ItoM( u32 to, u32 from );
// adc r32 to r32 
void ADC32RtoR( x86IntRegType to, x86IntRegType from );
// adc m32 to r32 
void ADC32MtoR( x86IntRegType to, u32 from );

// inc r32 
void INC32R( x86IntRegType to );
// inc m32 
void INC32M( u32 to );
// inc r16 
void INC16R( x86IntRegType to );
// inc m16 
void INC16M( u32 to );

// sub m64 to r64 
void SUB64MtoR( x86IntRegType to, u32 from );

// sub imm32 to r32 
void SUB32ItoR( x86IntRegType to, u32 from );
// sub imm32 to m32
void SUB32ItoM( u32 to, u32 from ) ;
// sub r32 to r32 
void SUB32RtoR( x86IntRegType to, x86IntRegType from );
// sub m32 to r32 
void SUB32MtoR( x86IntRegType to, u32 from );
// sub imm16 to r16 
void SUB16ItoR( x86IntRegType to, u16 from );
// sub imm16 to m16
void SUB16ItoM( u32 to, u16 from ) ;
// sub m16 to r16 
void SUB16MtoR( x86IntRegType to, u32 from );

// sbb r64 to r64 
void SBB64RtoR( x86IntRegType to, x86IntRegType from );

// sbb imm32 to r32 
void SBB32ItoR( x86IntRegType to, u32 from );
// sbb imm32 to m32 
void SBB32ItoM( u32 to, u32 from );
// sbb r32 to r32 
void SBB32RtoR( x86IntRegType to, x86IntRegType from );
// sbb m32 to r32 
void SBB32MtoR( x86IntRegType to, u32 from );

// dec r32 
void DEC32R( x86IntRegType to );
// dec m32 
void DEC32M( u32 to );
// dec r16 
void DEC16R( x86IntRegType to );
// dec m16 
void DEC16M( u32 to );

// mul eax by r32 to edx:eax 
void MUL32R( x86IntRegType from );
// mul eax by m32 to edx:eax 
void MUL32M( u32 from );

// imul eax by r32 to edx:eax 
void IMUL32R( x86IntRegType from );
// imul eax by m32 to edx:eax 
void IMUL32M( u32 from );
// imul r32 by r32 to r32 
void IMUL32RtoR( x86IntRegType to, x86IntRegType from );

// div eax by r32 to edx:eax 
void DIV32R( x86IntRegType from );
// div eax by m32 to edx:eax 
void DIV32M( u32 from );

// idiv eax by r32 to edx:eax 
void IDIV32R( x86IntRegType from );
// idiv eax by m32 to edx:eax 
void IDIV32M( u32 from );

////////////////////////////////////
// shifting instructions          //
////////////////////////////////////

// shl imm8 to r64 
void SHL64ItoR( x86IntRegType to, u8 from );
// shl cl to r64
void SHL64CLtoR( x86IntRegType to );
// shr imm8 to r64 
void SHR64ItoR( x86IntRegType to, u8 from );
// shr cl to r64
void SHR64CLtoR( x86IntRegType to );
// sar imm8 to r64 
void SAR64ItoR( x86IntRegType to, u8 from );
// sar cl to r64
void SAR64CLtoR( x86IntRegType to );

// shl imm8 to r32 
void SHL32ItoR( x86IntRegType to, u8 from );
/* shl imm8 to m32 */
void SHL32ItoM( u32 to, u8 from );
// shl cl to r32 
void SHL32CLtoR( x86IntRegType to );

// shr imm8 to r32 
void SHR32ItoR( x86IntRegType to, u8 from );
/* shr imm8 to m32 */
void SHR32ItoM( u32 to, u8 from );
// shr cl to r32 
void SHR32CLtoR( x86IntRegType to );

// sar imm8 to r32 
void SAR32ItoR( x86IntRegType to, u8 from );
// sar imm8 to m32 
void SAR32ItoM( u32 to, u8 from );
// sar cl to r32 
void SAR32CLtoR( x86IntRegType to );

void RCR32ItoR( x86IntRegType to,u8 from );
// shld imm8 to r32
void SHLD32ItoR( u32 to, u32 from, u8 shift );
// shrd imm8 to r32
void SHRD32ItoR( u32 to, u32 from, u8 shift );

// sal imm8 to r32 
#define SAL32ItoR SHL32ItoR
// sal cl to r32 
#define SAL32CLtoR SHL32CLtoR

// logical instructions

// or imm32 to r64 
void OR64ItoR( x86IntRegType to, u32 from );
// or m64 to r64 
void OR64MtoR( x86IntRegType to, u32 from );

// or imm32 to r32 
void OR32ItoR( x86IntRegType to, u32 from );
// or imm32 to m32 
void OR32ItoM( u32 to, u32 from );
// or r32 to r32 
void OR32RtoR( x86IntRegType to, x86IntRegType from );
// or r32 to m32 
void OR32RtoM( u32 to, x86IntRegType from );
// or m32 to r32 
void OR32MtoR( x86IntRegType to, u32 from );
// or m16 to r16 
void OR16MtoR( x86IntRegType to, u32 from );

// xor imm32 to r64 
void XOR64ItoR( x86IntRegType to, u32 from );
// xor r64 to r64 
void XOR64RtoR( x86IntRegType to, x86IntRegType from );
// xor m64 to r64
void XOR64MtoR( x86IntRegType to, u32 from );

// xor imm32 to r32 
void XOR32ItoR( x86IntRegType to, u32 from );
// xor imm32 to m32 
void XOR32ItoM( u32 to, u32 from );
// xor r32 to r32 
void XOR32RtoR( x86IntRegType to, x86IntRegType from );
// xor r16 to r16 
void XOR16RtoR( x86IntRegType to, x86IntRegType from );
// xor r32 to m32 
void XOR32RtoM( u32 to, x86IntRegType from );
// xor m32 to r32 
void XOR32MtoR( x86IntRegType to, u32 from );

// and imm32 to r64 
void AND64ItoR( x86IntRegType to, u32 from );
// and m64 to r64
void AND64MtoR( x86IntRegType to, u32 from );
// and r64 to m64
void AND64RtoM( x86IntRegType to, u32 from );

// and imm32 to r32 
void AND32ItoR( x86IntRegType to, u32 from );
// and imm32 to m32 
void AND32ItoM( u32 to, u32 from );
// and r32 to r32 
void AND32RtoR( x86IntRegType to, x86IntRegType from );
// and r32 to m32 
void AND32RtoM( u32 to, x86IntRegType from );
// and m32 to r32 
void AND32MtoR( x86IntRegType to, u32 from );
// and r16 to m16
void AND16RtoM( u32 to, x86IntRegType from );
// and m16 to r16 
void AND16MtoR( x86IntRegType to, u32 from );

// not r64 
void NOT64R( x86IntRegType from );
// not r32 
void NOT32R( x86IntRegType from );
// neg r64 
void NEG64R( x86IntRegType from );
// neg r32 
void NEG32R( x86IntRegType from );
// neg r16 
void NEG16R( x86IntRegType from );

////////////////////////////////////
// jump instructions              //
////////////////////////////////////

// jmp rel8 
u8*  JMP8( u8 to );

// jmp rel32 
u32* JMP32( u32 to );
// jmp r32 
void JMP32R( x86IntRegType to );

// jp rel8 
u8*  JP8( u8 to );
// jnp rel8 
u8*  JNP8( u8 to );
// je rel8 
u8*  JE8( u8 to );
// jz rel8 
u8*  JZ8( u8 to );
// jg rel8 
u8*  JG8( u8 to );
// jge rel8 
u8*  JGE8( u8 to );
// js rel8 
u8*  JS8( u8 to );
// jns rel8 
u8*  JNS8( u8 to );
// jl rel8 
u8*  JL8( u8 to );
// ja rel8 
u8*  JA8( u8 to );
// jae rel8 
u8*  JAE8( u8 to );
// jb rel8 
u8*  JB8( u8 to );
// jbe rel8 
u8*  JBE8( u8 to );
// jle rel8 
u8*  JLE8( u8 to );
// jne rel8 
u8*  JNE8( u8 to );
// jnz rel8 
u8*  JNZ8( u8 to );
// jng rel8 
u8*  JNG8( u8 to );
// jnge rel8 
u8*  JNGE8( u8 to );
// jnl rel8 
u8*  JNL8( u8 to );
// jnle rel8 
u8*  JNLE8( u8 to );
// jo rel8 
u8*  JO8( u8 to );
// jno rel8 
u8*  JNO8( u8 to );

// je rel32 
u32* JE32( u32 to );
// jz rel32 
u32* JZ32( u32 to );
// jg rel32 
u32* JG32( u32 to );
// jge rel32 
u32* JGE32( u32 to );
// jl rel32 
u32* JL32( u32 to );
// jle rel32 
u32* JLE32( u32 to );
// jne rel32 
u32* JNE32( u32 to );
// jnz rel32 
u32* JNZ32( u32 to );
// jng rel32 
u32* JNG32( u32 to );
// jnge rel32 
u32* JNGE32( u32 to );
// jnl rel32 
u32* JNL32( u32 to );
// jnle rel32 
u32* JNLE32( u32 to );
// jo rel32 
u32* JO32( u32 to );
// jno rel32 
u32* JNO32( u32 to );

// call func 
void CALLFunc( u32 func); // based on CALL32
// call rel32 
void CALL32( u32 to );
// call r32 
void CALL32R( x86IntRegType to );
// call m32 
void CALL32M( u32 to );

////////////////////////////////////
// misc instructions              //
////////////////////////////////////

// cmp imm32 to r64 
void CMP64ItoR( x86IntRegType to, u32 from );
// cmp m64 to r64 
void CMP64MtoR( x86IntRegType to, u32 from );

// cmp imm32 to r32 
void CMP32ItoR( x86IntRegType to, u32 from );
// cmp imm32 to m32 
void CMP32ItoM( u32 to, u32 from );
// cmp r32 to r32 
void CMP32RtoR( x86IntRegType to, x86IntRegType from );
// cmp m32 to r32 
void CMP32MtoR( x86IntRegType to, u32 from );
// cmp imm8 to [r32]
void CMP32ItoRm( x86IntRegType to, u8 from);

// cmp imm16 to r16 
void CMP16ItoR( x86IntRegType to, u16 from );
// cmp imm16 to m16 
void CMP16ItoM( u32 to, u16 from );
// cmp r16 to r16 
void CMP16RtoR( x86IntRegType to, x86IntRegType from );
// cmp m16 to r16 
void CMP16MtoR( x86IntRegType to, u32 from );

// test imm32 to r32 
void TEST32ItoR( x86IntRegType to, u32 from );
// test r32 to r32 
void TEST32RtoR( x86IntRegType to, x86IntRegType from );

// sets r8 
void SETS8R( x86IntRegType to );
// setl r8 
void SETL8R( x86IntRegType to );
// setb r8 
void SETB8R( x86IntRegType to );
// setnz r8 
void SETNZ8R( x86IntRegType to );

// cbw 
void CBW( void );
// cwd 
void CWD( void );
// cdq 
void CDQ( void );

// push r32 
void PUSH32R( x86IntRegType from );
// push m32 
void PUSH32M( u32 from );
// push imm32 
void PUSH32I( u32 from );
// pop r32 
void POP32R( x86IntRegType from );
// pushad 
void PUSHA32( void );
// popad 
void POPA32( void );
// pushfd 
void PUSHFD( void );
// popfd 
void POPFD( void );
// ret 
void RET( void );

void BT32ItoR( x86IntRegType to, x86IntRegType from );
void BSRRtoR(x86IntRegType to, x86IntRegType from);
void BSWAP32R( x86IntRegType to );
//******************
// FPU instructions 
//******************

// fild m32 to fpu reg stack 
void FILD32( u32 from );
// fistp m32 from fpu reg stack 
void FISTP32( u32 from );
// fld m32 to fpu reg stack 
void FLD32( u32 from );
// fst m32 from fpu reg stack 
void FST32( u32 to );
// fstp m32 from fpu reg stack 
void FSTP32( u32 to );

// fldcw fpu control word from m16 
void FLDCW( u32 from );
// fstcw fpu control word to m16 
void FNSTCW( u32 to );

// fadd ST(src) to fpu reg stack ST(0) 
void FADD32Rto0( x86IntRegType src );
// fadd ST(0) to fpu reg stack ST(src) 
void FADD320toR( x86IntRegType src );
// fsub ST(src) to fpu reg stack ST(0) 
void FSUB32Rto0( x86IntRegType src );
// fsub ST(0) to fpu reg stack ST(src) 
void FSUB320toR( x86IntRegType src );
// fsubp -> subtract ST(0) from ST(1), store in ST(1) and POP stack 
void FSUBP( void );
// fmul ST(src) to fpu reg stack ST(0) 
void FMUL32Rto0( x86IntRegType src );
// fmul ST(0) to fpu reg stack ST(src) 
void FMUL320toR( x86IntRegType src );
// fdiv ST(src) to fpu reg stack ST(0) 
void FDIV32Rto0( x86IntRegType src );
// fdiv ST(0) to fpu reg stack ST(src) 
void FDIV320toR( x86IntRegType src );

// fadd m32 to fpu reg stack 
void FADD32( u32 from );
// fsub m32 to fpu reg stack 
void FSUB32( u32 from );
// fmul m32 to fpu reg stack 
void FMUL32( u32 from );
// fdiv m32 to fpu reg stack 
void FDIV32( u32 from );
// fcomi st, st( i) 
void FCOMI( x86IntRegType src );
// fcomip st, st( i) 
void FCOMIP( x86IntRegType src );
// fucomi st, st( i) 
void FUCOMI( x86IntRegType src );
// fucomip st, st( i) 
void FUCOMIP( x86IntRegType src );
// fcom m32 to fpu reg stack 
void FCOM32( u32 from );
// fabs fpu reg stack 
void FABS( void );
// fsqrt fpu reg stack 
void FSQRT( void );
// fchs fpu reg stack 
void FCHS( void );

// fcmovb fpu reg to fpu reg stack 
void FCMOVB32( x86IntRegType from );
// fcmove fpu reg to fpu reg stack 
void FCMOVE32( x86IntRegType from );
// fcmovbe fpu reg to fpu reg stack 
void FCMOVBE32( x86IntRegType from );
// fcmovu fpu reg to fpu reg stack 
void FCMOVU32( x86IntRegType from );
// fcmovnb fpu reg to fpu reg stack 
void FCMOVNB32( x86IntRegType from );
// fcmovne fpu reg to fpu reg stack 
void FCMOVNE32( x86IntRegType from );
// fcmovnbe fpu reg to fpu reg stack 
void FCMOVNBE32( x86IntRegType from );
// fcmovnu fpu reg to fpu reg stack 
void FCMOVNU32( x86IntRegType from );
void FCOMP32( u32 from );
void FNSTSWtoAX( void );

//******************
// MMX instructions 
//******************

// r64 = mm

// movq m64 to r64 
void MOVQMtoR( x86MMXRegType to, u32 from );
// movq r64 to m64 
void MOVQRtoM( u32 to, x86MMXRegType from );

// pand r64 to r64 
void PANDRtoR( x86MMXRegType to, x86MMXRegType from );
void PANDNRtoR( x86MMXRegType to, x86MMXRegType from );
// pand m64 to r64 ;
void PANDMtoR( x86MMXRegType to, u32 from );
// pandn r64 to r64 
void PANDNRtoR( x86MMXRegType to, x86MMXRegType from );
// pandn r64 to r64 
void PANDNMtoR( x86MMXRegType to, u32 from );
// por r64 to r64 
void PORRtoR( x86MMXRegType to, x86MMXRegType from );
// por m64 to r64 
void PORMtoR( x86MMXRegType to, u32 from );
// pxor r64 to r64 
void PXORRtoR( x86MMXRegType to, x86MMXRegType from );
// pxor m64 to r64 
void PXORMtoR( x86MMXRegType to, u32 from );

// psllq r64 to r64 
void PSLLQRtoR( x86MMXRegType to, x86MMXRegType from );
// psllq m64 to r64 
void PSLLQMtoR( x86MMXRegType to, u32 from );
// psllq imm8 to r64 
void PSLLQItoR( x86MMXRegType to, u8 from );
// psrlq r64 to r64 
void PSRLQRtoR( x86MMXRegType to, x86MMXRegType from );
// psrlq m64 to r64 
void PSRLQMtoR( x86MMXRegType to, u32 from );
// psrlq imm8 to r64 
void PSRLQItoR( x86MMXRegType to, u8 from );

// paddusb r64 to r64 
void PADDUSBRtoR( x86MMXRegType to, x86MMXRegType from );
// paddusb m64 to r64 
void PADDUSBMtoR( x86MMXRegType to, u32 from );
// paddusw r64 to r64 
void PADDUSWRtoR( x86MMXRegType to, x86MMXRegType from );
// paddusw m64 to r64 
void PADDUSWMtoR( x86MMXRegType to, u32 from );

// paddb r64 to r64 
void PADDBRtoR( x86MMXRegType to, x86MMXRegType from );
// paddb m64 to r64 
void PADDBMtoR( x86MMXRegType to, u32 from );
// paddw r64 to r64 
void PADDWRtoR( x86MMXRegType to, x86MMXRegType from );
// paddw m64 to r64 
void PADDWMtoR( x86MMXRegType to, u32 from );
// paddd r64 to r64 
void PADDDRtoR( x86MMXRegType to, x86MMXRegType from );
// paddd m64 to r64 
void PADDDMtoR( x86MMXRegType to, u32 from );
void PADDSBRtoR( x86MMXRegType to, x86MMXRegType from );
void PADDSWRtoR( x86MMXRegType to, x86MMXRegType from );

void PSUBSBRtoR( x86MMXRegType to, x86MMXRegType from ); 
void PSUBSWRtoR( x86MMXRegType to, x86MMXRegType from );

void PSUBBRtoR( x86MMXRegType to, x86MMXRegType from );
void PSUBWRtoR( x86MMXRegType to, x86MMXRegType from );
void PSUBDRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from );
void PCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from );
void PSRLWItoR( x86MMXRegType to, u8 from );
void PSRLDItoR( x86MMXRegType to, u8 from );
void PSLLWItoR( x86MMXRegType to, u8 from );
void PSLLDItoR( x86MMXRegType to, u8 from );
void PSRAWItoR( x86MMXRegType to, u8 from );
void PSRADItoR( x86MMXRegType to, u8 from );
void PUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from );
void PUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from );
void MOVQ64ItoR( x86MMXRegType reg, u64 i ); //Prototype.Todo add all consts to end of block.not after jr $+8
void MOVQRtoR( x86MMXRegType to, x86MMXRegType from );
void MOVDMtoMMX( x86MMXRegType to, u32 from );
void MOVDMMXtoM( u32 to, x86MMXRegType from );
void MOVD32RtoMMX( x86MMXRegType to, x86IntRegType from );
void MOVD64MMXtoR( x86IntRegType to, x86MMXRegType from );
// emms 
void EMMS( void );

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word 64bits
//**********************************************************************************
void PACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from);
void PACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from);

void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from);

//*********************
// SSE   instructions *
//*********************
void SSE_MOVAPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_MOVAPS_XMM_to_M128( u32 to, x86SSERegType from );
void SSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_MOVSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from );
void SSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from );

void SSE_MOVLPS_M64_to_XMM( x86SSERegType to, u32 from );
void SSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from );
void SSE_MOVHPS_M64_to_XMM( x86SSERegType to, u32 from );
void SSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from );       
void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MOVLPSRmtoR( x86MMXRegType to, x86IntRegType from );
void SSE_MOVLPSRmtoROffset( x86MMXRegType to, x86IntRegType from, u32 offset );
void SSE_MOVLPSRtoRm( x86MMXRegType to, x86IntRegType from );

void SSE_MOVAPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVAPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVAPSRtoRm( x86IntRegType to, x86IntRegType from );
void SSE_MOVAPSRmtoR( x86IntRegType to, x86IntRegType from );
void SSE_MOVAPSRmtoROffset( x86IntRegType to, x86IntRegType from, u32 offset );
void SSE_MOVUPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVUPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale );
void SSE_MOVUPSRtoRm( x86IntRegType to, x86IntRegType from );
void SSE_MOVUPSRmtoR( x86IntRegType to, x86IntRegType from );

void SSE_RCPPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RCPPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_ORPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_ORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_XORPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_XORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ANDPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_ANDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ANDNPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_ANDNPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ADDPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_ADDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_ADDSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_ADDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SUBPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_SUBPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SUBSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_SUBSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MULPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_MULPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MULSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_MULSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPEQSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPEQSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLTSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLESS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPUNORDSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPUNORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNESS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPNESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLTSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPNLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLESS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPNLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPORDSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from );
void SSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from );
void SSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, u32 from );
void SSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from );
void SSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, u32 from );
void SSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from );
void SSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MAXPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_MAXPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MAXSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_MAXSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MINPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_MINPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_MINSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_MINSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RSQRTPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_RSQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_RSQRTSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_RSQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SQRTPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_SQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SQRTSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_SQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE_SHUFPS_M128_to_XMM( x86SSERegType to, u32 from, u8 imm8 );
void SSE_CMPEQPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPEQPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLTPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPLEPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPUNORDPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPUNORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNEPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPNEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLTPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPNLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPNLEPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPNLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_CMPORDPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_CMPORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_DIVPS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE_DIVPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE_DIVSS_M32_to_XMM( x86SSERegType to, u32 from );
void SSE_DIVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2_PSHUFD_M128_to_XMM( x86SSERegType to, u32 from, u8 imm8 );

void SSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, u32 from, u8 imm8 );
void SSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, u32 from, u8 imm8 );

void SSE_STMXCSR( u32 from );
void SSE_LDMXCSR( u32 from );


//*********************
//  SSE 2 Instructions*
//*********************
void SSE2_MOVDQA_M128_to_XMM(x86SSERegType to, u32 from); 
void SSE2_MOVDQA_XMM_to_M128( u32 to, x86SSERegType from); 
/*
void SSE2_PSRLW_I8_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSRLD_I8_to_XMM(x86SSERegType to, u32 from); 
void SSE2_PSRAW_I8_to_XMM(x86SSERegType to, u32 from); 
void SSE2_PSRAD_I8_to_XMM(x86SSERegType to, u32 from); 
void SSE2_PSLLW_I8_to_XMM(x86SSERegType to, u32 from); 
void SSE2_PSLLD_I8_to_XMM(x86SSERegType to, u32 from);
void SSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
*/
void SSE2_PSRLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRLW_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSRLW_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRLD_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSRLD_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRLQ_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSRLQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRLDQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRAW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRAW_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSRAW_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSRAD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSRAD_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSRAD_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSLLW_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSLLW_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSLLD_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSLLD_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PSLLQ_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PSLLQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PSLLDQ_I8_to_XMM(x86SSERegType to, u8 imm8);
void SSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMAXSW_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PMAXUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMAXUB_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMINSW_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PMINUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PMINUB_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDSB_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDSW_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PSUBSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSUBSB_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PSUBSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PSUBSW_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PXOR_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PADDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDW_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PADDUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDUSB_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PADDUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_PADDUSW_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2_PADDB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDB_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PADDD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PADDD_M128_to_XMM(x86SSERegType to, u32 from );

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word
//**********************************************************************************
void SSE2_PACKSSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PACKSSWB_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PACKSSDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PACKSSDW_M128_to_XMM(x86SSERegType to, u32 from);

void SSE2_PACKUSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PACKUSWB_M128_to_XMM(x86SSERegType to, u32 from);

//**********************************************************************************/
//PUNPCKHWD: Unpack 16bit high
//**********************************************************************************
void SSE2_PUNPCKLBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLBW_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PUNPCKHBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHBW_M128_to_XMM(x86SSERegType to, u32 from);

void SSE2_PUNPCKLWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLWD_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PUNPCKHWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHWD_M128_to_XMM(x86SSERegType to, u32 from);

void SSE2_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, u32 from);

void SSE2_PUNPCKLQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PUNPCKLQDQ_M128_to_XMM(x86SSERegType to, u32 from);

// mult by half words
void SSE2_PMULLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PMULLW_M128_to_XMM(x86SSERegType to, u32 from);
void SSE2_PMULHW_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PMULHW_M128_to_XMM(x86SSERegType to, u32 from);

void SSE2_PMULUDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from);
void SSE2_PMULUDQ_M128_to_XMM(x86SSERegType to, u32 from);


//**********************************************************************************/
//PMOVMSKB: Create 16bit mask from signs of 8bit integers
//**********************************************************************************
void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from);

//**********************************************************************************/
//PEXTRW,PINSRW: Packed Extract/Insert Word                                        *
//**********************************************************************************
void SSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 );
void SSE_PINSRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 );


//**********************************************************************************/
//PSUBx: Subtract Packed Integers                                                  *
//**********************************************************************************
void SSE2_PSUBB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBB_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PSUBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBW_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PSUBD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBD_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PSUBQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PSUBQ_M128_to_XMM(x86SSERegType to, u32 from );
///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PCMPxx: Compare Packed Integers                                                  *
//**********************************************************************************
void SSE2_PCMPGTB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPGTB_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PCMPGTW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPGTW_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PCMPGTD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPGTD_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PCMPEQB_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPEQB_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PCMPEQW_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPEQW_M128_to_XMM(x86SSERegType to, u32 from );
void SSE2_PCMPEQD_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
void SSE2_PCMPEQD_M128_to_XMM(x86SSERegType to, u32 from );
////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVD: Move Dword(32bit) to /from XMM reg                                         *
//**********************************************************************************
void SSE2_MOVD_M32_to_XMM( x86SSERegType to, u32 from );
void SSE2_MOVD_R32_to_XMM( x86SSERegType to, x86IntRegType from );
void SSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
void SSE2_MOVD_XMM_to_R32( x86IntRegType to, x86SSERegType from );
////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//POR : SSE Bitwise OR                                                             *
//**********************************************************************************
void SSE2_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2_POR_M128_to_XMM( x86SSERegType to, u32 from );
/////////////////////////////////////////////////////////////////////////////////////
//*********************
// 3DNOW instructions * 
//*********************
void FEMMS( void );
void PFCMPEQMtoR( x86IntRegType to, u32 from );
void PFCMPGTMtoR( x86IntRegType to, u32 from );
void PFCMPGEMtoR( x86IntRegType to, u32 from );
void PFADDMtoR( x86IntRegType to, u32 from );
void PFADDRtoR( x86IntRegType to, x86IntRegType from );
void PFSUBMtoR( x86IntRegType to, u32 from );
void PFSUBRtoR( x86IntRegType to, x86IntRegType from );
void PFMULMtoR( x86IntRegType to, u32 from );
void PFMULRtoR( x86IntRegType to, x86IntRegType from );
void PFRCPMtoR( x86IntRegType to, u32 from );
void PFRCPRtoR( x86IntRegType to, x86IntRegType from );
void PFRCPIT1RtoR( x86IntRegType to, x86IntRegType from );
void PFRCPIT2RtoR( x86IntRegType to, x86IntRegType from );
void PFRSQRTRtoR( x86IntRegType to, x86IntRegType from );
void PFRSQIT1RtoR( x86IntRegType to, x86IntRegType from );
void PF2IDMtoR( x86IntRegType to, u32 from );
void PF2IDRtoR( x86IntRegType to, x86IntRegType from );
void PI2FDMtoR( x86IntRegType to, u32 from );
void PI2FDRtoR( x86IntRegType to, x86IntRegType from );
void PFMAXMtoR( x86IntRegType to, u32 from );
void PFMAXRtoR( x86IntRegType to, x86IntRegType from );
void PFMINMtoR( x86IntRegType to, u32 from );
void PFMINRtoR( x86IntRegType to, x86IntRegType from );

/* SSE2 emulated functions for SSE CPU's by kekko*/

void SSE2EMU_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 );
void SSE2EMU_MOVD_XMM_to_R32( x86IntRegType to, x86SSERegType from );
void SSE2EMU_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
void SSE2EMU_CVTDQ2PS_M128_to_XMM( x86SSERegType to, u32 from );
void SSE2EMU_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
void SSE2EMU_MOVD_R32_to_XMM( x86SSERegType to, x86IntRegType from );

////////////////////////////////////////////////////
#ifdef _DEBUG
#define WRITECHECK() CheckX86Ptr()
#else
#define WRITECHECK()
#endif

#define write8(val )  {  \
	*(u8*)x86Ptr = (u8)val; \
	x86Ptr++; \
} \

#define write16(val ) \
{ \
	*(u16*)x86Ptr = (u16)val; \
	x86Ptr += 2;  \
} \

#define write32( val ) \
{ \
	*(u32*)x86Ptr = val; \
	x86Ptr += 4; \
} \

#ifdef __cplusplus
}
#endif

#endif // __IX86_H__
