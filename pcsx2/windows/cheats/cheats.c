#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include "windows/resource.h"
#include "PS2Etypes.h"
#include "PS2Edefs.h"
#include "Memory.h"

#include "cheats.h"

char *sizenames[4]={"char","half","word","dword"};

char mtext[100];

HINSTANCE pInstance;

int TSelect;

bool Unsigned;
int Source;
int Compare;
int Size;
int CompareTo;

any CompareValue;

result *results;
int aresults;
int nresults;

//int mresults;

bool FirstSearch;

bool FirstShow;

char *olds;

char tn[100];
char to[100];
char tv[100];

#ifdef WIN32_VIRTUAL_MEM
char *mptr[2]={PS2MEM_BASE,PS2MEM_PSX};
#else
char *mptr[2];
extern s8 *psxM;
#endif

int  msize[2]={0x02000000,0x00200000};

int lticks;

HWND hWndFinder;

LVCOLUMN cols[3]={
	{LVCF_TEXT|LVCF_WIDTH,0,60,"Address",0,0,0},
	{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,60,"Old V",1,0,0},
	{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,60,"New V",2,0,0}
};

LVITEM   item[3]={
	{LVIF_TEXT|LVIF_STATE,0,0,0,0,tn,0,0,0,0},
	{LVIF_TEXT|LVIF_STATE,0,1,0,0,to,0,0,0,0},
	{LVIF_TEXT|LVIF_STATE,0,2,0,0,tv,0,0,0,0}
};

void DoEvents()
{
	MSG msg;
	while(PeekMessage(&msg,0,0,0,PM_REMOVE)!=0)
		DispatchMessage(&msg);
}

void UpdateStatus()
{
	int nticks=GetTickCount();
	if((nticks-lticks)>250)
	{
		int nshown=ListView_GetItemCount(GetDlgItem(hWndFinder,IDC_RESULTS));
		sprintf(mtext,"%d matches found (%d shown).",nresults,nshown);
		SetWindowText(GetDlgItem(hWndFinder,IDC_MATCHES),mtext);
		lticks=nticks;
		DoEvents();
	}
}

void SearchReset()
{
	if(olds) free(olds);
	olds=malloc(msize[Source]);
	memcpy(olds,mptr[Source],msize[Source]);
	FirstSearch=true;
	if(results) free(results);
	results=NULL;
	aresults=0;
	nresults=0;
}

int AddResult(u32 addr, any old)
{
	result*tr;
	result nr;

	//if(nresults>=32768) return;

	if(aresults==0)
	{
		nresults=0;
		aresults=256;
		results=(result*)malloc(sizeof(result)*aresults);
	}
	else if(nresults==aresults)
	{
		tr=results;
		aresults<<=1;
		results=(result*)malloc(sizeof(result)*aresults);
		if(results==0)
		{
			aresults>>=1;
			results=tr;
			MessageBox(hWndFinder,
				"Failed to allocate more memory to save the results: "
				"Search aborted.","Warning!",MB_OK|MB_ICONWARNING);
			return 0;
		}
		memcpy(results,tr,sizeof(result)*nresults);
		free(tr);
	}
	nr.address=addr;
	nr.oldval=old;
	results[nresults++]=nr;
	return 1;
}

bool CompareAny(any val,any cto)
{
	if(Unsigned) switch(Compare)
	{
		case 0: /* EQ */
			switch(Size){
				case 0:return val.vu8 ==cto.vu8;
				case 1:return val.vu16==cto.vu16;
				case 2:return val.vu32==cto.vu32;
				case 3:return val.vu64==cto.vu64;
				default:return false;
			}
		case 1: /* GT */
			switch(Size){
				case 0:return val.vu8 > cto.vu8;
				case 1:return val.vu16> cto.vu16;
				case 2:return val.vu32> cto.vu32;
				case 3:return val.vu64> cto.vu64;
				default:return false;
			}
		case 2: /* LT */
			switch(Size){
				case 0:return val.vu8 < cto.vu8;
				case 1:return val.vu16< cto.vu16;
				case 2:return val.vu32< cto.vu32;
				case 3:return val.vu64< cto.vu64;
				default:return false;
			}
		case 3: /* GE */
			switch(Size){
				case 0:return val.vu8 >=cto.vu8;
				case 1:return val.vu16>=cto.vu16;
				case 2:return val.vu32>=cto.vu32;
				case 3:return val.vu64>=cto.vu64;
				default:return false;
			}
		case 4: /* LE */
			switch(Size){
				case 0:return val.vu8 <=cto.vu8;
				case 1:return val.vu16<=cto.vu16;
				case 2:return val.vu32<=cto.vu32;
				case 3:return val.vu64<=cto.vu64;
				default:return false;
			}
		default:/* NE */
			switch(Size){
				case 0:return val.vu8 !=cto.vu8;
				case 1:return val.vu16!=cto.vu16;
				case 2:return val.vu32!=cto.vu32;
				case 3:return val.vu64!=cto.vu64;
				default:return false;
			}
	}
	else switch(Compare)
	{
		case 0: /* EQ */
			switch(Size){
				case 0:return val.vs8 ==cto.vs8;
				case 1:return val.vs16==cto.vs16;
				case 2:return val.vs32==cto.vs32;
				case 3:return val.vs64==cto.vs64;
				default:return false;
			}
		case 1: /* GT */
			switch(Size){
				case 0:return val.vs8 > cto.vs8;
				case 1:return val.vs16> cto.vs16;
				case 2:return val.vs32> cto.vs32;
				case 3:return val.vs64> cto.vs64;
				default:return false;
			}
		case 2: /* LT */
			switch(Size){
				case 0:return val.vs8 < cto.vs8;
				case 1:return val.vs16< cto.vs16;
				case 2:return val.vs32< cto.vs32;
				case 3:return val.vs64< cto.vs64;
				default:return false;
			}
		case 3: /* GE */
			switch(Size){
				case 0:return val.vs8 >=cto.vs8;
				case 1:return val.vs16>=cto.vs16;
				case 2:return val.vs32>=cto.vs32;
				case 3:return val.vs64>=cto.vs64;
				default:return false;
			}
		case 4: /* LE */
			switch(Size){
				case 0:return val.vs8 <=cto.vs8;
				case 1:return val.vs16<=cto.vs16;
				case 2:return val.vs32<=cto.vs32;
				case 3:return val.vs64<=cto.vs64;
				default:return false;
			}
		default:/* NE */
			switch(Size){
				case 0:return val.vs8 !=cto.vs8;
				case 1:return val.vs16!=cto.vs16;
				case 2:return val.vs32!=cto.vs32;
				case 3:return val.vs64!=cto.vs64;
				default:return false;
			}
	}
}
#define COMPAREOLD switch(Compare) { \
		case 0: /* EQ */ r=memcmp(&val,&cto,MSize)==0; break; \
		case 1: /* GT */ r=memcmp(&val,&cto,MSize)>0; break; \
		case 2: /* LT */ r=memcmp(&val,&cto,MSize)<0; break; \
		case 3: /* GE */ r=memcmp(&val,&cto,MSize)>=0; break; \
		case 4: /* LE */ r=memcmp(&val,&cto,MSize)<=0; break; \
		default:/* NE */ r=memcmp(&val,&cto,MSize)!=0; break; \
	}

void SearchFirst()
{
	int MSize=1<<Size;
	any*cur=(any*)mptr[Source];
	any cto=CompareValue;
	any val;
	any old;
	int addr;

	addr=0;
	while((addr+MSize)<msize[Source])
	{
		val.vu64=0;
		memcpy(&val,cur,MSize);			//update the buffer
		memcpy(&old,olds+addr,MSize);	//
		memcpy(olds+addr,cur,MSize);
		if(CompareTo==0)
		{
			cto=old;
		}

		if(CompareAny(val,cto)) {
			AddResult(addr,old);
			cur=(any*)(((char*)cur)+MSize);
			addr+=MSize;
			UpdateStatus();
		}
		else {
			cur=(any*)(((char*)cur)+1);
			addr+=1;
		}
	}
	FirstSearch=false;
}

void SearchMore()
{
	int i;
	int MSize=1<<Size;
	any cto=CompareValue;
	any val;
	any old;
	int addr;

	result*oldr=results;
	int noldr=nresults;

	results=NULL;
	aresults=0;
	nresults=0;

	for(i=0;i<noldr;i++)
	{
		val.vu64=0;
		addr=oldr[i].address;
		memcpy(&val,mptr[Source]+addr,MSize);
		memcpy(&old,olds+addr,MSize);
		memcpy(olds+addr,mptr[Source]+addr,MSize);
		if(CompareTo==0)
		{
			cto=old;
		}
		
		if(CompareAny(val,cto)) {
			AddResult(addr,old);
			UpdateStatus();
		}
	}
}

#define INIT_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,value?BST_UNCHECKED:BST_CHECKED,0)
#define HANDLE_CHECK(idc,hvar)	case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

#define HANDLE_GROUP_ITEM(idc)	case idc: 
#define BEGIN_GROUP_HANDLER(first,hvar) TSelect=wmId;hvar=TSelect-first
#define GROUP_SELECT(idc)	SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(TSelect==idc)?BST_CHECKED:BST_UNCHECKED,0)
#define GROUP_INIT(first,hvar) TSelect=first+hvar

BOOL CALLBACK AddCheatProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent,i,mresults;
	static HWND hParent;
	UINT state;
	any value;
	static int Selected;

	switch(uMsg)
	{

		case WM_PAINT:
			INIT_CHECK(IDC_UNSIGNED,Unsigned);
			return FALSE;
		case WM_INITDIALOG:
			hParent=(HWND)lParam;

			mresults=ListView_GetItemCount(GetDlgItem(hParent,IDC_RESULTS));
			for(i=0;i<mresults;i++)
			{
				state=ListView_GetItemState(GetDlgItem(hParent,IDC_RESULTS),i,LVIS_SELECTED);
				if(state==LVIS_SELECTED)
				{
					Selected=i;
					ListView_GetItemText(GetDlgItem(hParent,IDC_RESULTS),i,0,tn,100);
					ListView_GetItemText(GetDlgItem(hParent,IDC_RESULTS),i,0,tv,100);

					sprintf(to,"patch=0,%s,%s,%s,<value>",
						Source?"IOP":"EE",
						tn,
						sizenames[Size]);

					SetWindowText(GetDlgItem(hWnd,IDC_ADDR),tn);
					SetWindowText(GetDlgItem(hWnd,IDC_VALUE),tv);
					SetWindowText(GetDlgItem(hWnd,IDC_NAME),to);

					break;
				}
			}


			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDCANCEL:
					EndDialog(hWnd,1);
					break;
				
				case IDOK:
					GetWindowText(GetDlgItem(hWnd,IDC_VALUE),tv,100);
					value.vs64=_atoi64(tv);
					AddPatch(Source,results[Selected].address,Size,Unsigned,&value);

					EndDialog(hWnd,1);
					break;


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void AddCheat(HINSTANCE hInstance, HWND hParent)
{
	INT_PTR retret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_ADD),hParent,(DLGPROC)AddCheatProc,(LPARAM)hParent);
}

void AddResults(HWND hWnd)
{
	int i,mresults;
	u64 sizemask=(1<<(1<<Size))-1;

	if(nresults>32768) {
		mresults=32768;
	}
	else {
		mresults=nresults;
	}

	ListView_DeleteAllItems(GetDlgItem(hWnd,IDC_RESULTS));

	for(i=0;i<mresults;i++)
	{
		any o=results[i].oldval;
		any v=*(any*)(mptr[Source]+results[i].address);

		sprintf(tn,"%08x",results[i].address);

		if(Unsigned) 
		{
			sprintf(to,"%I64u",o.vu64&sizemask);
			sprintf(tv,"%I64u",v.vu64&sizemask);
		}
		else
		{
			switch(Size)
			{
				case 0:
					o.vs64=o.vs8;
					v.vs64=v.vs8;
					break;
				case 1:
					o.vs64=o.vs16;
					v.vs64=v.vs16;
					break;
				case 2:
					o.vs64=o.vs32;
					v.vs64=v.vs32;
					break;
			}
			sprintf(to,"%I64d",o.vs64);
			sprintf(tv,"%I64d",v.vs64);
		}

		item[0].iItem=i;
		item[1].iItem=i;
		item[2].iItem=i;

		//Listview Sample Data
		ListView_InsertItem(GetDlgItem(hWnd,IDC_RESULTS),&item[0]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_RESULTS),&item[1]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_RESULTS),&item[2]);
		UpdateStatus();
	}

	if(nresults>32768) {
		sprintf(mtext,"%d matches found (32768 shown).",nresults);
		SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),mtext);
	}
	else {
		sprintf(mtext,"%d matches found.",nresults);
		SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),mtext);
	}

}

BOOL CALLBACK FinderProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	LRESULT lStyle;

	switch(uMsg)
	{

		case WM_PAINT:
			INIT_CHECK(IDC_UNSIGNED,Unsigned);
			return FALSE;
		case WM_INITDIALOG:

#ifndef WIN32_VIRTUAL_MEM
			mptr[0]=psM;
			mptr[1]=psxM;
#endif


			hWndFinder=hWnd;

			ENABLE_CONTROL(IDC_VALUE,false);

			GROUP_INIT(IDC_EE,Source);
				GROUP_SELECT(IDC_EE);
				GROUP_SELECT(IDC_IOP);

			GROUP_INIT(IDC_OLD,CompareTo);
				GROUP_SELECT(IDC_OLD);
				GROUP_SELECT(IDC_SET);
				ENABLE_CONTROL(IDC_VALUE,(CompareTo!=0));

			GROUP_INIT(IDC_EQ,Compare);
				GROUP_SELECT(IDC_EQ);
				GROUP_SELECT(IDC_GT);
				GROUP_SELECT(IDC_LT);
				GROUP_SELECT(IDC_GE);
				GROUP_SELECT(IDC_LE);

			GROUP_INIT(IDC_8B,Size);
				GROUP_SELECT(IDC_8B);
				GROUP_SELECT(IDC_16B);
				GROUP_SELECT(IDC_32B);
				GROUP_SELECT(IDC_64B);

			INIT_CHECK(IDC_UNSIGNED,Unsigned);

			//Listview Init
			lStyle = SendMessage(GetDlgItem(hWnd,IDC_RESULTS), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
			SendMessage(GetDlgItem(hWnd,IDC_RESULTS), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle | LVS_EX_FULLROWSELECT);

			ListView_InsertColumn(GetDlgItem(hWnd,IDC_RESULTS),0,&cols[0]);
			ListView_InsertColumn(GetDlgItem(hWnd,IDC_RESULTS),1,&cols[1]);
			ListView_InsertColumn(GetDlgItem(hWnd,IDC_RESULTS),2,&cols[2]);

			if(FirstShow)
			{
				SearchReset();
				SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),"ready to search.");
				FirstShow=false;
			}
			else {
				AddResults(hWnd);
			}

			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDCANCEL:
					EndDialog(hWnd,1);
					break;
				
				case IDC_ADD:
					AddCheat(pInstance,hWnd);
					break;

				case IDC_RESET:
					ENABLE_CONTROL(IDC_EE,		true);
					ENABLE_CONTROL(IDC_IOP,		true);
					ENABLE_CONTROL(IDC_LRESULTS,true);
					ENABLE_CONTROL(IDC_STATUS,	true);
					ENABLE_CONTROL(IDC_UNSIGNED,true);
					ENABLE_CONTROL(IDC_8B,		true);
					ENABLE_CONTROL(IDC_16B,		true);
					ENABLE_CONTROL(IDC_32B,		true);
					ENABLE_CONTROL(IDC_64B,		true);
					SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),"ready to search.");
					SearchReset();
					ListView_DeleteAllItems(GetDlgItem(hWnd,IDC_RESULTS));
					break;

				case IDC_SEARCH:
					GetWindowText(GetDlgItem(hWnd,IDC_VALUE),mtext,100);
					CompareValue.vs64=atoi(mtext);
					ENABLE_CONTROL(IDC_SEARCH,	false);
					ENABLE_CONTROL(IDC_RESET,	false);
					ENABLE_CONTROL(IDC_ADD,		false);
					ENABLE_CONTROL(IDCANCEL,	false);
					if(FirstSearch) {
						ENABLE_CONTROL(IDC_EE,		false);
						ENABLE_CONTROL(IDC_IOP,		false);
						ENABLE_CONTROL(IDC_LRESULTS,false);
						ENABLE_CONTROL(IDC_STATUS,	false);
						ENABLE_CONTROL(IDC_UNSIGNED,false);
						ENABLE_CONTROL(IDC_8B,		false);
						ENABLE_CONTROL(IDC_16B,		false);
						ENABLE_CONTROL(IDC_32B,		false);
						ENABLE_CONTROL(IDC_64B,		false);
						SearchFirst();
					}
					else			SearchMore();
					
					AddResults(hWnd);

					ENABLE_CONTROL(IDC_SEARCH,	true);
					ENABLE_CONTROL(IDC_RESET,	true);
					ENABLE_CONTROL(IDC_ADD,		true);
					ENABLE_CONTROL(IDCANCEL,	true);

					break;

				HANDLE_CHECK(IDC_UNSIGNED,Unsigned);

				HANDLE_GROUP_ITEM(IDC_EE);
				HANDLE_GROUP_ITEM(IDC_IOP);
				BEGIN_GROUP_HANDLER(IDC_EE,Source);
					GROUP_SELECT(IDC_EE);
					GROUP_SELECT(IDC_IOP);
					break;

				HANDLE_GROUP_ITEM(IDC_OLD);
				HANDLE_GROUP_ITEM(IDC_SET);
				BEGIN_GROUP_HANDLER(IDC_OLD,CompareTo);
					GROUP_SELECT(IDC_OLD);
					GROUP_SELECT(IDC_SET);
					ENABLE_CONTROL(IDC_VALUE,(CompareTo!=0));
					break;

				HANDLE_GROUP_ITEM(IDC_EQ);
				HANDLE_GROUP_ITEM(IDC_GT);
				HANDLE_GROUP_ITEM(IDC_LT);
				HANDLE_GROUP_ITEM(IDC_GE);
				HANDLE_GROUP_ITEM(IDC_LE);
				BEGIN_GROUP_HANDLER(IDC_EQ,Compare);
					GROUP_SELECT(IDC_EQ);
					GROUP_SELECT(IDC_GT);
					GROUP_SELECT(IDC_LT);
					GROUP_SELECT(IDC_GE);
					GROUP_SELECT(IDC_LE);
					break;

				HANDLE_GROUP_ITEM(IDC_8B);
				HANDLE_GROUP_ITEM(IDC_16B);
				HANDLE_GROUP_ITEM(IDC_32B);
				HANDLE_GROUP_ITEM(IDC_64B);
				BEGIN_GROUP_HANDLER(IDC_8B,Size);
					GROUP_SELECT(IDC_8B);
					GROUP_SELECT(IDC_16B);
					GROUP_SELECT(IDC_32B);
					GROUP_SELECT(IDC_64B);
					break;

				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void ShowFinder(HINSTANCE hInstance, HWND hParent)
{
	INT_PTR ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_FINDER),hParent,(DLGPROC)FinderProc,1);
}
