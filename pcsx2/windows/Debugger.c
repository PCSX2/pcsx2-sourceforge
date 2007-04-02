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


#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "Debugger.h"
#include "Common.h"
#include "win32.h"
#include "PsxMem.h"
#include "R3000A.h"

extern void (*IOP_DEBUG_BSC[64])(char *buf);
extern void UpdateR5900op();
void RefreshIOPDebugger(void);
extern int ISR3000A;//for disasm
HWND hWnd_debugdisasm, hWnd_debugscroll,hWnd_IOP_debugdisasm, hWnd_IOP_debugscroll;
unsigned long DebuggerPC = 0;
HWND hRegDlg;//for debug registers..
HWND debughWnd;
unsigned long DebuggerIOPPC=0;
HWND hIOPDlg;//IOP debugger


void RefreshDebugAll()//refresh disasm and register window
{
  RefreshDebugger();
  RefreshIOPDebugger();
  UpdateRegs();


}

void MakeDebugOpcode(void)
{
	memRead32( opcode_addr, &cpuRegs.code );
}

void MakeIOPDebugOpcode(void)
{
	psxRegs.code = PSXMu32( opcode_addr);
}

BOOL HasBreakpoint()
{
	int t;

	for (t = 0; t < NUM_BREAKPOINTS; t++)
	{
		switch (bkpt_regv[t].type) {
			case 1: // exec
				if (cpuRegs.pc == bkpt_regv[t].value) return TRUE;
				break;

			case 2: // count
				if ((cpuRegs.cycle - 10) <= bkpt_regv[t].value &&
					(cpuRegs.cycle + 10) >= bkpt_regv[t].value) return TRUE;
				break;
		}
	}
	return FALSE;

}
BOOL APIENTRY JumpProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[16];
    unsigned long temp;

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", cpuRegs.pc);
            SetDlgItemText(hDlg, IDC_JUMP_PC, buf);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_JUMP_PC, buf, 9);

                buf[8] = 0;
                sscanf(buf, "%x", &temp);

                temp      &= 0xFFFFFFFC;
                DebuggerPC = temp - 0x00000038;

                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}

extern void EEDumpRegs(FILE * fp);
extern void IOPDumpRegs(FILE * fp);
BOOL APIENTRY DumpProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char start[16], end[16], fname[128], tmp[128], buf[128];
    unsigned long start_pc, end_pc, temp;

	FILE *fp;

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", cpuRegs.pc);
            SetDlgItemText(hDlg, IDC_DUMP_START, buf);
            SetDlgItemText(hDlg, IDC_DUMP_END,   buf);
			SetDlgItemText(hDlg, IDC_DUMP_FNAME, "EEdisasm.txt");

			sprintf(buf, "%08X", psxRegs.pc);
            SetDlgItemText(hDlg, IDC_DUMP_STARTIOP, buf);
            SetDlgItemText(hDlg, IDC_DUMP_ENDIOP,   buf);
			SetDlgItemText(hDlg, IDC_DUMP_FNAMEIOP, "IOPdisasm.txt");
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_DUMP_START, start, 9);
                start[8] = 0;
                sscanf(start, "%x", &start_pc);
                start_pc &= 0xFFFFFFFC;

                GetDlgItemText(hDlg, IDC_DUMP_END, end, 9);
                end[8] = 0;
                sscanf(end, "%x", &end_pc);
                end_pc &= 0xFFFFFFFC;

				GetDlgItemText(hDlg, IDC_DUMP_FNAME, fname, 128);
				fp = fopen(fname, "wt");
				if (fp == NULL)
				{
					//MessageBox(MainhWnd, "Can't open file for writing!", NULL, MB_OK);
				}
				else
				{
					fprintf(fp,"----------------------------------\n");
					fprintf(fp,"EE DISASM TEXT DOCUMENT BY PCSX2  \n");
					fprintf(fp,"----------------------------------\n");
					for (temp = start_pc; temp <= end_pc; temp += 4)
					{
				
						
						opcode_addr=temp;
						MakeDebugOpcode();											
                        OpcodePrintTable[(cpuRegs.code) >> 26](tmp);
						if (HasBreakpoint(temp))
						{
								sprintf(buf, "*%08X %08X: %s", temp, cpuRegs.code, tmp);
						}
						else
						{
								sprintf(buf, "%08X %08X: %s", temp, cpuRegs.code, tmp);
						}

						fprintf(fp, "%s\n", buf);
					}


					fprintf(fp,"\n\n\n----------------------------------\n");
					fprintf(fp,"EE REGISTER DISASM TEXT DOCUMENT BY PCSX2  \n");
					fprintf(fp,"----------------------------------\n");
					EEDumpRegs(fp);
					fclose(fp);
				}



				GetDlgItemText(hDlg, IDC_DUMP_STARTIOP, start, 9);
                start[8] = 0;
                sscanf(start, "%x", &start_pc);
                start_pc &= 0xFFFFFFFC;

                GetDlgItemText(hDlg, IDC_DUMP_ENDIOP, end, 9);
                end[8] = 0;
                sscanf(end, "%x", &end_pc);
                end_pc &= 0xFFFFFFFC;

				GetDlgItemText(hDlg, IDC_DUMP_FNAMEIOP, fname, 128);
				fp = fopen(fname, "wt");
				if (fp == NULL)
				{
					//MessageBox(MainhWnd, "Can't open file for writing!", NULL, MB_OK);
				}
				else
				{
					fprintf(fp,"----------------------------------\n");
					fprintf(fp,"IOP DISASM TEXT DOCUMENT BY PCSX2 \n");
					fprintf(fp,"----------------------------------\n");
					for (temp = start_pc; temp <= end_pc; temp += 4)
					{
						opcode_addr=temp;
						MakeIOPDebugOpcode();											
                        IOP_DEBUG_BSC[(psxRegs.code) >> 26](tmp);
						sprintf(buf, "%08X %08X: %s", temp, psxRegs.code, tmp);
						fprintf(fp, "%s\n", buf);
					}

					fprintf(fp,"\n\n\n----------------------------------\n");
					fprintf(fp,"IOP REGISTER DISASM TEXT DOCUMENT BY PCSX2  \n");
					fprintf(fp,"----------------------------------\n");
					IOPDumpRegs(fp);
					fclose(fp);
				}
                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}

BOOL APIENTRY BpexecProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[16];

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", bkpt_regv[0].value);
            SetDlgItemText(hDlg, IDC_EXECBP, buf);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_EXECBP, buf, 9);

                buf[8] = 0;
                sscanf(buf, "%x", &bkpt_regv[0].value);
				bkpt_regv[0].type = 1;

                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}

BOOL APIENTRY BpcntProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[16];

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", bkpt_regv[1].value);
            SetDlgItemText(hDlg, IDC_CNTBP, buf);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_CNTBP, buf, 9);

                buf[8] = 0;
                sscanf(buf, "%x", &bkpt_regv[1].value);
				bkpt_regv[1].type = 2;

				EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}
HINSTANCE m2_hInst;
HWND m2_hWnd;
HWND hIopDlg;

LRESULT CALLBACK IOP_DISASM(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
            hWnd_IOP_debugdisasm = GetDlgItem(hDlg, IDC_DEBUG_DISASM_IOP);
            hWnd_IOP_debugscroll = GetDlgItem(hDlg, IDC_DEBUG_SCROLL_IOP);

            SendMessage(hWnd_IOP_debugdisasm, LB_INITSTORAGE, 29, 1131);
            SendMessage(hWnd_IOP_debugscroll, SBM_SETRANGE, 0, MAXLONG);
            SendMessage(hWnd_IOP_debugscroll, SBM_SETPOS, MAXLONG / 2, TRUE);
            RefreshIOPDebugger();
	    return (TRUE);
     case WM_VSCROLL:
		   switch ((int) LOWORD(wParam))
           {

              case SB_LINEDOWN: DebuggerIOPPC += 0x00000004; RefreshIOPDebugger(); break;
              case SB_LINEUP:   DebuggerIOPPC -= 0x00000004; RefreshIOPDebugger(); break;
              case SB_PAGEDOWN: DebuggerIOPPC += 0x00000029; RefreshIOPDebugger(); break;
              case SB_PAGEUP:   DebuggerIOPPC -= 0x00000029; RefreshIOPDebugger(); break;
			}
            return TRUE;
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}
int CreatePropertySheet2(HWND hwndOwner)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh;

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = m2_hInst;
	psp[0].pszTemplate = MAKEINTRESOURCE( IDD_IOP_DEBUG);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc =(DLGPROC)IOP_DISASM;
	psp[0].pszTitle = "Iop Disasm";
	psp[0].lParam = 0;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS;
	psh.hwndParent =hwndOwner;
	psh.hInstance = m2_hInst;
	psh.pszIcon = NULL;
	psh.pszCaption = (LPSTR) "IOP Debugger";
	psh.nStartPage = 0;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) &psp;
   
      return (PropertySheet(&psh)); 
}

BOOL APIENTRY DebuggerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	
    FARPROC jmpproc, dumpproc;
    FARPROC bpexecproc, bpcntproc;
	u32 oldpc = 0;

    switch (message)
    {
        case WM_INITDIALOG:
			ShowCursor(TRUE);

			SetWindowText(hDlg, "R5900 Debugger");
            debughWnd=hDlg;
            DebuggerPC = 0;
            // Clear all breakpoints.
			memset(bkpt_regv, 0, sizeof(bkpt_regv));

			hWnd_debugdisasm = GetDlgItem(hDlg, IDC_DEBUG_DISASM);
            hWnd_debugscroll = GetDlgItem(hDlg, IDC_DEBUG_SCROLL);

            SendMessage(hWnd_debugdisasm, LB_INITSTORAGE, 29, 1131);
            SendMessage(hWnd_debugscroll, SBM_SETRANGE, 0, MAXLONG);
            SendMessage(hWnd_debugscroll, SBM_SETPOS, MAXLONG / 2, TRUE);

            hRegDlg = (HWND)CreatePropertySheet(hDlg);
			hIopDlg = (HWND)CreatePropertySheet2(hDlg);
	        UpdateRegs();   
            SetWindowPos(hRegDlg, NULL, 425, 0, 600, 515,0 );
			SetWindowPos(hIopDlg, NULL, 0  ,515,600,230,0);
            RefreshDebugger();
			

			
			RefreshIOPDebugger();
            return TRUE;

        case WM_VSCROLL:
				
                  switch ((int) LOWORD(wParam))
                 {

                 case SB_LINEDOWN: DebuggerPC += 0x00000004; RefreshDebugAll(); break;
                 case SB_LINEUP:   DebuggerPC -= 0x00000004; RefreshDebugAll(); break;
                 case SB_PAGEDOWN: DebuggerPC += 0x00000074; RefreshDebugAll(); break;
                 case SB_PAGEUP:   DebuggerPC -= 0x00000074; RefreshDebugAll(); break;
				 }
	



            return TRUE;
	

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_DEBUG_STEP:
					oldpc = psxRegs.pc;
                    Cpu->Step();
					while(oldpc == psxRegs.pc) Cpu->Step();
                    DebuggerPC = 0;
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_SKIP:
					cpuRegs.pc+= 4;
                    DebuggerPC = 0;
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_GO:
					for (;;) {
						if (HasBreakpoint()) {
							Cpu->Step();
							break;
						}
						Cpu->Step();
					}
                    DebuggerPC = 0;
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_LOG:
#ifdef PCSX2_DEVBUILD
					Log = 1 - Log;
#endif
                    return TRUE;

                case IDC_DEBUG_RESETTOPC:
                    DebuggerPC = 0;
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_JUMP:
                    jmpproc = MakeProcInstance((FARPROC)JumpProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_JUMP), debughWnd, (DLGPROC)jmpproc);
                    FreeProcInstance(jmpproc);
                    
                    RefreshDebugAll();
                    return TRUE;
				case IDC_CPUOP:
			
                    UpdateR5900op();
					return TRUE;

                case IDC_DEBUG_BP_EXEC:
                    bpexecproc = MakeProcInstance((FARPROC)BpexecProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_BPEXEC), debughWnd, (DLGPROC)bpexecproc);
                    FreeProcInstance(bpexecproc);
                    
                    return TRUE;

				case IDC_DEBUG_BP_COUNT:
                    bpcntproc = MakeProcInstance((FARPROC)BpcntProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_BPCNT), debughWnd, (DLGPROC)bpcntproc);
                    FreeProcInstance(bpcntproc);
					return TRUE;

                case IDC_DEBUG_BP_CLEAR:
					memset(bkpt_regv, 0, sizeof(bkpt_regv));
					return TRUE;

				case IDC_DEBUG_DUMP:
                    dumpproc = MakeProcInstance((FARPROC)DumpProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_DUMP), debughWnd, (DLGPROC)dumpproc);
                    FreeProcInstance(dumpproc);
					return TRUE;

				case IDC_DEBUG_MEMORY:
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MEMORY), debughWnd, (DLGPROC)MemoryProc);
					return TRUE;

                case IDC_DEBUG_CLOSE:

                    EndDialog(hRegDlg ,TRUE);                  
					EndDialog(hDlg,TRUE);
					EndDialog(hIopDlg,TRUE);

                    ClosePlugins();

                    return TRUE;
            }
            break;
    }

    return FALSE;
}

void RefreshDebugger(void)
{
    unsigned long t;
    int cnt;
    

    
    if (DebuggerPC == 0) 
       DebuggerPC = cpuRegs.pc; //- 0x00000038;

    SendMessage(hWnd_debugdisasm, LB_RESETCONTENT, 0, 0);

    for (t = DebuggerPC, cnt = 0; t < (DebuggerPC + 0x00000074); t += 0x00000004, cnt++)
    {
		// Make the opcode.
		u32 *mem = (u32*)PSM(t);
		char *str;
		if (mem == NULL) {
			char nullAddr[256];
			sprintf(nullAddr, "%8.8lx 00000000: NULL MEMORY", t); str = nullAddr;
		} else {
			str = disR5900Fasm(*mem, t);
		}
        SendMessage(hWnd_debugdisasm, LB_ADDSTRING, 0, (LPARAM)str);
	}
    
}

void RefreshIOPDebugger(void)
{
    unsigned long t;
    int cnt;
	
    DebuggerIOPPC = psxRegs.pc; //- 0x00000038;

    SendMessage(hWnd_IOP_debugdisasm, LB_RESETCONTENT, 0, 0);

    for (t = DebuggerIOPPC, cnt = 0; t < (DebuggerIOPPC + 0x00000029); t += 0x00000004, cnt++)
    {
		// Make the opcode.
		u32 mem = PSXMu32(t);
		char *str = disR3000Fasm(mem, t);
        SendMessage(hWnd_IOP_debugdisasm, LB_ADDSTRING, 0, (LPARAM)str);
	}
    
}
