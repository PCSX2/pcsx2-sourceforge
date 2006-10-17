/*  SPU2null
 *  Copyright (C) 2002-2005  SPU2null Team
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

#include <stdio.h>
#include <string.h>

#include "SPU2.h"
#include "regs.h"

const unsigned char version  = PS2E_SPU2_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 5;    // increase that with each version

static char *libraryName      = "SPU2null Driver";

s8 *spu2regs;

#define spu2Rs16(mem)	(*(s16*)&spu2regs[(mem) & 0xffff])
#define spu2Ru16(mem)	(*(u16*)&spu2regs[(mem) & 0xffff])

void (CALLBACK *irqCallbackDMA4)()=0;                  // func of main emu, called on spu irq
void (CALLBACK *irqCallbackDMA7)()=0;                  // func of main emu, called on spu irq


u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_SPU2;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16)|(revision<<8)|build;
}

#ifdef __LINUX__

GtkWidget *MsgDlg;

void OnMsg_Ok() {
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void SysMessage(char *fmt, ...) {
	GtkWidget *Ok,*Txt;
	GtkWidget *Box,*Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;

	MsgDlg = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "SPU2null Msg");
	gtk_container_set_border_width(GTK_CONTAINER(MsgDlg), 5);

	Box = gtk_vbox_new(5, 0);
	gtk_container_add(GTK_CONTAINER(MsgDlg), Box);
	gtk_widget_show(Box);

	Txt = gtk_label_new(msg);
	
	gtk_box_pack_start(GTK_BOX(Box), Txt, FALSE, FALSE, 5);
	gtk_widget_show(Txt);

	Box1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(Box), Box1, FALSE, FALSE, 0);
	gtk_widget_show(Box1);

	Ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(MsgDlg);	

	gtk_main();
}

#endif


void __Log(char *fmt, ...) {
	va_list list;

	if (!conf.Log || spu2Log == NULL) return;

	va_start(list, fmt);
	vfprintf(spu2Log, fmt, list);
	va_end(list);
}

s32 CALLBACK SPU2init() {
#ifdef SPU2_LOG
	spu2Log = fopen("logs/spu2Log.txt", "w");
	if (spu2Log) setvbuf(spu2Log, NULL,  _IONBF, 0);
	SPU2_LOG("Spu2 null version %d,%d\n",revision,build);  
	SPU2_LOG("SPU2init\n");
#endif
	spu2regs = (s8*)malloc(0x10000);
	if (spu2regs == NULL) {
		SysMessage("Error allocating Memory\n"); return -1;
	}

	return 0;
}

s32 CALLBACK SPU2open(void *pDsp) {
	return 0;
}

void CALLBACK SPU2close() {
}

void CALLBACK SPU2shutdown() {
	free(spu2regs);
#ifdef SPU2_LOG
	if (spu2Log) fclose(spu2Log);
#endif
}

void CALLBACK SPU2async(u32 cycles) {
}

void CALLBACK SPU2readDMA4Mem(u16 *pMem, int size) {
	if(spu2Rs16(REG__1B0) != 1 && spu2Rs16(CORE0_ATTR) & 0x20) {
	irqCallbackDMA4();
	spu2Rs16(REG__1B0) = 0;
	}
}

void CALLBACK SPU2writeDMA4Mem(u16* pMem, int size) {
#ifdef SPU2_LOG
	SPU2_LOG("SPU2 writeDMA4Mem size %x\n", size);
#endif
	if(spu2Rs16(REG__1B0) != 1 && spu2Rs16(CORE0_ATTR) & 0x20) {
	irqCallbackDMA4();
	spu2Rs16(REG__1B0) = 0;
	}
}

void CALLBACK SPU2interruptDMA4() {
#ifdef SPU2_LOG
	SPU2_LOG("SPU2 interruptDMA4\n");
#endif
	spu2Rs16(CORE0_ATTR)&= ~0x30;
	spu2Rs16(REG__1B0) = 0;
	spu2Rs16(SPU2_STATX_WRDY_M)|= 0x80;
}

void CALLBACK SPU2readDMA7Mem(u16* pMem, int size) {
	if(spu2Rs16(REG__5B0) != 2 && spu2Rs16(CORE1_ATTR) & 0x1) {
	irqCallbackDMA7();
	spu2Rs16(REG__5B0) = 0;
	}
}

void CALLBACK SPU2writeDMA7Mem(u16* pMem, int size) {
#ifdef SPU2_LOG
	SPU2_LOG("SPU2 writeDMA7Mem size %x\n", size);
#endif
	if(spu2Rs16(REG__5B0) != 2 && spu2Rs16(CORE1_ATTR) & 0x1) {
	irqCallbackDMA7();
	//spu2Rs16(REG__5B0) = 0;
	}
}

void CALLBACK SPU2interruptDMA7() {
#ifdef SPU2_LOG
	SPU2_LOG("SPU2 interruptDMA7\n");
#endif
	spu2Rs16(CORE1_ATTR)&= ~0x30;
	//spu2Rs16(REG__5B0) = 0;
	spu2Rs16(SPU2_STATX_DREQ)|= 0x80;
}

void CALLBACK SPU2write(u32 mem, u16 value) {
#ifdef SPU2_LOG
	SPU2_LOG("SPU2 write mem %x value %x\n", mem, value);
#endif
	switch(mem) {
		case REG__1B0:
			spu2Rs16(SPU2_STATX_WRDY_M)&= ~0x80;
			spu2Rs16(SPU2_STATX_DREQ)&= ~0x80;
			spu2Ru16(mem) = value;
			break;

/*		case CORE1_ATTR: //that is needed else bios looping
			if (value & 0x20) {
				spu2Rs16(SPU2_STATX_WRDY_M) = -1;
				spu2Rs16(SPU2_STATX_DREQ) = -1;
			} else {
				spu2Rs16(SPU2_STATX_WRDY_M) = 0;
				spu2Rs16(SPU2_STATX_DREQ) = 0;
			}
			break;*/
		default:
			spu2Ru16(mem) = value;
			break;
	}
}

u16  CALLBACK SPU2read(u32 mem) {
	u16 ret=0;

	switch(mem) {
		default:
			ret = spu2Ru16(mem);
	}
#ifdef SPU2_LOG
	SPU2_LOG("SPU2 read mem %x: %x\n", mem, ret);
#endif	

	return ret;
}

/*void CALLBACK SPU2irqCallback(void (*callback)()) {
}*/

void CALLBACK SPU2DMA4irqCallback(void (CALLBACK *callback)(int))
{
 irqCallbackDMA4 = callback;
}

void CALLBACK SPU2DMA7irqCallback(void (CALLBACK *callback)(int))
{
 irqCallbackDMA7 = callback;
}
#ifdef __LINUX__
void CALLBACK SPU2configure() {
	SysMessage("Nothing to Configure");
}

void CALLBACK SPU2about() {
	SysMessage("%s %d.%d", libraryName, version, build);
}
#endif
s32 CALLBACK SPU2test() {
	return 0;
}

typedef struct {
	u8 spu2regs[0x10000];
} SPU2freezeData;

s32  CALLBACK SPU2freeze(int mode, freezeData *data){
	SPU2freezeData *spud;

	if (mode == FREEZE_LOAD) {
		spud = (SPU2freezeData*)data->data;
		memcpy(spu2regs, spud->spu2regs, 0x10000);
	} else
	if (mode == FREEZE_SAVE) {
		spud = (SPU2freezeData*)data->data;
		memcpy(spud->spu2regs, spu2regs, 0x10000);
	} else
	if (mode == FREEZE_SIZE) {
		data->size = sizeof(SPU2freezeData);
	}

	return 0;
}
