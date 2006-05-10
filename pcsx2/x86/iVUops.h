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

#ifdef __MSCW32__
#pragma warning(disable:4244)
#endif

#define REC_VUOP(VU, f) { \
	_freeXMMregs(&VU); \
	MOV32ItoM((u32)&VU.code, (u32)VU.code); \
	CALLFunc((u32)VU##MI_##f); \
}

#define REC_VUOPFLAGS(VU, f) { \
	_freeXMMregs(&VU); \
	MOV32ItoM((u32)&VU.code, (u32)VU.code); \
	CALLFunc((u32)VU##MI_##f); \
}

#define REC_VUBRANCH(VU, f) { \
	_freeXMMregs(&VU); \
	MOV32ItoM((u32)&VU.code, (u32)VU.code); \
	MOV32ItoM((u32)&VU.VI[REG_TPC].UL, (u32)pc); \
	CALLFunc((u32)VU##MI_##f); \
	branch = 1; \
}

/******************************/
/*   VU Upper instructions    */
/******************************/

#define rec_vuABS(VU) { \
	REC_VUOP(VU, ABS); \
}

#define rec_vuADD(VU) { \
	REC_VUOP(VU, ADD); \
}

#define rec_vuADDi(VU) { \
	REC_VUOP(VU, ADDi); \
}

#define rec_vuADDq(VU) { \
	REC_VUOP(VU, ADDq); \
}

#define rec_vuADDx(VU) { \
	REC_VUOP(VU, ADDx); \
}

#define rec_vuADDy(VU) { \
	REC_VUOP(VU, ADDy); \
}

#define rec_vuADDz(VU) { \
	REC_VUOP(VU, ADDz); \
}

#define rec_vuADDw(VU) { \
	REC_VUOP(VU, ADDw); \
}

#define rec_vuADDA(VU) { \
	REC_VUOP(VU, ADDA); \
}

#define rec_vuADDAi(VU) { \
	REC_VUOP(VU, ADDAi); \
}

#define rec_vuADDAq(VU) { \
	REC_VUOP(VU, ADDAq); \
}

#define rec_vuADDAx(VU) { \
	REC_VUOP(VU, ADDAx); \
}

#define rec_vuADDAy(VU) { \
	REC_VUOP(VU, ADDAy); \
}

#define rec_vuADDAz(VU) { \
	REC_VUOP(VU, ADDAz); \
}

#define rec_vuADDAw(VU) { \
	REC_VUOP(VU, ADDAw); \
}

#define rec_vuSUB(VU) { \
	REC_VUOP(VU, SUB); \
}

#define rec_vuSUBi(VU) { \
	REC_VUOP(VU, SUBi); \
}

#define rec_vuSUBq(VU) { \
	REC_VUOP(VU, SUBq); \
}

#define rec_vuSUBx(VU) { \
	REC_VUOP(VU, SUBx); \
}


#define rec_vuSUBy(VU) { \
	REC_VUOP(VU, SUBy); \
}

#define rec_vuSUBz(VU) { \
	REC_VUOP(VU, SUBz); \
}

#define rec_vuSUBw(VU) { \
	REC_VUOP(VU, SUBw); \
}

#define rec_vuSUBA(VU) { \
	REC_VUOP(VU, SUBA); \
}

#define rec_vuSUBAi(VU) { \
	REC_VUOP(VU, SUBAi); \
}

#define rec_vuSUBAq(VU) { \
	REC_VUOP(VU, SUBAq); \
}

#define rec_vuSUBAx(VU) { \
	REC_VUOP(VU, SUBAx); \
}

#define rec_vuSUBAy(VU) { \
	REC_VUOP(VU, SUBAy); \
}

#define rec_vuSUBAz(VU) { \
	REC_VUOP(VU, SUBAz); \
}

#define rec_vuSUBAw(VU) { \
	REC_VUOP(VU, SUBAw); \
}

#define rec_vuMUL(VU) { \
	REC_VUOP(VU, MUL); \
}

#define rec_vuMULi(VU) { \
	REC_VUOP(VU, MULi); \
}

#define rec_vuMULq(VU) { \
	REC_VUOP(VU, MULq); \
}

#define rec_vuMULx(VU) { \
	REC_VUOP(VU, MULx); \
}

#define rec_vuMULy(VU) { \
	REC_VUOP(VU, MULy); \
}

#define rec_vuMULz(VU) { \
	REC_VUOP(VU, MULz); \
}

#define rec_vuMULw(VU) { \
	REC_VUOP(VU, MULw); \
}

#define rec_vuMULA(VU) { \
	REC_VUOP(VU, MULA); \
}

#define rec_vuMULAi(VU) { \
	REC_VUOP(VU, MULAi); \
}

#define rec_vuMULAq(VU) { \
	REC_VUOP(VU, MULAq); \
}

#define rec_vuMULAx(VU) { \
	REC_VUOP(VU, MULAx); \
}

#define rec_vuMULAy(VU) { \
	REC_VUOP(VU, MULAy); \
}

#define rec_vuMULAz(VU) { \
	REC_VUOP(VU, MULAz); \
}

#define rec_vuMULAw(VU) { \
	REC_VUOP(VU, MULAw); \
}

#define rec_vuMADD(VU) { \
	REC_VUOP(VU, MADD); \
}

#define rec_vuMADDi(VU) { \
	REC_VUOP(VU, MADDi); \
}

#define rec_vuMADDq(VU) { \
	REC_VUOP(VU, MADDq); \
}

#define rec_vuMADDx(VU) { \
	REC_VUOP(VU, MADDx); \
}

#define rec_vuMADDy(VU) { \
	REC_VUOP(VU, MADDy); \
}

#define rec_vuMADDz(VU) { \
	REC_VUOP(VU, MADDz); \
}

#define rec_vuMADDw(VU) { \
	REC_VUOP(VU, MADDw); \
}

#define rec_vuMADDA(VU) { \
	REC_VUOP(VU, MADDA); \
}

#define rec_vuMADDAi(VU) { \
	REC_VUOP(VU, MADDAi); \
}

#define rec_vuMADDAq(VU) { \
	REC_VUOP(VU, MADDAq); \
}

#define rec_vuMADDAx(VU) { \
	REC_VUOP(VU, MADDAx); \
}

#define rec_vuMADDAy(VU) { \
	REC_VUOP(VU, MADDAy); \
}

#define rec_vuMADDAz(VU) { \
	REC_VUOP(VU, MADDAz); \
}

#define rec_vuMADDAw(VU) { \
	REC_VUOP(VU, MADDAw); \
}

#define rec_vuMSUB(VU) { \
	REC_VUOP(VU, MSUB); \
}

#define rec_vuMSUBi(VU) { \
	REC_VUOP(VU, MSUBi); \
}

#define rec_vuMSUBq(VU) { \
	REC_VUOP(VU, MSUBq); \
}

#define rec_vuMSUBx(VU) { \
	REC_VUOP(VU, MSUBx); \
}

#define rec_vuMSUBy(VU) { \
	REC_VUOP(VU, MSUBy); \
}

#define rec_vuMSUBz(VU) { \
	REC_VUOP(VU, MSUBz); \
}

#define rec_vuMSUBw(VU) { \
	REC_VUOP(VU, MSUBw); \
}

#define rec_vuMSUBA(VU) { \
	REC_VUOP(VU, MSUBA); \
}

#define rec_vuMSUBAi(VU) { \
	REC_VUOP(VU, MSUBAi); \
}

#define rec_vuMSUBAq(VU) { \
	REC_VUOP(VU, MSUBAq); \
}

#define rec_vuMSUBAx(VU) { \
	REC_VUOP(VU, MSUBAx); \
}

#define rec_vuMSUBAy(VU) { \
	REC_VUOP(VU, MSUBAy); \
}

#define rec_vuMSUBAz(VU) { \
	REC_VUOP(VU, MSUBAz); \
}

#define rec_vuMSUBAw(VU) { \
	REC_VUOP(VU, MSUBAw); \
}

#define rec_vuMAX(VU) { \
	REC_VUOP(VU, MAX); \
}

#define rec_vuMAXi(VU) { \
	REC_VUOP(VU, MAXi); \
}

#define rec_vuMAXx(VU) { \
	REC_VUOP(VU, MAXx); \
}

#define rec_vuMAXy(VU) { \
	REC_VUOP(VU, MAXy); \
}

#define rec_vuMAXz(VU) { \
	REC_VUOP(VU, MAXz); \
}

#define rec_vuMAXw(VU) { \
	REC_VUOP(VU, MAXw); \
}

#define rec_vuMINI(VU) { \
	REC_VUOP(VU, MINI); \
}

#define rec_vuMINIi(VU) { \
	REC_VUOP(VU, MINIi); \
}

#define rec_vuMINIx(VU) { \
	REC_VUOP(VU, MINIx); \
}

#define rec_vuMINIy(VU) { \
	REC_VUOP(VU, MINIy); \
}

#define rec_vuMINIz(VU) { \
	REC_VUOP(VU, MINIz); \
}

#define rec_vuMINIw(VU) { \
	REC_VUOP(VU, MINIw); \
}

#define rec_vuOPMULA(VU) { \
	REC_VUOP(VU, OPMULA); \
}

#define rec_vuOPMSUB(VU) { \
	REC_VUOP(VU, OPMSUB); \
}

#define rec_vuNOP(VU) { \
}

#define rec_vuFTOI0(VU) { \
	REC_VUOP(VU, FTOI0); \
}

#define rec_vuFTOI4(VU) { \
	REC_VUOP(VU, FTOI4); \
}

#define rec_vuFTOI12(VU) { \
	REC_VUOP(VU, FTOI12); \
}

#define rec_vuFTOI15(VU) { \
	REC_VUOP(VU, FTOI15); \
}

#define rec_vuITOF0(VU) { \
	REC_VUOP(VU, ITOF0); \
}

#define rec_vuITOF4(VU) { \
	REC_VUOP(VU, ITOF4); \
}

#define rec_vuITOF12(VU) { \
	REC_VUOP(VU, ITOF12); \
}

#define rec_vuITOF15(VU) { \
	REC_VUOP(VU, ITOF15); \
}

#define rec_vuCLIP(VU) { \
	REC_VUOP(VU, CLIP); \
}

/******************************/ \
/*   VU Lower instructions    */ \
/******************************/ \

#define rec_vuDIV(VU) { \
	REC_VUOP(VU, DIV); \
}

#define rec_vuSQRT(VU) { \
	REC_VUOP(VU, SQRT); \
}

#define rec_vuRSQRT(VU) { \
	REC_VUOP(VU, RSQRT); \
}

#define rec_vuIADDI(VU) { \
	REC_VUOP(VU, IADDI); \
}

#define rec_vuIADDIU(VU) { \
	REC_VUOP(VU, IADDIU); \
}

#define rec_vuIADD(VU) { \
	REC_VUOP(VU, IADD); \
}

#define rec_vuIAND(VU) { \
	REC_VUOP(VU, IAND); \
}

#define rec_vuIOR(VU) { \
	REC_VUOP(VU, IOR); \
}

#define rec_vuISUB(VU) { \
	REC_VUOP(VU, ISUB); \
}

#define rec_vuISUBIU(VU) { \
	REC_VUOP(VU, ISUBIU); \
}

#define rec_vuMOVE(VU) { \
	REC_VUOP(VU, MOVE); \
}

#define rec_vuMFIR(VU) { \
	REC_VUOP(VU, MFIR); \
}

#define rec_vuMTIR(VU) { \
	REC_VUOP(VU, MTIR); \
} 

#define rec_vuMR32(VU) { \
	REC_VUOP(VU, MR32); \
}

#define rec_vuLQ(VU) { \
	REC_VUOP(VU, LQ); \
}

#define rec_vuLQD(VU) { \
	REC_VUOP(VU, LQD); \
}

#define rec_vuLQI(VU) { \
	REC_VUOP(VU, LQI); \
}

#define rec_vuSQ(VU) { \
	REC_VUOP(VU, SQ); \
}

#define rec_vuSQD(VU) { \
	REC_VUOP(VU, SQD); \
}

#define rec_vuSQI(VU) { \
	REC_VUOP(VU, SQI); \
}

#define rec_vuILW(VU) { \
	REC_VUOP(VU, ILW); \
}

#define rec_vuISW(VU) { \
	REC_VUOP(VU, ISW); \
}

#define rec_vuILWR(VU) { \
	REC_VUOP(VU, ILWR); \
}

#define rec_vuISWR(VU) { \
	REC_VUOP(VU, ISWR); \
}

#define rec_vuLOI(VU) { \
	REC_VUOP(VU, LOI); \
}

#define rec_vuRINIT(VU) { \
	REC_VUOP(VU, RINIT); \
}

#define rec_vuRGET(VU) { \
	REC_VUOP(VU, RGET); \
}

#define rec_vuRNEXT(VU) { \
	REC_VUOP(VU, RNEXT); \
}

#define rec_vuRXOR(VU) { \
	REC_VUOP(VU, RXOR); \
}

#define rec_vuWAITQ(VU) { \
	REC_VUOP(VU, WAITQ); \
}

#define rec_vuFSAND(VU) { \
	REC_VUOPFLAGS(VU, FSAND); \
}

#define rec_vuFSEQ(VU) { \
	REC_VUOPFLAGS(VU, FSEQ); \
}

#define rec_vuFSOR(VU) { \
	REC_VUOPFLAGS(VU, FSOR); \
}

#define rec_vuFSSET(VU) { \
	REC_VUOP(VU, FSSET); \
}

#define rec_vuFMAND(VU) { \
	REC_VUOPFLAGS(VU, FMAND); \
}

#define rec_vuFMEQ(VU) { \
	REC_VUOPFLAGS(VU, FMEQ); \
}

#define rec_vuFMOR(VU) { \
	REC_VUOPFLAGS(VU, FMOR); \
}

#define rec_vuFCAND(VU) { \
	REC_VUOPFLAGS(VU, FCAND); \
}

#define rec_vuFCEQ(VU) { \
	REC_VUOPFLAGS(VU, FCEQ); \
}

#define rec_vuFCOR(VU) { \
	REC_VUOPFLAGS(VU, FCOR); \
}

#define rec_vuFCSET(VU) { \
	REC_VUOP(VU, FCSET); \
}

#define rec_vuFCGET(VU) { \
	REC_VUOPFLAGS(VU, FCGET); \
}

#define rec_vuIBEQ(VU) { \
	REC_VUBRANCH(VU, IBEQ); \
}

#define rec_vuIBGEZ(VU) { \
	REC_VUBRANCH(VU, IBGEZ); \
}

#define rec_vuIBGTZ(VU) { \
	REC_VUBRANCH(VU, IBGTZ); \
}

#define rec_vuIBLEZ(VU) { \
	REC_VUBRANCH(VU, IBLEZ); \
}

#define rec_vuIBLTZ(VU) { \
	REC_VUBRANCH(VU, IBLTZ); \
}

#define rec_vuIBNE(VU) { \
	REC_VUBRANCH(VU, IBNE); \
}

#define rec_vuB(VU) { \
	REC_VUBRANCH(VU, B); \
}

#define rec_vuBAL(VU) { \
	REC_VUBRANCH(VU, BAL); \
}

#define rec_vuJR(VU) { \
	REC_VUBRANCH(VU, JR); \
}

#define rec_vuJALR(VU) { \
	REC_VUBRANCH(VU, JALR); \
}

#define rec_vuMFP(VU) { \
	REC_VUOP(VU, MFP); \
}

#define rec_vuWAITP(VU) { \
}

#define rec_vuESADD(VU) { \
	REC_VUOP(VU, ESADD); \
}

#define rec_vuERSADD(VU) { \
	REC_VUOP(VU, ERSADD); \
}

#define rec_vuELENG(VU) { \
	REC_VUOP(VU, ELENG); \
}

#define rec_vuERLENG(VU) { \
	REC_VUOP(VU, ERLENG); \
}

#define rec_vuEATANxy(VU) { \
	REC_VUOP(VU, EATANxy); \
}

#define rec_vuEATANxz(VU) { \
	REC_VUOP(VU, EATANxz); \
}

#define rec_vuESUM(VU) { \
	REC_VUOP(VU, ESUM); \
}

#define rec_vuERCPR(VU) { \
	REC_VUOP(VU, ERCPR); \
}

#define rec_vuESQRT(VU) { \
	REC_VUOP(VU, ESQRT); \
}

#define rec_vuERSQRT(VU) { \
	REC_VUOP(VU, ERSQRT); \
}

#define rec_vuESIN(VU) { \
	REC_VUOP(VU, ESIN); \
}

#define rec_vuEATAN(VU) { \
	REC_VUOP(VU, EATAN); \
}

#define rec_vuEEXP(VU) { \
	REC_VUOP(VU, EEXP); \
}

#define rec_vuXITOP(VU) { \
	REC_VUOP(VU, XITOP); \
}

