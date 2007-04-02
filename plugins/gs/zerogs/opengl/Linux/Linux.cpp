/*  ZeroGS
 *  Copyright (C) 2002-2004  GSsoft Team
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <dlfcn.h>

#include "GS.h"

extern "C" {
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

#include "Linux.h"

#include <map>

static int prevbilinearfilter;
static map<string, int> mapConfOpts;

extern void OnKeyboardF5(int);
extern void OnKeyboardF6(int);
extern void OnKeyboardF7(int);
extern void OnKeyboardF9(int);

void CALLBACK GSkeyEvent(keyEvent *ev)
{
    static bool bShift = false;
    static bool bAlt = false;
    
    switch(ev->event) {
    case KEYPRESS:
        switch(ev->key) {
        case XK_F5:
            OnKeyboardF5(bShift);
            break;
        case XK_F6:
            OnKeyboardF6(bShift);
            break;
        case XK_F7:
            OnKeyboardF7(bShift);
            break;
        case XK_F9:
            OnKeyboardF9(bShift);
            break;
        case XK_Escape:
            break;
        case XK_Shift_L:
        case XK_Shift_R:
            bShift = true;
            break;
        case XK_Alt_L:
        case XK_Alt_R:
            bAlt = true;
            break;
        }
        break;
    case KEYRELEASE:
        switch(ev->key) {
        case XK_Shift_L:
        case XK_Shift_R:
            bShift = false;
            break;
        case XK_Alt_L:
        case XK_Alt_R:
            bAlt = false;
            break;
        }
    }
}

GtkWidget *Conf;
GtkWidget *Logging;
GList *fresl;
GList *wresl;
GList *cachesizel;
GList *codecl;
GList *filtersl;

void OnConf_Ok(GtkButton       *button, gpointer         user_data)
{
	GtkWidget *Btn;
	char *str;
	int i;

	u32 newinterlace = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkInterlace")));

	if( !conf.interlace ) conf.interlace = newinterlace;
	else if( !newinterlace ) conf.interlace = 2; // off

	conf.bilinear = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkBilinear")));
    // restore
    if( conf.bilinear && prevbilinearfilter )
        conf.bilinear = prevbilinearfilter;

	//conf.mrtdepth = 1;//IsDlgButtonChecked(hW, IDC_CONFIG_DEPTHWRITE);

	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioAANone"))) ) {
		conf.aa = 0;
	}
    else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioAA2X"))) ) {
		conf.aa = 1;
	}
	else conf.aa = 2;

	conf.options = 0;
	conf.options |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkAVI"))) ? GSOPTION_CAPTUREAVI : 0;
	conf.options |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkWireframe"))) ? GSOPTION_WIREFRAME : 0;
	conf.options |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton6"))) ? GSOPTION_FULLSCREEN : 0;
	conf.options |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkTGA"))) ? GSOPTION_TGASNAP : 0;

    conf.gamesettings = 0;
    for(map<string, int>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it) {
        GtkWidget* widget = lookup_widget(Conf, it->first.c_str());
        if( widget != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) )
            conf.gamesettings |= it->second;
    }
    GSsetGameCRC(0, conf.gamesettings);

    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize640"))) )
        conf.options |= GSOPTION_WIN640;
	else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize800"))) )
        conf.options |= GSOPTION_WIN800;
	else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize1024"))) )
        conf.options |= GSOPTION_WIN1024;
	else if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize1280"))) )
        conf.options |= GSOPTION_WIN1280;

    SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton       *button, gpointer         user_data) {
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

#define PUT_CONF(id) mapConfOpts["Opt"#id] = 0x##id

void CALLBACK GSconfigure()
{
	char name[32];
	int nmodes, i;

    if( !(conf.options & GSOPTION_LOADED) )
		LoadConfig();

	Conf = create_Config();
    
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkInterlace")), conf.interlace);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkBilinear")), !!conf.bilinear);
    //gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton6")), conf.mrtdepth);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioAANone")), conf.aa==0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioAA2X")), conf.aa==1);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioAA4X")), conf.aa==2);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkWireframe")), (conf.options&GSOPTION_WIREFRAME)?1:0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkAVI")), (conf.options&GSOPTION_CAPTUREAVI)?1:0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton6")), (conf.options&GSOPTION_FULLSCREEN)?1:0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkTGA")), (conf.options&GSOPTION_TGASNAP)?1:0);

    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize640")), ((conf.options&GSOPTION_WINDIMS)>>4)==0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize800")), ((conf.options&GSOPTION_WINDIMS)>>4)==1);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize1024")), ((conf.options&GSOPTION_WINDIMS)>>4)==2);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "radioSize1280")), ((conf.options&GSOPTION_WINDIMS)>>4)==3);

    prevbilinearfilter = conf.bilinear;

    mapConfOpts.clear();
    PUT_CONF(00000001);
    PUT_CONF(00000002);
    PUT_CONF(00000004);
    PUT_CONF(00000008);
    PUT_CONF(00000010);
    PUT_CONF(00000020);
    PUT_CONF(00000040);
    PUT_CONF(00000080);
    PUT_CONF(00000200);
    PUT_CONF(00000400);
    PUT_CONF(00000800);
    PUT_CONF(00001000);
    PUT_CONF(00002000);
    PUT_CONF(00004000);
    PUT_CONF(00008000);
    PUT_CONF(00010000);
    PUT_CONF(00020000);
    PUT_CONF(00040000);
    PUT_CONF(00080000);
    PUT_CONF(00100000);
    PUT_CONF(00200000);

    for(map<string, int>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it) {
        GtkWidget* widget = lookup_widget(Conf, it->first.c_str());
        if( widget != NULL )
            gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(widget), (conf.gamesettings&it->second)?1:0);   
    }

	gtk_widget_show_all(Conf);
	gtk_main();
}

GtkWidget *About;

void OnAbout_Ok(GtkButton       *button, gpointer         user_data) {
	gtk_widget_destroy(About);
	gtk_main_quit();
}

void CALLBACK GSabout() {

	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

s32 CALLBACK GStest() {
	return 0;
}

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

	MsgDlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "GSsoft Msg");
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

void *SysLoadLibrary(char *lib) {
	return dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
}

void *SysLoadSym(void *lib, char *sym) {
	void *ret = dlsym(lib, sym);
	if (ret == NULL) printf("null: %s\n", sym);
	return dlsym(lib, sym);
}

char *SysLibError() {
	return dlerror();
}

void SysCloseLibrary(void *lib) {
	dlclose(lib);
}
