#include <stdlib.h>

#include "GS.h"

#define GetKeyV(name, var, s, t) \
	size = s; type = t; \
	RegQueryValueEx(myKey, name, 0, &type, (LPBYTE) var, &size);

#define GetKeyVdw(name, var) \
	GetKeyV(name, var, 4, REG_DWORD);

#define SetKeyV(name, var, s, t) \
	RegSetValueEx(myKey, name, 0, t, (LPBYTE) var, s);

#define SetKeyVdw(name, var) \
	SetKeyV(name, var, 4, REG_DWORD);

void SaveConfig() {
	HKEY myKey;
	DWORD myDisp;

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\PS2Eplugin\\GS\\GS SDL", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &myKey, &myDisp);

	SetKeyVdw("modeWidth", &conf.mode.width);
	SetKeyVdw("modeHeight", &conf.mode.height);
	SetKeyVdw("modeBpp", &conf.bpp);
	SetKeyVdw("fullscreen", &conf.fullscreen);
	SetKeyVdw("fps", &conf.fps);
	SetKeyVdw("frameskip", &conf.frameskip);
	SetKeyVdw("stretch", &conf.stretch);
//	SetKeyV("dddriver", &conf.dddriver, sizeof(conf.dddriver), REG_BINARY);

	RegCloseKey(myKey);
}

void LoadConfig() {
	HKEY myKey;
	DWORD type, size;

	memset(&conf, 0, sizeof(conf));
	conf.mode.width  = 640;
	conf.mode.height = 480;
	conf.bpp    = 16;
	conf.fullscreen  = 0;
	conf.stretch     = 1;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\PS2Eplugin\\GS\\GS SDL", 0, KEY_ALL_ACCESS, &myKey)!=ERROR_SUCCESS) {
		SaveConfig(); return;
	}

	GetKeyVdw("modeWidth", &conf.mode.width);
	GetKeyVdw("modeHeight", &conf.mode.height);
	GetKeyVdw("modeBpp", &conf.bpp);
	GetKeyVdw("fullscreen", &conf.fullscreen);
	GetKeyVdw("fps", &conf.fps);
	GetKeyVdw("frameskip", &conf.frameskip);
	GetKeyVdw("stretch", &conf.stretch);
//	GetKeyV("dddriver", &conf.dddriver, sizeof(conf.dddriver), REG_BINARY);

	RegCloseKey(myKey);
}

