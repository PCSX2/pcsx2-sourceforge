/*  ZeroPAD
 *  Copyright (C) 2006-2007 
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

#include <string.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "resource.h"
#include "zeropad.h"

#include <string>
using namespace std;

HINSTANCE hInst=NULL;

void SaveConfig()
{
    char *szTemp;
	char szIniFile[256], szValue[256], szProf[256];
    int i, j;

	GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return;
	strcpy(szTemp, "\\inis\\zeropad.ini");

	for (j=0; j<2; j++) {
		for (i=0; i<PADKEYS; i++) {
            sprintf(szProf, "%d_%d", j, i);
			sprintf(szValue, "%d", conf.keys[j][i]);
            WritePrivateProfileString("Interface", szProf,szValue,szIniFile);
		}
	}

    sprintf(szValue,"%u",conf.log);
	WritePrivateProfileString("Interface", "Logging",szValue,szIniFile);
}

void LoadConfig()
{
    FILE *fp;
	char *szTemp;
	char szIniFile[256], szValue[256], szProf[256];
	int i, j;

    memset(&conf, 0, sizeof(conf));
#ifdef _WIN32
    conf.keys[0][0] = 'W';			// L2
    conf.keys[0][1] = 'O';			// R2
    conf.keys[0][2] = 'A';			// L1
    conf.keys[0][3] = ';';			// R1
    conf.keys[0][4] = 'I';			// TRIANGLE
    conf.keys[0][5] = 'L';			// CIRCLE
    conf.keys[0][6] = 'K';			// CROSS
    conf.keys[0][7] = 'J';			// SQUARE
    conf.keys[0][8] = 'V';	// SELECT
    conf.keys[0][11] = 'N';	// START
    conf.keys[0][12] = 'E';	// UP
    conf.keys[0][13] = 'F';	// RIGHT
    conf.keys[0][14] = 'D';	// DOWN
    conf.keys[0][15] = 'S';	// LEFT
#endif
	conf.log = 0;

    GetModuleFileName(GetModuleHandle((LPCSTR)hInst), szIniFile, 256);
	szTemp = strrchr(szIniFile, '\\');

	if(!szTemp) return ;
	strcpy(szTemp, "\\inis\\zeropad.ini");
    fp=fopen("inis\\zeropad.ini","rt");//check if usbnull.ini really exists
	if (!fp) {
		CreateDirectory("inis",NULL); 
        SaveConfig();//save and return
		return ;
	}
	fclose(fp);

    for (j=0; j<2; j++) {
		for (i=0; i<PADKEYS; i++) {
            sprintf(szProf, "%d_%d", j, i);
            GetPrivateProfileString("Interface", szProf, NULL, szValue, 20, szIniFile);
            conf.keys[j][i] = strtoul(szValue, NULL, 10);
		}
	}

    GetPrivateProfileString("Interface", "Logging", NULL, szValue, 20, szIniFile);
    conf.log = strtoul(szValue, NULL, 10);
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "PADwinKeyb Msg", 0);
}

s32  _PADopen(void *pDsp)
{
	memset(&event, 0, sizeof(event));
	LoadConfig();
	
    return 0;
}

void _PADclose()
{
}

void _PADupdate(int pad)
{
}

string GetKeyLabel(const int pad, const int index)
{
	const int key = conf.keys[pad][index];
	char buff[16];
	if (key < 0x100)
	{
//		if (key == 0)
//			strcpy (buff, "NONE");
//        else {
//            return SDL_GetKeyName((SDLKey)key);
//        }
    }
	else if (key >= 0x1000 && key < 0x2000)
	{
		sprintf (buff, "J%d_%d", (key & 0xfff) / 0x100, (key & 0xff) + 1);
	}
	else if (key >= 0x2000 && key < 0x3000)
	{
		static const char name[][4] = { "MIN", "MAX" };
		const int axis = (key & 0xff);
		sprintf (buff, "J%d_AXIS%d_%s", (key & 0xfff) / 0x100, axis / 2, name[axis % 2]);
		if (index >= 17 && index <= 20)
			buff[strlen (buff) -4] = '\0';
	}
	else if (key >= 0x3000 && key < 0x4000)
	{
		static const char name[][7] = { "FOWARD", "RIGHT", "BACK", "LEFT" };
		const int pov = (key & 0xff);
		sprintf (buff, "J%d_POV%d_%s", (key & 0xfff) / 0x100, pov /4, name[pov % 4]);
	}

    return buff;
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND hWC;
	TCITEM tcI; 
	int i,key, numkeys;
    u8* pkeyboard;
	static int disabled=0;
    static int padn=0;

	switch(uMsg) {
		case WM_INITDIALOG:
			LoadConfig();
            padn = 0;
            if (conf.log)        CheckDlgButton(hW, IDC_LOG, TRUE);

			for (i=0; i<PADKEYS; i++) {
				hWC = GetDlgItem(hW, IDC_L2 + i*2);
				Button_SetText(hWC, GetKeyLabel(padn, i).c_str());
			}

			hWC = GetDlgItem(hW, IDC_TABC);

			tcI.mask = TCIF_TEXT;
			tcI.pszText = "PAD 1";

			TabCtrl_InsertItem(hWC, 0, &tcI);

			tcI.mask = TCIF_TEXT;
			tcI.pszText = "PAD 2";

			TabCtrl_InsertItem(hWC, 1, &tcI);
			return TRUE;

		case WM_TIMER:
			if (disabled){
                key = 0;
                //pkeyboard = SDL_GetKeyState(&numkeys);
                for (int i = 0; i < numkeys; ++i) {
				    if( pkeyboard[i] ) {
                        key = i;
                        break;
                    }
				}
    
                if( key == 0 ) {
                    // check joystick
                }
                
				if (key != 0){
					KillTimer(hW, 0x80);
					hWC = GetDlgItem(hW, disabled);
                    conf.keys[padn][disabled-IDC_L2] = key;
                    Button_SetText(hWC, GetKeyLabel(padn, disabled-IDC_L2).c_str());
					EnableWindow(hWC, TRUE);
					disabled=0;
					return TRUE;
				}
            }
			return TRUE;

		case WM_COMMAND:
			for(i = IDC_L2; i <= IDC_LEFT; i+=2)
			{
				if(LOWORD(wParam) == i)
				{
				  if (disabled)//change selection
					EnableWindow(GetDlgItem(hW, disabled), TRUE);

				  EnableWindow(GetDlgItem(hW, disabled=wParam), FALSE);
				
				  SetTimer(hW, 0x80, 250, NULL);
				
				  return TRUE;
				}
			}

			switch(LOWORD(wParam)) {
				case IDCANCEL:
					KillTimer(hW, 0x80);
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					KillTimer(hW, 0x80);
					if (IsDlgButtonChecked(hW, IDC_LOG))
						 conf.log = 1;
					else conf.log = 0;
					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
            break;

		case WM_NOTIFY:
			switch (wParam) {
				case IDC_TABC:
					hWC = GetDlgItem(hW, IDC_TABC);
					padn = TabCtrl_GetCurSel(hWC);
					
					for (i=0; i<PADKEYS; i++) {
						hWC = GetDlgItem(hW, IDC_EL3 + i);
                        Button_SetText(hWC, GetKeyLabel(padn, i).c_str());
					}

					return TRUE;
			}
			return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK PADconfigure() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_DIALOG1),
              GetActiveWindow(),  
              (DLGPROC)ConfigureDlgProc);
}

void CALLBACK PADabout() {
	SysMessage("Author: zerofrog\nThanks to SSSPSXPad, TwinPAD, and PADwin plugins");
}

s32 CALLBACK PADtest() {
	return 0;
}

void CALLBACK PADupate()
{
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}
