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

#ifndef __IVUMICRO_H__
#define __IVUMICRO_H__


#define RECOMPILE_VUMI_ABS
#define RECOMPILE_VUMI_SUB
#define RECOMPILE_VUMI_SUBA
#define RECOMPILE_VUMI_MADD
#define RECOMPILE_VUMI_MADDA
#define RECOMPILE_VUMI_MSUB
#define RECOMPILE_VUMI_MSUBA

#define RECOMPILE_VUMI_ADD
#define RECOMPILE_VUMI_ADDA
#define RECOMPILE_VUMI_MUL
#define RECOMPILE_VUMI_MULA
#define RECOMPILE_VUMI_MAX
#define RECOMPILE_VUMI_MINI
#define RECOMPILE_VUMI_FTOI

#define RECOMPILE_VUMI_MATH
#define RECOMPILE_VUMI_MISC
#define RECOMPILE_VUMI_E
#define RECOMPILE_VUMI_X
#define RECOMPILE_VUMI_RANDOM
#define RECOMPILE_VUMI_FLAG
#define RECOMPILE_VUMI_BRANCH
#define RECOMPILE_VUMI_ARITHMETIC
#define RECOMPILE_VUMI_LOADSTORE

#ifdef __x86_64__
#undef RECOMPILE_VUMI_X
#endif


#define MODE_READ	0x1
#define MODE_WRITE	0x2

void recResetFlags( void );
void recVUFlushFloatRegister( void );
void recUpdateFlags( void );

void _recvuTestPipes(VURegs * VU);
void _recvuFlushFDIV(VURegs * VU);
void _recvuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
void _recvuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn);
void _recvuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
void _recvuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn);

// save on mem
typedef struct {
	int cycle;
	int cycles;
	u8 statusflag;
	u8 macflag;
	u8 clipflag;
	u8 dummy;
	u8 q;
	u8 p;
	u16 pqinst; // bit of instruction specifying index (srec only)
} _vuopinfo;
_vuopinfo *cinfo;

void SuperVU1AnalyzeOp(VURegs *VU, _vuopinfo *info, _VURegsNum* pCodeRegs);
void _vurecAnalyzeOp(VURegs *VU, _vuopinfo *info);
void _vurecAnalyzeBlock(VURegs *VU, _vuopinfo *info, int count);


extern int vucycle;
extern int vucycleold;


/*****************************************
   VU Micromode Upper instructions
*****************************************/

void recVUMI_ABS(VURegs *vuRegs);
void recVUMI_ADD(VURegs *vuRegs);
void recVUMI_ADDi(VURegs *vuRegs);
void recVUMI_ADDq(VURegs *vuRegs);
void recVUMI_ADDx(VURegs *vuRegs);
void recVUMI_ADDy(VURegs *vuRegs);
void recVUMI_ADDz(VURegs *vuRegs);
void recVUMI_ADDw(VURegs *vuRegs);
void recVUMI_ADDA(VURegs *vuRegs);
void recVUMI_ADDAi(VURegs *vuRegs);
void recVUMI_ADDAq(VURegs *vuRegs);
void recVUMI_ADDAx(VURegs *vuRegs);
void recVUMI_ADDAy(VURegs *vuRegs);
void recVUMI_ADDAz(VURegs *vuRegs);
void recVUMI_ADDAw(VURegs *vuRegs);
void recVUMI_SUB(VURegs *vuRegs);
void recVUMI_SUBi(VURegs *vuRegs);
void recVUMI_SUBq(VURegs *vuRegs);
void recVUMI_SUBx(VURegs *vuRegs);
void recVUMI_SUBy(VURegs *vuRegs);
void recVUMI_SUBz(VURegs *vuRegs);
void recVUMI_SUBw(VURegs *vuRegs);
void recVUMI_SUBA(VURegs *vuRegs);
void recVUMI_SUBAi(VURegs *vuRegs);
void recVUMI_SUBAq(VURegs *vuRegs);
void recVUMI_SUBAx(VURegs *vuRegs);
void recVUMI_SUBAy(VURegs *vuRegs);
void recVUMI_SUBAz(VURegs *vuRegs);
void recVUMI_SUBAw(VURegs *vuRegs);
void recVUMI_MUL(VURegs *vuRegs);
void recVUMI_MULi(VURegs *vuRegs);
void recVUMI_MULq(VURegs *vuRegs);
void recVUMI_MULx(VURegs *vuRegs);
void recVUMI_MULy(VURegs *vuRegs);
void recVUMI_MULz(VURegs *vuRegs);
void recVUMI_MULw(VURegs *vuRegs);
void recVUMI_MULA(VURegs *vuRegs);
void recVUMI_MULAi(VURegs *vuRegs);
void recVUMI_MULAq(VURegs *vuRegs);
void recVUMI_MULAx(VURegs *vuRegs);
void recVUMI_MULAy(VURegs *vuRegs);
void recVUMI_MULAz(VURegs *vuRegs);
void recVUMI_MULAw(VURegs *vuRegs);
void recVUMI_MADD(VURegs *vuRegs);
void recVUMI_MADDi(VURegs *vuRegs);
void recVUMI_MADDq(VURegs *vuRegs);
void recVUMI_MADDx(VURegs *vuRegs);
void recVUMI_MADDy(VURegs *vuRegs);
void recVUMI_MADDz(VURegs *vuRegs);
void recVUMI_MADDw(VURegs *vuRegs);
void recVUMI_MADDA(VURegs *vuRegs);
void recVUMI_MADDAi(VURegs *vuRegs);
void recVUMI_MADDAq(VURegs *vuRegs);
void recVUMI_MADDAx(VURegs *vuRegs);
void recVUMI_MADDAy(VURegs *vuRegs);
void recVUMI_MADDAz(VURegs *vuRegs);
void recVUMI_MADDAw(VURegs *vuRegs);
void recVUMI_MSUB(VURegs *vuRegs);
void recVUMI_MSUBi(VURegs *vuRegs);
void recVUMI_MSUBq(VURegs *vuRegs);
void recVUMI_MSUBx(VURegs *vuRegs);
void recVUMI_MSUBy(VURegs *vuRegs);
void recVUMI_MSUBz(VURegs *vuRegs);
void recVUMI_MSUBw(VURegs *vuRegs);
void recVUMI_MSUBA(VURegs *vuRegs);
void recVUMI_MSUBAi(VURegs *vuRegs);
void recVUMI_MSUBAq(VURegs *vuRegs);
void recVUMI_MSUBAx(VURegs *vuRegs);
void recVUMI_MSUBAy(VURegs *vuRegs);
void recVUMI_MSUBAz(VURegs *vuRegs);
void recVUMI_MSUBAw(VURegs *vuRegs);
void recVUMI_MAX(VURegs *vuRegs);
void recVUMI_MAXi(VURegs *vuRegs);
void recVUMI_MAXx(VURegs *vuRegs);
void recVUMI_MAXy(VURegs *vuRegs);
void recVUMI_MAXz(VURegs *vuRegs);
void recVUMI_MAXw(VURegs *vuRegs);
void recVUMI_MINI(VURegs *vuRegs);
void recVUMI_MINIi(VURegs *vuRegs);
void recVUMI_MINIx(VURegs *vuRegs);
void recVUMI_MINIy(VURegs *vuRegs);
void recVUMI_MINIz(VURegs *vuRegs);
void recVUMI_MINIw(VURegs *vuRegs);
void recVUMI_OPMULA(VURegs *vuRegs);
void recVUMI_OPMSUB(VURegs *vuRegs);
void recVUMI_NOP(VURegs *vuRegs);
void recVUMI_FTOI0(VURegs *vuRegs);
void recVUMI_FTOI4(VURegs *vuRegs);
void recVUMI_FTOI12(VURegs *vuRegs);
void recVUMI_FTOI15(VURegs *vuRegs);
void recVUMI_ITOF0(VURegs *vuRegs);
void recVUMI_ITOF4(VURegs *vuRegs);
void recVUMI_ITOF12(VURegs *vuRegs);
void recVUMI_ITOF15(VURegs *vuRegs);
void recVUMI_CLIP(VURegs *vuRegs);

/*****************************************
   VU Micromode Lower instructions
*****************************************/

void recVUMI_DIV(VURegs *vuRegs);
void recVUMI_SQRT(VURegs *vuRegs);
void recVUMI_RSQRT(VURegs *vuRegs);
void recVUMI_IADD(VURegs *vuRegs);
void recVUMI_IADDI(VURegs *vuRegs);
void recVUMI_IADDIU(VURegs *vuRegs);
void recVUMI_IAND(VURegs *vuRegs);
void recVUMI_IOR(VURegs *vuRegs);
void recVUMI_ISUB(VURegs *vuRegs);
void recVUMI_ISUBIU(VURegs *vuRegs);
void recVUMI_MOVE(VURegs *vuRegs);
void recVUMI_MFIR(VURegs *vuRegs);
void recVUMI_MTIR(VURegs *vuRegs);
void recVUMI_MR32(VURegs *vuRegs);
void recVUMI_LQ(VURegs *vuRegs);
void recVUMI_LQD(VURegs *vuRegs);
void recVUMI_LQI(VURegs *vuRegs);
void recVUMI_SQ(VURegs *vuRegs);
void recVUMI_SQD(VURegs *vuRegs);
void recVUMI_SQI(VURegs *vuRegs);
void recVUMI_ILW(VURegs *vuRegs);
void recVUMI_ISW(VURegs *vuRegs);
void recVUMI_ILWR(VURegs *vuRegs);
void recVUMI_ISWR(VURegs *vuRegs);
void recVUMI_LOI(VURegs *vuRegs);
void recVUMI_RINIT(VURegs *vuRegs);
void recVUMI_RGET(VURegs *vuRegs);
void recVUMI_RNEXT(VURegs *vuRegs);
void recVUMI_RXOR(VURegs *vuRegs);
void recVUMI_WAITQ(VURegs *vuRegs);
void recVUMI_FSAND(VURegs *vuRegs);
void recVUMI_FSEQ(VURegs *vuRegs);
void recVUMI_FSOR(VURegs *vuRegs);
void recVUMI_FSSET(VURegs *vuRegs);
void recVUMI_FMAND(VURegs *vuRegs);
void recVUMI_FMEQ(VURegs *vuRegs);
void recVUMI_FMOR(VURegs *vuRegs);
void recVUMI_FCAND(VURegs *vuRegs);
void recVUMI_FCEQ(VURegs *vuRegs);
void recVUMI_FCOR(VURegs *vuRegs);
void recVUMI_FCSET(VURegs *vuRegs);
void recVUMI_FCGET(VURegs *vuRegs);
void recVUMI_IBEQ(VURegs *vuRegs);
void recVUMI_IBGEZ(VURegs *vuRegs);
void recVUMI_IBGTZ(VURegs *vuRegs);
void recVUMI_IBLTZ(VURegs *vuRegs);
void recVUMI_IBLEZ(VURegs *vuRegs);
void recVUMI_IBNE(VURegs *vuRegs);
void recVUMI_B(VURegs *vuRegs);
void recVUMI_BAL(VURegs *vuRegs);
void recVUMI_JR(VURegs *vuRegs);
void recVUMI_JALR(VURegs *vuRegs);
void recVUMI_MFP(VURegs *vuRegs);
void recVUMI_WAITP(VURegs *vuRegs);
void recVUMI_ESADD(VURegs *vuRegs);
void recVUMI_ERSADD(VURegs *vuRegs);
void recVUMI_ELENG(VURegs *vuRegs);
void recVUMI_ERLENG(VURegs *vuRegs);
void recVUMI_EATANxy(VURegs *vuRegs);
void recVUMI_EATANxz(VURegs *vuRegs);
void recVUMI_ESUM(VURegs *vuRegs);
void recVUMI_ERCPR(VURegs *vuRegs);
void recVUMI_ESQRT(VURegs *vuRegs);
void recVUMI_ERSQRT(VURegs *vuRegs);
void recVUMI_ESIN(VURegs *vuRegs); 
void recVUMI_EATAN(VURegs *vuRegs);
void recVUMI_EEXP(VURegs *vuRegs);
void recVUMI_XGKICK(VURegs *vuRegs);
void recVUMI_XTOP(VURegs *vuRegs);
void recVUMI_XITOP(VURegs *vuRegs);
void recVUMI_XTOP( VURegs *VU );

#endif /* __IVUMICRO_H__ */
