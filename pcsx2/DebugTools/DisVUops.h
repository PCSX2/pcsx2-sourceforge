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


#define _disVUOpcodes(VU) \
 \
/*****************/ \
/* LOWER OPCODES */ \
/*****************/ \
 \
MakeDisF(dis##VU##MI_DIV,		dName("VU_DIV");     dCP232f(_Fs_, _Fsf_); dCP232f(_Ft_, _Ftf_);) \
MakeDisF(dis##VU##MI_SQRT,		dName("VU_SQRT");    dCP232f(_Ft_, _Ftf_);) \
MakeDisF(dis##VU##MI_RSQRT, 	dName("VU_RSQRT");   dCP232f(_Fs_, _Fsf_); dCP232f(_Ft_, _Ftf_);) \
MakeDisF(dis##VU##MI_IADDI, 	dName("VU_IADDI");   dCP232i(_Ft_); dCP232i(_Fs_); dImm5();) \
MakeDisF(dis##VU##MI_IADDIU, 	dName("VU_IADDIU");  dCP232i(_Ft_); dCP232i(_Fs_); dImm11();) \
MakeDisF(dis##VU##MI_IADD, 		dName("VU_IADD");    dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_IAND, 		dName("VU_IAND");    dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_IOR, 		dName("VU_IOR");     dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_ISUB, 		dName("VU_ISUB");    dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_ISUBIU, 	dName("VU_ISUBIU");  dCP232i(_Ft_); dCP232i(_Fs_); dImm11();) \
MakeDisF(dis##VU##MI_MOVE, 		if (_Fs_ == 0 && _Ft_ == 0) { dNameU("VU_NOP"); } else { dName("VU_MOVE");    dCP2128f(_Fs_); dCP2128f(_Ft_); }) \
MakeDisF(dis##VU##MI_MFIR, 		dNameU("VU_MFIR");   dCP232i(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MTIR, 		dNameU("VU_MTIR");   dCP232i(_Ft_); dCP232f(_Fs_, _Fsf_);) \
MakeDisF(dis##VU##MI_MR32, 		dNameU("VU_MR32");    dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_LQ, 		dNameU("VU_LQ");      dCP2128f(_Ft_); dCP232i(_Fs_); dImm11();) \
MakeDisF(dis##VU##MI_LQD, 		dNameU("VU_LQD");     dCP2128f(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_LQI, 		dNameU("VU_LQI");     dCP2128f(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_SQ, 		dNameU("VU_SQ");      dCP2128f(_Fs_); dCP232i(_Ft_); dImm11(); ) \
MakeDisF(dis##VU##MI_SQD, 		dNameU("VU_SQD");     dCP2128f(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_SQI, 		dNameU("VU_SQI");     dCP2128f(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_ILW, 		dNameU("VU_ILW");     dCP232i(_Ft_); dImm11(); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_ISW, 		dNameU("VU_ISW");     dCP232i(_Ft_); dImm11(); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_ILWR, 		dNameU("VU_ILWR");    dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_ISWR, 		dNameU("VU_ISWR");    dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_LOI, 		dName("VU_LOI"); ) \
MakeDisF(dis##VU##MI_RINIT, 	dNameU("VU_RINIT");   dCP232i(REG_R); dCP232f(_Fs_, _Fsf_);) \
MakeDisF(dis##VU##MI_RGET, 		dNameU("VU_RGET");    dCP232i(REG_R); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_RNEXT, 	dNameU("VU_RNEXT");   dCP232i(REG_R); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_RXOR, 		dNameU("VU_RXOR");    dCP232i(REG_R); dCP232f(_Fs_, _Fsf_);) \
MakeDisF(dis##VU##MI_WAITQ, 	dName("VU_WAITQ");  ) \
MakeDisF(dis##VU##MI_FSAND, 	dName("VU_FSAND");   dCP232i(_Ft_); dCP232i(REG_STATUS_FLAG);) \
MakeDisF(dis##VU##MI_FSEQ, 		dName("VU_FSEQ");    dCP232i(_Ft_); dCP232i(REG_STATUS_FLAG);) \
MakeDisF(dis##VU##MI_FSOR, 		dName("VU_FSOR");    dCP232i(_Ft_); dCP232i(REG_STATUS_FLAG);) \
MakeDisF(dis##VU##MI_FSSET, 	dName("VU_FSSET");   dCP232i(REG_STATUS_FLAG);) \
MakeDisF(dis##VU##MI_FMAND, 	dName("VU_FMAND");   dCP232i(_Ft_); dCP232i(REG_MAC_FLAG); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_FMEQ, 		dName("VU_FMEQ");    dCP232i(_Ft_); dCP232i(REG_MAC_FLAG); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_FMOR, 		dName("VU_FMOR");    dCP232i(_Ft_); dCP232i(REG_MAC_FLAG); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_FCAND, 	dName("VU_FCAND");   dCP232i(1);    dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_FCEQ, 		dName("VU_FCEQ");    dCP232i(1);    dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_FCOR, 		dName("VU_FCOR");    dCP232i(1);    dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_FCSET, 	dName("VU_FCSET");   dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_FCGET, 	dName("VU_FCGET");   dCP232i(_Ft_); dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_IBEQ, 		dName("VU_IBEQ");    dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBGEZ, 	dName("VU_IBEZ");    dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBGTZ, 	dName("VU_IBGTZ");   dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBLEZ, 	dName("VU_IBLEZ");   dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBLTZ, 	dName("VU_IBLTZ");   dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBNE, 		dName("VU_IBNE");    dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_B, 		dName("VU_B");       dImm11();) \
MakeDisF(dis##VU##MI_BAL, 		dName("VU_BAL");     dImm11(); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_JR, 		dName("VU_JR");      dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_JALR, 		dName("VU_JALR");    dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_MFP, 		dName("VU_MFP");     dCP2128f(_Fs_); dCP232i(REG_P);) \
MakeDisF(dis##VU##MI_WAITP, 	dName("VU_WAITP");  ) \
MakeDisF(dis##VU##MI_ESADD, 	dName("VU_ESADD");  ) \
MakeDisF(dis##VU##MI_ERSADD, 	dName("VU_ERSADD"); ) \
MakeDisF(dis##VU##MI_ELENG, 	dName("VU_ELENG");  ) \
MakeDisF(dis##VU##MI_ERLENG, 	dName("VU_ERLENG"); ) \
MakeDisF(dis##VU##MI_EATANxy,	dName("VU_EATANxy"); ) \
MakeDisF(dis##VU##MI_EATANxz,	dName("VU_EATANxz"); ) \
MakeDisF(dis##VU##MI_ESUM, 		dName("VU_ESUM");   ) \
MakeDisF(dis##VU##MI_ERCPR, 	dName("VU_ERCPR");  ) \
MakeDisF(dis##VU##MI_ESQRT, 	dName("VU_ESQRT");  ) \
MakeDisF(dis##VU##MI_ERSQRT, 	dName("VU_ERSQRT"); ) \
MakeDisF(dis##VU##MI_ESIN, 		dName("VU_ESIN");   ) \
MakeDisF(dis##VU##MI_EATAN, 	dName("VU_EATAN");  ) \
MakeDisF(dis##VU##MI_EEXP, 		dName("VU_EEXP");   ) \
MakeDisF(dis##VU##MI_XITOP, 	dName("VU_XITOP");  ) \
MakeDisF(dis##VU##MI_XGKICK, 	dName("VU_XGKICK"); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_XTOP, 		dName("VU_XTOP");   dCP232i(_Ft_);) \
 \
 \
/*****************/ \
/* UPPER OPCODES */ \
/*****************/ \
 \
MakeDisF(dis##VU##MI_ABS, 		dNameU("VU_ABS");	 dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ADD, 		dNameU("VU_ADD");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_ADDi,		dNameU("VU_ADDi");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_ADDq, 		dNameU("VU_ADDq");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_ADDx, 		dNameU("VU_ADDx");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_ADDy, 		dNameU("VU_ADDy");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_ADDz, 		dNameU("VU_ADDz");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_ADDw, 		dNameU("VU_ADDw");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_ADDA, 		dNameU("VU_ADDA");	 dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAi,		dNameU("VU_ADDAi");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_ADDAq, 	dNameU("VU_ADDAq");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_ADDAx, 	dNameU("VU_ADDAx");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAy, 	dNameU("VU_ADDAy");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAz, 	dNameU("VU_ADDAz");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAw, 	dNameU("VU_ADDAw");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_SUB, 		dNameU("VU_SUB");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_SUBi, 		dNameU("VU_SUBi");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_SUBq, 		dNameU("VU_SUBq");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_SUBx, 		dNameU("VU_SUBx");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_SUBy, 		dNameU("VU_SUBy");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_SUBz, 		dNameU("VU_SUBz");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_SUBw, 		dNameU("VU_SUBw");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_SUBA, 		dNameU("VU_SUBA");	 dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAi,		dNameU("VU_SUBAi");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_SUBAq, 	dNameU("VU_SUBAq");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_SUBAx, 	dNameU("VU_SUBAx");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAy, 	dNameU("VU_SUBAy");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAz, 	dNameU("VU_SUBAz");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAw, 	dNameU("VU_SUBAw");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MUL, 		dNameU("VU_MUL");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MULi, 		dNameU("VU_MULi");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MULq, 		dNameU("VU_MULq");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MULx, 		dNameU("VU_MULx");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MULy, 		dNameU("VU_MULy");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MULz, 		dNameU("VU_MULz");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MULw, 		dNameU("VU_MULw");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MULA, 		dNameU("VU_MULA"); 	 dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MULAi,		dNameU("VU_MULAi");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MULAq, 	dNameU("VU_MULAq");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MULAx, 	dNameU("VU_MULAx");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MULAy, 	dNameU("VU_MULAy");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MULAz, 	dNameU("VU_MULAz");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MULAw, 	dNameU("VU_MULAw");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MADD, 		dNameU("VU_MADD");   dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MADDi, 	dNameU("VU_MADDi");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MADDq, 	dNameU("VU_MADDq");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MADDx, 	dNameU("VU_MADDx");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MADDy, 	dNameU("VU_MADDy");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MADDz, 	dNameU("VU_MADDz");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MADDw, 	dNameU("VU_MADDw");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MADDA, 	dNameU("VU_MADDA");  dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAi, 	dNameU("VU_MADDAi"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MADDAq, 	dNameU("VU_MADDAq"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MADDAx, 	dNameU("VU_MADDAx"); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAy, 	dNameU("VU_MADDAy"); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAz, 	dNameU("VU_MADDAz"); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAw, 	dNameU("VU_MADDAw"); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MSUB, 		dNameU("VU_MSUB");   dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBi, 	dNameU("VU_MSUBi");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MSUBq, 	dNameU("VU_MSUBq");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MSUBx, 	dNameU("VU_MSUBx");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBy, 	dNameU("VU_MSUBy");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBz, 	dNameU("VU_MSUBz");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBw, 	dNameU("VU_MSUBw");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBA, 	dNameU("VU_MSUBA");  dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAi, 	dNameU("VU_MSUBAi"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MSUBAq, 	dNameU("VU_MSUBAq"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MSUBAx, 	dNameU("VU_MSUBAx"); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAy, 	dNameU("VU_MSUBAy"); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAz, 	dNameU("VU_MSUBAz"); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAw, 	dNameU("VU_MSUBAw"); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MAX, 		dNameU("VU_MAX");    dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MAXi, 		dNameU("VU_MAXi");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MAXx, 		dNameU("VU_MAXx");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MAXy, 		dNameU("VU_MAXy");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MAXz, 		dNameU("VU_MAXz");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MAXw, 		dNameU("VU_MAXw");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MINI, 		dNameU("VU_MINI");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MINIi, 	dNameU("VU_MINIi");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MINIx, 	dNameU("VU_MINIx");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MINIy, 	dNameU("VU_MINIy");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MINIz, 	dNameU("VU_MINIz");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MINIw, 	dNameU("VU_MINIw");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_OPMULA, 	dNameU("VU_OPMULA"); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_OPMSUB, 	dNameU("VU_OPMSUB"); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_NOP, 		dName("VU_NOP");) \
MakeDisF(dis##VU##MI_FTOI0, 	dNameU("VU_FTOI0");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_FTOI4, 	dNameU("VU_FTOI4");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_FTOI12, 	dNameU("VU_FTOI12"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_FTOI15, 	dNameU("VU_FTOI15"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF0, 	dNameU("VU_ITOF0");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF4, 	dNameU("VU_ITOF4");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF12, 	dNameU("VU_ITOF12"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF15, 	dNameU("VU_ITOF15"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_CLIP, 		dNameU("VU_CLIP");   dCP2128f(_Fs_); dCP232w(_Ft_);) \

