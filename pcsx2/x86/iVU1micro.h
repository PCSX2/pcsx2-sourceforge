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

#ifndef __IVU1MICRO_H__
#define __IVU1MICRO_H__

void recVU1Init();
void recVU1Shutdown();
void recResetVU1();
void recExecuteVU1Block( void );
void recClearVU1( u32 Addr, u32 Size );

extern void (*recVU1_LOWER_OPCODE[128])();
extern void (*recVU1_UPPER_OPCODE[64])();

extern void (*recVU1_UPPER_FD_00_TABLE[32])();
extern void (*recVU1_UPPER_FD_01_TABLE[32])();
extern void (*recVU1_UPPER_FD_10_TABLE[32])();
extern void (*recVU1_UPPER_FD_11_TABLE[32])();


void recVU1_UPPER_FD_00();
void recVU1_UPPER_FD_01();
void recVU1_UPPER_FD_10();
void recVU1_UPPER_FD_11();

void recVU1LowerOP();
void recVU1LowerOP_T3_00();
void recVU1LowerOP_T3_01();
void recVU1LowerOP_T3_10();
void recVU1LowerOP_T3_11();

void recVU1unknown();

/*****************************************
   VU1 Micromode Upper instructions
*****************************************/

void recVU1MI_ABS();
void recVU1MI_ADD();
void recVU1MI_ADDi();
void recVU1MI_ADDq();
void recVU1MI_ADDx();
void recVU1MI_ADDy();
void recVU1MI_ADDz();
void recVU1MI_ADDw();
void recVU1MI_ADDA();
void recVU1MI_ADDAi();
void recVU1MI_ADDAq();
void recVU1MI_ADDAx();
void recVU1MI_ADDAy();
void recVU1MI_ADDAz();
void recVU1MI_ADDAw();
void recVU1MI_SUB();
void recVU1MI_SUBi();
void recVU1MI_SUBq();
void recVU1MI_SUBx();
void recVU1MI_SUBy();
void recVU1MI_SUBz();
void recVU1MI_SUBw();
void recVU1MI_SUBA();
void recVU1MI_SUBAi();
void recVU1MI_SUBAq();
void recVU1MI_SUBAx();
void recVU1MI_SUBAy();
void recVU1MI_SUBAz();
void recVU1MI_SUBAw();
void recVU1MI_MUL();
void recVU1MI_MULi();
void recVU1MI_MULq();
void recVU1MI_MULx();
void recVU1MI_MULy();
void recVU1MI_MULz();
void recVU1MI_MULw();
void recVU1MI_MULA();
void recVU1MI_MULAi();
void recVU1MI_MULAq();
void recVU1MI_MULAx();
void recVU1MI_MULAy();
void recVU1MI_MULAz();
void recVU1MI_MULAw();
void recVU1MI_MADD();
void recVU1MI_MADDi();
void recVU1MI_MADDq();
void recVU1MI_MADDx();
void recVU1MI_MADDy();
void recVU1MI_MADDz();
void recVU1MI_MADDw();
void recVU1MI_MADDA();
void recVU1MI_MADDAi();
void recVU1MI_MADDAq();
void recVU1MI_MADDAx();
void recVU1MI_MADDAy();
void recVU1MI_MADDAz();
void recVU1MI_MADDAw();
void recVU1MI_MSUB();
void recVU1MI_MSUBi();
void recVU1MI_MSUBq();
void recVU1MI_MSUBx();
void recVU1MI_MSUBy();
void recVU1MI_MSUBz();
void recVU1MI_MSUBw();
void recVU1MI_MSUBA();
void recVU1MI_MSUBAi();
void recVU1MI_MSUBAq();
void recVU1MI_MSUBAx();
void recVU1MI_MSUBAy();
void recVU1MI_MSUBAz();
void recVU1MI_MSUBAw();
void recVU1MI_MAX();
void recVU1MI_MAXi();
void recVU1MI_MAXx();
void recVU1MI_MAXy();
void recVU1MI_MAXz();
void recVU1MI_MAXw();
void recVU1MI_MINI();
void recVU1MI_MINIi();
void recVU1MI_MINIx();
void recVU1MI_MINIy();
void recVU1MI_MINIz();
void recVU1MI_MINIw();
void recVU1MI_OPMULA();
void recVU1MI_OPMSUB();
void recVU1MI_NOP();
void recVU1MI_FTOI0();
void recVU1MI_FTOI4();
void recVU1MI_FTOI12();
void recVU1MI_FTOI15();
void recVU1MI_ITOF0();
void recVU1MI_ITOF4();
void recVU1MI_ITOF12();
void recVU1MI_ITOF15();
void recVU1MI_CLIP();

/*****************************************
   VU1 Micromode Lower instructions
*****************************************/

void recVU1MI_DIV();
void recVU1MI_SQRT();
void recVU1MI_RSQRT();
void recVU1MI_IADD();
void recVU1MI_IADDI();
void recVU1MI_IADDIU();
void recVU1MI_IAND();
void recVU1MI_IOR();
void recVU1MI_ISUB();
void recVU1MI_ISUBIU();
void recVU1MI_MOVE();
void recVU1MI_MFIR();
void recVU1MI_MTIR();
void recVU1MI_MR32();
void recVU1MI_LQ();
void recVU1MI_LQD();
void recVU1MI_LQI();
void recVU1MI_SQ();
void recVU1MI_SQD();
void recVU1MI_SQI();
void recVU1MI_ILW();
void recVU1MI_ISW();
void recVU1MI_ILWR();
void recVU1MI_ISWR();
void recVU1MI_RINIT();
void recVU1MI_RGET();
void recVU1MI_RNEXT();
void recVU1MI_RXOR();
void recVU1MI_WAITQ();
void recVU1MI_FSAND();
void recVU1MI_FSEQ();
void recVU1MI_FSOR();
void recVU1MI_FSSET();
void recVU1MI_FMAND();
void recVU1MI_FMEQ();
void recVU1MI_FMOR();
void recVU1MI_FCAND();
void recVU1MI_FCEQ();
void recVU1MI_FCOR();
void recVU1MI_FCSET();
void recVU1MI_FCGET();
void recVU1MI_IBEQ();
void recVU1MI_IBGEZ();
void recVU1MI_IBGTZ();
void recVU1MI_IBLEZ();
void recVU1MI_IBLTZ();
void recVU1MI_IBNE();
void recVU1MI_B();
void recVU1MI_BAL();
void recVU1MI_JR();
void recVU1MI_JALR();
void recVU1MI_MFP();
void recVU1MI_WAITP();
void recVU1MI_ESADD();
void recVU1MI_ERSADD();
void recVU1MI_ELENG();
void recVU1MI_ERLENG();
void recVU1MI_EATANxy();
void recVU1MI_EATANxz();
void recVU1MI_ESUM();
void recVU1MI_ERCPR();
void recVU1MI_ESQRT();
void recVU1MI_ERSQRT();
void recVU1MI_ESIN(); 
void recVU1MI_EATAN();
void recVU1MI_EEXP();
void recVU1MI_XITOP();
void recVU1MI_XGKICK();
void recVU1MI_XTOP();

extern char *recMemVU1;	/* VU1 blocks */
extern char *recVU1;	   /* VU1 mem */
extern char *recVU1mac;
extern char *recVU1status;
extern char *recVU1clip;
extern char *recVU1Q;
extern char *recVU1P;
extern char *recVU1cycles;
extern char* recVU1XMMRegs;
extern char *recPtrVU1;
extern char* recVU1Blocks;

extern u32 vu1recpcold;
extern u32 vu1reccountold;

#endif /* __IVU1MICRO_H__ */
