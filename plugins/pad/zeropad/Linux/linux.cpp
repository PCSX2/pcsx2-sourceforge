/*  ZeroPAD - author: zerofrog(@gmail.com)
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

#ifdef JOYSTICK_SUPPORT
#include <linux/joystick.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#endif

#include "zeropad.h"

extern "C" {
#include "interface.h"
#include "support.h"
#include "callbacks.h"
}

Display *GSdsp;
static pthread_spinlock_t s_mutexStatus;
static u32 s_keyPress[2], s_keyRelease[2]; // thread safe

// holds all joystick info
class JoystickInfo
{
public:
    JoystickInfo();
    ~JoystickInfo() { Destroy(); }
    
    void Destroy(); 
    // opens handles to all possible joysticks
    static void EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks);
    
    bool Init(const char* pdev, int id, bool bStartThread=true); // opens a handle and gets information
    void Assign(int pad); // assigns a joystick to a pad
    void ProcessData(); // reads data from the joystick

    void TestForce();

    const string& GetName() { return devname; }
    int GetNumButtons() { return vbuttonstate.size(); }
    int GetButtonState(int ibut) { return vbuttonstate[ibut]; }
    int GetNumAxes() { return vaxes.size(); }
    int GetAxisState(int iaxis) { return vaxes[iaxis]; }
    int GetVersion() { return version; }
    int GetId() { return _id; }
    int GetPAD() { return pad; }
    
private:

#ifdef JOYSTICK_SUPPORT
    static void* pollthread(void* p);
    void* _pollthread();
#endif

    string devid; // device id
    string devname; // pretty device name
    int _id, js_fd;
    vector<int> vbuttonstate;
    vector<int> vaxes; // values of the axes
    int axisrange;
    int pad;
    int version;

    bool bTerminateThread;
    pthread_t pthreadpoll;
};

static vector<JoystickInfo*> s_vjoysticks;

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
		for (i=0; i<PADKEYS; i++) {
			fprintf(f, "[%d][%d] = 0x%lx\n", j, i, conf.keys[j][i]);
		}
	}
	fprintf(f, "log = %d\n", conf.log);
    fprintf(f, "options = %d\n", conf.options);
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
		for (i=0; i<PADKEYS; i++) {
			sprintf(str, "[%d][%d] = 0x%%x\n", j, i);
			fscanf(f, str, &conf.keys[j][i]);
		}
	}
    fscanf(f, "log = %d\n", &conf.log);
    fscanf(f, "options = %d\n", &conf.options);
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

    JoystickInfo::EnumerateJoysticks(s_vjoysticks);

	return 0;
}

void _PADclose()
{
    pthread_spin_destroy(&s_mutexStatus);
    XAutoRepeatOn(GSdsp);

    vector<JoystickInfo*>::iterator it;
    FORIT(it, s_vjoysticks) delete *it;
    s_vjoysticks.clear();
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

int _GetJoystickIdFromPAD(int pad)
{
    // select the right joystick id
    int joyid = -1;
    for(int i = 0; i < PADKEYS; ++i) {
        if( IS_JOYSTICK(conf.keys[pad][i]) || IS_JOYBUTTONS(conf.keys[pad][i]) ) {
            joyid = PAD_GETJOYID(conf.keys[pad][i]);
            break;
        }
    }

    return joyid;
}

void CALLBACK PADupdate(int pad)
{
    int i;
    XEvent E;
    int keyPress=0,keyRelease=0;
    KeySym key;

    // keyboard input
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

    // joystick info
    for(int i = 0; i < (int)s_vjoysticks.size(); ++i)
        s_vjoysticks[i]->ProcessData();
    
    pthread_spin_lock(&s_mutexStatus);
    s_keyPress[pad] |= keyPress;
    s_keyPress[pad] &= ~keyRelease;
    s_keyRelease[pad] |= keyRelease;
    s_keyRelease[pad] &= ~keyPress;
    pthread_spin_unlock(&s_mutexStatus);
}

static GtkWidget *Conf=NULL, *s_devicecombo=NULL;
static int s_selectedpad = 0;

void UpdateConf(int pad)
{
    s_selectedpad = pad;

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

        string tmp;
        if( IS_KEYBOARD(conf.keys[pad][i]) ) {
            char* pstr = XKeysymToString(PAD_GETKEY(conf.keys[pad][i]));
            if( pstr != NULL )
                tmp = pstr;
        }
        else if( IS_JOYBUTTONS(conf.keys[pad][i]) ) {
            tmp.resize(20);
            sprintf(&tmp[0], "JBut %d", PAD_GETJOYBUTTON(conf.keys[pad][i]));
        }
        else if( IS_JOYSTICK(conf.keys[pad][i]) ) {
            tmp.resize(20);
            sprintf(&tmp[0], "JAxis %d", PAD_GETJOYSTICK_AXIS(conf.keys[pad][i]));
        }
        else if( IS_POV(conf.keys[pad][i]) ) {
            tmp.resize(20);
            sprintf(&tmp[0], "JPOV %d%s", PAD_GETJOYSTICK_AXIS(conf.keys[pad][i]), PAD_GETPOVSIGN(conf.keys[pad][i])?"-":"+");
        }

        if (tmp.size() > 0) {
            gtk_entry_set_text(GTK_ENTRY(Btn), tmp.c_str());
        }
		else
			gtk_entry_set_text(GTK_ENTRY(Btn), "Unknown");

        gtk_object_set_user_data(GTK_OBJECT(Btn), (void*)(PADKEYS*pad+i));
	}

    // check bounds
    int joyid = _GetJoystickIdFromPAD(pad);
    if( joyid < 0 || joyid >= (int)s_vjoysticks.size() ) {
        // get first unused joystick
        for(joyid = 0; joyid < s_vjoysticks.size(); ++joyid) {
            if( s_vjoysticks[joyid]->GetPAD() < 0 )
                break;
        }
    }
    
    if( joyid >= 0 && joyid < (int)s_vjoysticks.size() ) {
        // select the combo
        gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), joyid);
    }
    else gtk_combo_box_set_active(GTK_COMBO_BOX(s_devicecombo), s_vjoysticks.size()); // no gamepad

    int padopts = conf.options>>(16*pad);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reverselx")), padopts&PADOPTION_REVERTLX);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reversely")), padopts&PADOPTION_REVERTLY);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reverserx")), padopts&PADOPTION_REVERTRX);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "checkbutton_reversery")), padopts&PADOPTION_REVERTRY);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(Conf, "forcefeedback")), padopts&PADOPTION_FORCEFEEDBACK);
}

void OnConf_Key(GtkButton *button, gpointer user_data)
{
    GdkEvent *ev;
    GtkWidget* label = lookup_widget(Conf, GetLabelFromButton(gtk_button_get_label(button)).c_str());
    if( label == NULL ) {
        printf("couldn't find correct label\n");
        return;
    }
    
    int id = (int)gtk_object_get_user_data(GTK_OBJECT(label));
    int pad = id/PADKEYS;
    int key = id%PADKEYS;
    unsigned long *pkey = &conf.keys[pad][key];

    vector<JoystickInfo*>::iterator it;

    for (;;) {
		ev = gdk_event_get();
		if (ev != NULL) {
	    	if (ev->type == GDK_KEY_PRESS) {
    		    *pkey = ev->key.keyval;

		    	char* tmp = XKeysymToString(*pkey);
		    	if (tmp != NULL)
					gtk_entry_set_text(GTK_ENTRY(label), tmp);
			    else
					gtk_entry_set_text(GTK_ENTRY(label), "Unknown");
				return;
			}
	    }

#ifdef JOYSTICK_SUPPORT
        FORIT(it, s_vjoysticks) {
            if( (*it)->GetPAD() == s_selectedpad ) {
                for(int i = 0; i < (*it)->GetNumButtons(); ++i) {
                    if( (*it)->GetButtonState(i) ) {
                        *pkey = PAD_JOYBUTTON((*it)->GetId(), i);
                        char str[32];
                        sprintf(str, "JBut %d", i);
                        gtk_entry_set_text(GTK_ENTRY(label), str);
                        return;
                    }
                }
                 
                for(int i = 0; i < (*it)->GetNumAxes(); ++i) {

                    if( abs((*it)->GetAxisState(i)) > 0x3fff ) {
                        if( key < 16 ) { // POV
                            *pkey = PAD_POV((*it)->GetId(), (*it)->GetAxisState(i)<0, i);
                            char str[32];
                            sprintf(str, "JPOV %d%s", i, (*it)->GetAxisState(i)<0?"-":"+");
                            gtk_entry_set_text(GTK_ENTRY(label), str);
                            return;
                        }
                        else { // axis
                            *pkey = PAD_JOYSTICK((*it)->GetId(), i);
                            char str[32];
                            sprintf(str, "JAxis %d", i);
                            gtk_entry_set_text(GTK_ENTRY(label), str);
                            return;
                        }
                    }
                }
            }
        }
#endif
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
    LoadConfig(); // load previous config
}

void CALLBACK PADconfigure()
{
	LoadConfig();

	Conf = create_Conf();

    // recreate
    JoystickInfo::EnumerateJoysticks(s_vjoysticks);

    s_devicecombo  = lookup_widget(Conf, "joydevicescombo");

    // fill the combo
    char str[255];
    vector<JoystickInfo*>::iterator it;
    FORIT(it, s_vjoysticks) {
        sprintf(str, "%d: %s - but: %d, axes: %d, ver: 0x%x", (*it)->GetId(), (*it)->GetName().c_str(),
                (*it)->GetNumButtons(), (*it)->GetNumAxes(), (*it)->GetVersion());
        gtk_combo_box_append_text (GTK_COMBO_BOX (s_devicecombo), str);
    }
    gtk_combo_box_append_text (GTK_COMBO_BOX (s_devicecombo), "No Gamepad");

	UpdateConf(0);

	gtk_widget_show_all(Conf);	
	gtk_main();
}

// GUI event handlers
void on_joydevicescombo_changed(GtkComboBox     *combobox, gpointer         user_data)
{
    int joyid = gtk_combo_box_get_active(combobox);

    // unassign every joystick with this pad
    for(int i = 0; i < (int)s_vjoysticks.size(); ++i) {
        if( s_vjoysticks[i]->GetPAD() == s_selectedpad )
            s_vjoysticks[i]->Assign(-1);
    }
    
    if( joyid >= 0 && joyid < (int)s_vjoysticks.size() )
        s_vjoysticks[joyid]->Assign(s_selectedpad);    
}

void on_checkbutton_reverselx_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTLX<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_checkbutton_reversely_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTLY<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_checkbutton_reverserx_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTRX<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_checkbutton_reversery_toggled(GtkToggleButton *togglebutton, gpointer         user_data)
{
    int mask = PADOPTION_REVERTRY<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) conf.options |= mask;
    else conf.options &= ~mask;
}

void on_forcefeedback_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    int mask = PADOPTION_REVERTLX<<(16*s_selectedpad);
    if( gtk_toggle_button_get_active(togglebutton) ) {
        conf.options |= mask;

        int joyid = gtk_combo_box_get_active(GTK_COMBO_BOX(s_devicecombo));
        if( joyid >= 0 && joyid < (int)s_vjoysticks.size() )
            s_vjoysticks[joyid]->TestForce();
    }
    else conf.options &= ~mask;
}

GtkWidget *About = NULL;

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

//////////////////////////
// Joystick definitions //
//////////////////////////

// opens handles to all possible joysticks
void JoystickInfo::EnumerateJoysticks(vector<JoystickInfo*>& vjoysticks)
{
#ifdef JOYSTICK_SUPPORT
    vector<JoystickInfo*>::iterator it;
    FORIT(it, vjoysticks) delete *it;
    vjoysticks.clear();
    
    char devid[32];
    JoystickInfo* pjoy = new JoystickInfo();
    for(int i = 0; i < 6; ++i) {
        sprintf(devid, "/dev/js%d", i);
        if( pjoy->Init(devid, i) ) {
            vjoysticks.push_back(pjoy);
            pjoy = new JoystickInfo();
        }
    }

    if( vjoysticks.size() == 0 ) {
        // try from /dev/input/
        for(int i = 0; i < 6; ++i) {
            sprintf(devid, "/dev/input/js%d", i);
            if( pjoy->Init(devid, i) ) {
                vjoysticks.push_back(pjoy);
                pjoy = new JoystickInfo();
            }
        }
    }

    delete pjoy;

    // set the pads
    for(int pad = 0; pad < 2; ++pad) {
        // select the right joystick id
        int joyid = -1;
        for(int i = 0; i < PADKEYS; ++i) {
            if( IS_JOYSTICK(conf.keys[pad][i]) || IS_JOYBUTTONS(conf.keys[pad][i]) ) {
                joyid = PAD_GETJOYID(conf.keys[pad][i]);
                break;
            }
        }

        if( joyid >= 0 && joyid < (int)s_vjoysticks.size() )
            s_vjoysticks[joyid]->Assign(pad);
    }

#endif
}

JoystickInfo::JoystickInfo()
{
    bTerminateThread = false;
    js_fd = -1;
    _id = -1;
    pad = -1;
    axisrange = 0x7fff;
}

void JoystickInfo::Destroy()
{
#ifdef JOYSTICK_SUPPORT
    if( js_fd >= 0 ) {
        bTerminateThread = true;
        void* ret;
        pthread_join(pthreadpoll, &ret);
        close(js_fd);
        js_fd = -1;
    }
#endif
}

bool JoystickInfo::Init(const char* pdevid, int id, bool bStartThread)
{
#ifdef JOYSTICK_SUPPORT
    assert(pdevid != NULL );
    Destroy();
    
    if ((js_fd = open(pdevid, O_RDWR)) < 0) {
        js_fd = -1;
        return false;
    }
  
    char name[256];
    ioctl(js_fd, JSIOCGVERSION, &version);

    if (version < 0x010000) {
        printf("joystick driver version is too old (%d)\n",version);
        return false;
    }

    char numaxes, numbuttons;
    ioctl(js_fd, JSIOCGAXES, &numaxes);
    ioctl(js_fd, JSIOCGBUTTONS, &numbuttons);
    ioctl(js_fd, JSIOCGNAME(256), name);

    // set to non-blocking
    int flags;
    if (-1 == (flags = fcntl(js_fd, F_GETFL, 0)))
        flags = 0;
    if( fcntl(js_fd, F_SETFL, flags | O_NONBLOCK) < 0 ) {
                printf("fcntl error\n");
        return false;
    }

    if( numaxes >= 0 ) vaxes.resize(numaxes);
    if( numbuttons >= 0 ) vbuttonstate.resize(numbuttons);
    
    devid = pdevid;
    devname = name;
    _id = id;

    if( bStartThread ) {
        // create thread
        if( pthread_create(&pthreadpoll, NULL, pollthread, this) ) {
            printf("failed to create thread\n");
            close(js_fd); js_fd = -1;
            return false;
        }
    }
    return true;
#else
    return false;
#endif
}

void JoystickInfo::ProcessData()
{
    if( pad < 0 || js_fd < 0 )
        return;

    for (int i=0; i<PADKEYS; i++) {
        int key = conf.keys[pad][i];
        if (IS_JOYBUTTONS(key) && PAD_GETJOYID(key) == _id) {
            if( vbuttonstate[PAD_GETJOYBUTTON(key)] )
                status[pad] &= ~(1<<i); // pressed
            else
                status[pad] |= (1<<i);
        }
        
        if (IS_JOYSTICK(key) && PAD_GETJOYID(key) == _id) {

            switch(i) {
                case PAD_LX:
                    g_lanalog[pad].x = vaxes[PAD_GETJOYSTICK_AXIS(key)]/256;
                    if( conf.options&PADOPTION_REVERTLX )
                        g_lanalog[pad].x = -g_lanalog[pad].x;
                    g_lanalog[pad].x += 128;
                    break;
                case PAD_LY:
                    g_lanalog[pad].y = vaxes[PAD_GETJOYSTICK_AXIS(key)]/256;
                    if( conf.options&PADOPTION_REVERTLY )
                        g_lanalog[pad].y = -g_lanalog[pad].y;
                    g_lanalog[pad].y += 128;
                    break;
                case PAD_RX:
                    g_ranalog[pad].x = vaxes[PAD_GETJOYSTICK_AXIS(key)]/256;
                    if( conf.options&PADOPTION_REVERTRX )
                        g_ranalog[pad].x = -g_ranalog[pad].x;
                    g_ranalog[pad].x += 128;
                    break;
                case PAD_RY:
                    g_ranalog[pad].y = vaxes[PAD_GETJOYSTICK_AXIS(key)]/256;
                    if( conf.options&PADOPTION_REVERTRY )
                        g_ranalog[pad].y = -g_ranalog[pad].y;
                    g_ranalog[pad].y += 128;
                    break;
            }
        }

        if( IS_POV(key) && PAD_GETJOYID(key) == _id) {
            if( PAD_GETPOVSIGN(key) && (vaxes[PAD_GETJOYSTICK_AXIS(key)]<-2048) )
                status[pad] &= ~(1<<i);
            else if( !PAD_GETPOVSIGN(key) && (vaxes[PAD_GETJOYSTICK_AXIS(key)]>2048) )
                status[pad] &= ~(1<<i);
            else
                status[pad] |= (1<<i);
        }
    }
}

// assigns a joystick to a pad
void JoystickInfo::Assign(int newpad)
{
    if( pad == newpad )
        return;

    pad = newpad;

    if( pad >= 0 ) {
        for(int i = 0; i < PADKEYS; ++i) {
            if( IS_JOYBUTTONS(conf.keys[pad][i]) ) {
                conf.keys[pad][i] = PAD_JOYBUTTON(_id, PAD_GETJOYBUTTON(conf.keys[pad][i]));
            }
            else if( IS_JOYSTICK(conf.keys[pad][i]) ) {
                conf.keys[pad][i] = PAD_JOYSTICK(_id, PAD_GETJOYBUTTON(conf.keys[pad][i]));
            }
        }
    }
}

void JoystickInfo::TestForce()
{
#ifdef JOYSTICK_SUPPORT
    // send a small force command
    //struct input_event play;
//	struct input_event stop;
//	struct ff_effect effect;
//	int fd, ret;
//
//	fd = open("/dev/input/js0", O_RDWR);
//    //fd = js_fd;
//
//    int features;
//    ret = ioctl(fd, EVIOCGBIT(EV_FF, sizeof(unsigned long)), &features);
//    printf("ret: %d, err: %d\n", ret, errno);
//    printf("features: %x\n", features);
//    
//    memset(&effect, 0, sizeof(effect));
//    effect.type = FF_RUMBLE;
//    effect.id = 1;
//    effect.u.rumble.weak_magnitude = 0x3fff;
//    effect.u.rumble.strong_magnitude = 0x3fff;
//
//    ret = ioctl(fd, EVIOCSFF, &effect);
//    printf("ret: %d, err: %d\n", ret, errno);
//    
//	/* Play three times */
//	play.type = EV_FF;
//	play.code = effect.id;
//	play.value = 3;
//	
//	ret = write(fd, (const void*) &play, sizeof(play));
//    printf("ret: %d, err: %d\n", ret, errno);

	/* Stop an effect */
//	stop.type = EV_FF;
//	stop.code = FF_RUMBLE;
//	stop.value = 0;
//	
//	write(fd, (const void*) &play, sizeof(stop));
//    close(fd);
#endif
}

#ifdef JOYSTICK_SUPPORT

void* JoystickInfo::pollthread(void* p)
{
    return ((JoystickInfo*)p)->_pollthread();
}

void* JoystickInfo::_pollthread()
{
    int ret;
    js_event js;
    while(!bTerminateThread) {
        usleep(5000); // 10ms

        if( js_fd < 0 ) {
            // try to recreate
            if( Init(devid.c_str(), _id, false) ) {
                printf("ZeroPAD: recreating joystick \"%s\"\n", devname.c_str());
            }
            else continue;
        }

        if ( (ret=read(js_fd, &js, sizeof(struct js_event))) != sizeof(struct js_event)) {

            if( errno == EBADF || errno == ENODEV ) {
                // close and recreate
                close(js_fd); js_fd = -1;
            }
            else if( errno != EINTR && errno != EAGAIN ) {
                printf("error in reading joystick data: %d\n", errno);
            }
            continue;
        }

        // ignore the startup events?
        if( js.type & JS_EVENT_INIT ) {
            continue;
        }
        
        if ((js.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
            vbuttonstate[js.number] = js.value;
        
        if( js.type == JS_EVENT_AXIS )
            vaxes[js.number] = js.value;
    }

    return NULL;
}

#endif
