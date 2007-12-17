
#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>

#include "eeload.h"
#include "eeirq.h"


u128 SavedSP;
u128 SavedRA;
u128 SavedAT;
u64  SavedT9;

eeRegs SavedRegs;

int	SRInitVal;
int	ConfigInitVal;

int	threadId;
int	threadPrio;
int	threadStatus;

u64 hvParam;

u32 machineType;
u64 gsIMR;
u32 memorySize;

u32 dmac_CHCR[10] = {
	0xB0008000,
	0xB0009000,
	0xB000A000,
	0xB000B000,
	0xB000B400,
	0xB000C000,
	0xB000C400,
	0xB000C800,
	0xB000D000,
	0xB000D400,
};

void (*VCRTable[14])() = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	SyscException, 0, 0, 0,
	0, 0
};

void (*VIntTable[8])() = {
	0, 0, DMACException, INTCException,
	0, 0, 0, 0,
};

void (*INTCTable[16])(int);
void (*DMACTable[16])(int);


void (*table_SYSCALL[0x80])() = {
	(void (*))_RFU___, 					// 0x00
	(void (*))_ResetEE, 
	(void (*))_SetGsCrt, 
	(void (*))_RFU___, 
	(void (*))__Exit, 					// 0x04
	(void (*))_RFU005, 
	(void (*))_LoadPS2Exe, 
	(void (*))_ExecPS2, 
	(void (*))_RFU___, 					// 0x08
	(void (*))_TlbWriteRandom, 
	(void (*))_AddSbusIntcHandler, 
	(void (*))_RemoveSbusIntcHandler, 
	(void (*))_Interrupt2Iop, 			// 0x0C
	(void (*))_SetVTLBRefillHandler, 
	(void (*))_SetVCommonHandler, 
	(void (*))_SetVInterruptHandler,
	(void (*))_AddIntcHandler, 			// 0x10
	(void (*))_RemoveIntcHandler, 
	(void (*))_AddDmacHandler, 
	(void (*))_RemoveDmacHandler, 
	(void (*))__EnableIntc, 			// 0x14
	(void (*))__DisableIntc, 
	(void (*))__EnableDmac, 
	(void (*))__DisableDmac, 
	(void (*))__SetAlarm, 				// 0x18
	(void (*))__ReleaseAlarm,
	(void (*))__EnableIntc, 
	(void (*))__DisableIntc, 
	(void (*))__EnableDmac,				// 0x1C 
	(void (*))__DisableDmac, 
	(void (*))__SetAlarm, 
	(void (*))__ReleaseAlarm, 
	(void (*))_CreateThread, 			// 0x20
	(void (*))_DeleteThread, 
	(void (*))_StartThread, 
	(void (*))_ExitThread,
	(void (*))_ExitDeleteThread, 		// 0x24
	(void (*))_TerminateThread, 
	(void (*))_iTerminateThread, 
	(void (*))_DisableDispatchThread,
	(void (*))_EnableDispatchThread, 	// 0x28
	(void (*))_ChangeThreadPriority, 
	(void (*))_iChangeThreadPriority, 
	(void (*))_RotateThreadReadyQueue,
	(void (*))_iRotateThreadReadyQueue, // 0x2C
	(void (*))_ReleaseWaitThread, 
	(void (*))_iReleaseWaitThread, 
	(void (*))_GetThreadId, 
	(void (*))_ReferThreadStatus, 		// 0x30
	(void (*))_ReferThreadStatus, 
	(void (*))_SleepThread, 
	(void (*))_WakeupThread,
	(void (*))_iWakeupThread, 
	(void (*))_CancelWakeupThread, 
	(void (*))_CancelWakeupThread, 
	(void (*))_SuspendThread,
	(void (*))_SuspendThread, 
	(void (*))_ResumeThread, 
	(void (*))_iResumeThread, 
	(void (*))_JoinThread,
	(void (*))_InitializeMainThread, 
	(void (*))_InitializeHeapArea, 
	(void (*))_EndOfHeap, 
	(void (*))_RFU___, 
	(void (*))_CreateSema, 				// 0x40
	(void (*))_DeleteSema, 
	(void (*))_SignalSema, 
	(void (*))_iSignalSema,
	(void (*))_WaitSema, 
	(void (*))_PollSema, 
	(void (*))_PollSema, 
	(void (*))_ReferSemaStatus,
	(void (*))_ReferSemaStatus, 
	(void (*))_RFU073, 
	(void (*))_SetOsdConfigParam, 
	(void (*))_GetOsdConfigParam, 
	(void (*))_GetGsHParam, 
	(void (*))_GetGsVParam, 
	(void (*))_SetGsHParam, 
	(void (*))_SetGsVParam, 
	(void (*))_CreateEventFlag, 		// 0x50
	(void (*))_DeleteEventFlag, 
	(void (*))_SetEventFlag, 
	(void (*))_iSetEventFlag,
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_RFU___, 
	(void (*))_EnableIntcHandler, 
	(void (*))_DisableIntcHandler, 
	(void (*))_EnableDmacHandler, 
	(void (*))_DisableDmacHandler, 
	(void (*))_KSeg0, 					// 0x60
	(void (*))_EnableCache, 
	(void (*))_DisableCache, 
	(void (*))_GetCop0,
	(void (*))_FlushCache, 
	(void (*))_105, 
	(void (*))_CpuConfig, 
	(void (*))_GetCop0, 
	(void (*))_FlushCache, 
	(void (*))_105, 
	(void (*))_CpuConfig, 
	(void (*))0,                      //_sceSifStopDma, 
	(void (*))_SetCPUTimerHandler, 
	(void (*))_SetCPUTimer, 
	(void (*))0, 
	(void (*))0, 
	(void (*))_GsGetIMR, 				// 0x70
	(void (*))_GsPutIMR, 
	(void (*))_SetPgifHandler, 
	(void (*))_SetVSyncFlag,
	(void (*))_SetSYSCALL, 
	(void (*))_print, 
	(void (*))_SifDmaStat, 
	(void (*))_SifSetDma,
	(void (*))_SifSetDChain, 
	(void (*))_SifSetReg, 
	(void (*))_SifGetReg,
	(void (*))_ExecOSD,
	(void (*))_RFU___, 
	(void (*))_PSMode, 
	(void (*))_MachineType, 
	(void (*))_GetMemorySize
};

