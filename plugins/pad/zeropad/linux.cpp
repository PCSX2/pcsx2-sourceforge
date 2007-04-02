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
#include <gtk/gtk.h>
#include <pthread.h>

#include "zeropad.h"

extern "C" {
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

Display *GSdsp;
static pthread_spinlock_t s_mutexStatus;
static u32 s_keyPress[2], s_keyRelease[2];
extern u16 status[2];

extern string s_strIniPath;

void SaveConfig()
{
    int i, j;
    FILE *f;
    char cfg[255];

    strcpy(cfg, s_strIniPath.c_str());
    f = fopen(cfg,"w");
    if (f == NULL) {
        printf("ZeroPAD: failed to save ini %s\n", s_strIniPath.c_str());
        return;
    }

	for (j=0; j<2; j++) {
		for (i=0; i<16; i++) {
			fprintf(f, "[%d][%d] = 0x%lx\n", j, i, conf.keys[j][i]);
		}
	}
	fprintf(f, "log = %d\n", conf.log);
    fclose(f);
}

static char* s_pGuiKeyMap[] = { "L2", "R2", "L1", "R1",
                                "Triangle", "Circle", "Cross", "Square",
                                "Select", "L3", "R3", "Start",
                                "Up", "Right", "Down", "Left",
                                "Lx", "Rx", "Ly", "Ry" };

string GetLabelFromButton(const char* buttonname)
{
    string label = "e";
    label += buttonname;
    return label;
}

void LoadConfig() {
    FILE *f;
	char str[256];
    char cfg[255];
	int i, j;

    memset(&conf, 0, sizeof(conf));
	conf.keys[0][0] = XK_a;			// L2
	conf.keys[0][1] = XK_semicolon;			// R2
	conf.keys[0][2] = XK_w;			// L1
	conf.keys[0][3] = XK_p;			// R1
	conf.keys[0][4] = XK_i;			// TRIANGLE
	conf.keys[0][5] = XK_l;			// CIRCLE
	conf.keys[0][6] = XK_k;			// CROSS
	conf.keys[0][7] = XK_j;			// SQUARE
	conf.keys[0][8] = XK_v;	// SELECT
	conf.keys[0][11] = XK_n;// START
	conf.keys[0][12] = XK_e;	// UP
	conf.keys[0][13] = XK_f;	// RIGHT
	conf.keys[0][14] = XK_d;	// DOWN
	conf.keys[0][15] = XK_s;	// LEFT
	conf.log = 0;

    strcpy(cfg, s_strIniPath.c_str());
    f = fopen(cfg, "r");
    if (f == NULL) {
        printf("ZeroPAD: failed to load ini %s\n", s_strIniPath.c_str());
        SaveConfig();//save and return
        return;
    }

	for (j=0; j<2; j++) {
		for (i=0; i<16; i++) {
			sprintf(str, "[%d][%d] = 0x%%x\n", j, i);
			fscanf(f, str, &conf.keys[j][i]);
		}
	}
	fscanf(f, "log = %d\n", &conf.log);
    fclose(f);
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

s32  _PADopen(void *pDsp)
{
    GSdsp = *(Display**)pDsp;
    pthread_spin_init(&s_mutexStatus, PTHREAD_PROCESS_PRIVATE);
    s_keyPress[0] = s_keyPress[1] = 0;
    s_keyRelease[0] = s_keyRelease[1] = 0;
    XAutoRepeatOff(GSdsp);
	return 0;
}

void _PADclose()
{
    pthread_spin_destroy(&s_mutexStatus);
    XAutoRepeatOn(GSdsp);
}

void _PADupdate(int pad)
{
    pthread_spin_lock(&s_mutexStatus);
    status[pad] |= s_keyRelease[pad];
    status[pad] &= ~s_keyPress[pad];
    s_keyRelease[pad] = 0;
    s_keyPress[pad] = 0;
    pthread_spin_unlock(&s_mutexStatus);
}

void CALLBACK PADupdate(int pad)
{
    int i;
    XEvent E;
    int keyPress=0,keyRelease=0;
    KeySym key;

    while (XPending(GSdsp) > 0) {
		XNextEvent(GSdsp, &E);
		switch (E.type) {
        case KeyPress:
            //_KeyPress(pad, XLookupKeysym((XKeyEvent *)&E, 0)); break;
            key = XLookupKeysym((XKeyEvent *)&E, 0);
            for (i=0; i<PADKEYS; i++) {
                if (key == conf.keys[pad][i]) {
                    keyPress|=(1<<i);
                    keyRelease&=~(1<<i);
                    break;
                }
            }
            
            event.event = KEYPRESS;
            event.key = key;
            break;
        case KeyRelease:
            key = XLookupKeysym((XKeyEvent *)&E, 0);
            //_KeyRelease(pad, XLookupKeysym((XKeyEvent *)&E, 0));
            for (i=0; i<PADKEYS; i++) {
                if (key == conf.keys[pad][i]) {
                    keyPress&=~(1<<i);
                    keyRelease|= (1<<i);
                    break;
                }
            }
            
            event.event = KEYRELEASE;
            event.key = key;
            break;
            
        case FocusIn:
            XAutoRepeatOff(GSdsp);
            break;
            
        case FocusOut:
            XAutoRepeatOn(GSdsp);
            break;
		}
    }

    pthread_spin_lock(&s_mutexStatus);
    s_keyPress[pad] |= keyPress;
    s_keyPress[pad] &= ~keyRelease;
    s_keyRelease[pad] |= keyRelease;
    s_keyRelease[pad] &= ~keyPress;
    pthread_spin_unlock(&s_mutexStatus);
}

GtkWidget *Conf;
char name[32];

void UpdateConf(int pad)
{
	int i;
    GtkWidget *Btn;
	for (i=0; i<ARRAYSIZE(s_pGuiKeyMap); i++) {
        
        if( s_pGuiKeyMap[i] == NULL )
            continue;

		Btn = lookup_widget(Conf, GetLabelFromButton(s_pGuiKeyMap[i]).c_str());
        if( Btn == NULL ) {
            printf("ZeroPAD: cannot find key %s\n", s_pGuiKeyMap[i]);
            continue;
        }

        char* tmp=NULL;
        if( IS_KEYBOARD(conf.keys[pad][i]) ) {
            tmp = XKeysymToString(PAD_GETKEY(conf.keys[pad][i]));
        }

        if (tmp != NULL) {
            gtk_entry_set_text(GTK_ENTRY(Btn), tmp);
        }
		else
			gtk_entry_set_text(GTK_ENTRY(Btn), "Unknown");

        gtk_object_set_user_data(GTK_OBJECT(Btn), &conf.keys[pad][i]);
	}
}

void OnConf_Key(GtkButton *button, gpointer user_data)
{
    GdkEvent *ev;
    GtkWidget* label = lookup_widget(Conf, GetLabelFromButton(gtk_button_get_label(button)).c_str());
    if( label == NULL ) {
        printf("couldn't find correct label\n");
        return;
    }
    
    unsigned long *key = (unsigned long*)gtk_object_get_user_data(GTK_OBJECT(label));

    if( key == NULL ) {
        printf("wrong user data\n");
        return;
    }

    for (;;) {
		ev = gdk_event_get();
		if (ev != NULL) {
	    	if (ev->type == GDK_KEY_PRESS) {
    		    *key = ev->key.keyval;

		    	char* tmp = XKeysymToString(*key);
		    	if (tmp != NULL)
					gtk_entry_set_text(GTK_ENTRY(label), tmp);
			    else
					gtk_entry_set_text(GTK_ENTRY(label), "Unknown");
				return;
			}
	    }
    }
}

void OnConf_Pad1(GtkButton *button, gpointer user_data)
{
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) )
        UpdateConf(0);
}

void OnConf_Pad2(GtkButton *button, gpointer user_data)
{
    if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) )
        UpdateConf(1);
}

void OnConf_Ok(GtkButton *button, gpointer user_data)
{
//	conf.analog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Analog));
	SaveConfig();

	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void OnConf_Cancel(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(Conf);
	gtk_main_quit();
}

void CALLBACK PADconfigure() {
	LoadConfig();

	Conf = create_Conf();

//	Analog = lookup_widget(Conf, "GtkCheckButton_Analog");
//	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(Analog), conf.analog);

	UpdateConf(0);

	gtk_widget_show_all(Conf);	
	gtk_main();
}

GtkWidget *About;

void OnAbout_Ok(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(About);
	gtk_main_quit();
}

void CALLBACK PADabout() {

	About = create_About();

	gtk_widget_show_all(About);
	gtk_main();
}

s32 CALLBACK PADtest() {
	return 0;
}
