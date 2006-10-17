/******************************************************************************
    GStaris - GS plugin for PS2 emulators < gstaris.ngemu.com        >
    Copyright (C) 2003 Absolute0          < absolutezero@ifrance.com >

    This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
******************************************************************************/

#include "..\GStaris.h"
#include <stdio.h>
#include <gl/glext.h>

#include "resource.h"

// .dll stuff

HINSTANCE hInstDLL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	hInstDLL = (HINSTANCE)hModule;
	return TRUE;
}

// Config stuff

s32 config_load()
{
	HKEY key;
	DWORD type,size=4;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\GS\\GStaris",0,KEY_ALL_ACCESS,&key)!=ERROR_SUCCESS)
		return -1;

	RegQueryValueEx(key,"config.video.width",0,&type,(LPBYTE)&config.video.width,&size);
	RegQueryValueEx(key,"config.video.height",0,&type,(LPBYTE)&config.video.height,&size);
	RegQueryValueEx(key,"config.video.bpp",0,&type,(LPBYTE)&config.video.bpp,&size);
	RegQueryValueEx(key,"config.video.fullscreen",0,&type,(LPBYTE)&config.video.fullscreen,&size);
	RegQueryValueEx(key,"config.video.force75hz",0,&type,(LPBYTE)&config.video.force75hz,&size);
	RegQueryValueEx(key,"config.video.ratio",0,&type,(LPBYTE)&config.video.ratio,&size);

	RegQueryValueEx(key,"config.video.antialiasing",0,&type,(LPBYTE)&config.video.antialiasing,&size);
	RegQueryValueEx(key,"config.video.filtering",0,&type,(LPBYTE)&config.video.filtering,&size);

	RegQueryValueEx(key,"config.speed.limit_on",0,&type,(LPBYTE)&config.speed.limit_on,&size);
	RegQueryValueEx(key,"config.speed.limit_to",0,&type,(LPBYTE)&config.speed.limit_to,&size);
	RegQueryValueEx(key,"config.itf.menu_visible",0,&type,(LPBYTE)&config.itf.menu_visible,&size);

	RegQueryValueEx(key,"config.log.logon",0,&type,(LPBYTE)&config.log.logon,&size);
	RegQueryValueEx(key,"config.log.isoc",0,&type,(LPBYTE)&config.log.isoc,&size);
	RegQueryValueEx(key,"config.log.gif",0,&type,(LPBYTE)&config.log.gif,&size);
	RegQueryValueEx(key,"config.log.idt",0,&type,(LPBYTE)&config.log.idt,&size);
	RegQueryValueEx(key,"config.log.prim",0,&type,(LPBYTE)&config.log.prim,&size);
	RegQueryValueEx(key,"config.log.registers",0,&type,(LPBYTE)&config.log.registers,&size);
	RegQueryValueEx(key,"config.log.pregisters",0,&type,(LPBYTE)&config.log.pregisters,&size);
	RegQueryValueEx(key,"config.log.textures",0,&type,(LPBYTE)&config.log.textures,&size);
	RegQueryValueEx(key,"config.log.texturesbuffer",0,&type,(LPBYTE)&config.log.texturesbuffer,&size);

	RegQueryValueEx(key,"config.fixes.fbfix",0,&type,(LPBYTE)&config.fixes.fbfix,&size);
	RegQueryValueEx(key,"config.fixes.missing_stuff",0,&type,(LPBYTE)&config.fixes.missing_stuff,&size);
	RegQueryValueEx(key,"config.fixes.disable_blend",0,&type,(LPBYTE)&config.fixes.disable_blend,&size);
	RegQueryValueEx(key,"config.fixes.disable_idt_onscreen",0,&type,(LPBYTE)&config.fixes.disable_idt_onscreen,&size);

	config.itf.menu_index   = 0;

	RegCloseKey(key);

	return 0;
}

s32 config_save()
{
	HKEY key;
	DWORD disp;

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\PS2Eplugin\\GS\\GStaris",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&key,&disp);

	RegSetValueEx(key,"config.video.width",0,REG_DWORD,(LPBYTE)&config.video.width,4);
	RegSetValueEx(key,"config.video.height",0,REG_DWORD,(LPBYTE)&config.video.height,4);
	RegSetValueEx(key,"config.video.bpp",0,REG_DWORD,(LPBYTE)&config.video.bpp,4);
	RegSetValueEx(key,"config.video.fullscreen",0,REG_DWORD,(LPBYTE)&config.video.fullscreen,4);
	RegSetValueEx(key,"config.video.force75hz",0,REG_DWORD,(LPBYTE)&config.video.force75hz,4);
	RegSetValueEx(key,"config.video.ratio",0,REG_DWORD,(LPBYTE)&config.video.ratio,4);

	RegSetValueEx(key,"config.video.antialiasing",0,REG_DWORD,(LPBYTE)&config.video.antialiasing,4);
	RegSetValueEx(key,"config.video.filtering",0,REG_DWORD,(LPBYTE)&config.video.filtering,4);

	RegSetValueEx(key,"config.speed.limit_on",0,REG_DWORD,(LPBYTE)&config.speed.limit_on,4);
	RegSetValueEx(key,"config.speed.limit_to",0,REG_DWORD,(LPBYTE)&config.speed.limit_to,4);
	RegSetValueEx(key,"config.itf.menu_visible",0,REG_DWORD,(LPBYTE)&config.itf.menu_visible,4);

	RegSetValueEx(key,"config.log.logon",0,REG_DWORD,(LPBYTE)&config.log.logon,4);
	RegSetValueEx(key,"config.log.isoc",0,REG_DWORD,(LPBYTE)&config.log.isoc,4);
	RegSetValueEx(key,"config.log.gif",0,REG_DWORD,(LPBYTE)&config.log.gif,4);
	RegSetValueEx(key,"config.log.idt",0,REG_DWORD,(LPBYTE)&config.log.idt,4);
	RegSetValueEx(key,"config.log.prim",0,REG_DWORD,(LPBYTE)&config.log.prim,4);
	RegSetValueEx(key,"config.log.registers",0,REG_DWORD,(LPBYTE)&config.log.registers,4);
	RegSetValueEx(key,"config.log.pregisters",0,REG_DWORD,(LPBYTE)&config.log.pregisters,4);
	RegSetValueEx(key,"config.log.textures",0,REG_DWORD,(LPBYTE)&config.log.textures,4);
	RegSetValueEx(key,"config.log.texturesbuffer",0,REG_DWORD,(LPBYTE)&config.log.texturesbuffer,4);

	RegSetValueEx(key,"config.fixes.fbfix",0,REG_DWORD,(LPBYTE)&config.fixes.fbfix,4);
	RegSetValueEx(key,"config.fixes.missing_stuff",0,REG_DWORD,(LPBYTE)&config.fixes.missing_stuff,4);
	RegSetValueEx(key,"config.fixes.disable_blend",0,REG_DWORD,(LPBYTE)&config.fixes.disable_blend,4);
	RegSetValueEx(key,"config.fixes.disable_idt_onscreen",0,REG_DWORD,(LPBYTE)&config.fixes.disable_idt_onscreen,4);

	RegCloseKey(key);

	return 0;
}

void config_default()
{
	config.video.width      = 640;
	config.video.height     = 480;
	config.video.bpp        = 32;
	config.video.fullscreen = 0;
	config.video.force75hz  = 0;
	config.video.ratio      = 1;

	config.video.antialiasing = 0;
	config.video.filtering    = 0;

	config.speed.limit_on   = 1;
	config.speed.limit_to   = 60;

	config.itf.menu_visible = 0;
	config.itf.menu_index   = 0;

	config.fixes.fbfix = 0;
	config.fixes.missing_stuff = 0;
	config.fixes.disable_blend = 0;
	config.fixes.disable_idt_onscreen = 0;

	config.log.isoc           = 0;
	config.log.gif            = 0;
	config.log.idt            = 0;
	config.log.prim           = 0;
	config.log.registers      = 0;
	config.log.pregisters     = 0;
	config.log.textures       = 0;
	config.log.texturesbuffer = 0;
}

// Message-window management

HWND GSmsgWnd = NULL;

LRESULT CALLBACK GSmsgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_CLOSE:
		case WM_QUIT:
			exit(0);
			return TRUE;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return FALSE;
}

void* GScreateMsgWindow()
{
	WNDCLASS wc;

	if (GSmsgWnd) GSdestroyMsgWindow();

	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hbrBackground = NULL;
	wc.hCursor       = NULL;
	wc.hIcon         = NULL;
	wc.hInstance     = hInstDLL;
	wc.lpfnWndProc   = GSmsgWindowProc;
	wc.lpszClassName = "GStaris.msg_wnd";
	wc.lpszMenuName  = NULL;
	wc.style         = 0;
	RegisterClass(&wc);

	GSmsgWnd = CreateWindow("GStaris.msg_wnd","GStaris.msg_wnd",WS_OVERLAPPED|WS_SYSMENU,0,0,1,1,NULL,NULL,wc.hInstance,NULL);
	UnregisterClass("GStaris.msg_wnd",wc.hInstance);
	if (!GSmsgWnd) return NULL;

	ShowWindow(GSmsgWnd,SW_HIDE);

	return GSmsgWnd;
}

void GSdestroyMsgWindow()
{
	DestroyWindow(GSmsgWnd);
	GSmsgWnd = NULL;
}

// Window management

HWND GSwnd = NULL;

LRESULT CALLBACK GSwindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_CLOSE:
		case WM_QUIT:
			exit(0);
			return TRUE;

		case WM_MOVE:
			GSupdateView();
			break;

		case WM_CHAR:
		case WM_KEYDOWN:
		case WM_KEYUP:
			SendMessage(GSmsgWnd,uMsg,wParam,lParam);
			break;

	    default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return FALSE;
}

s32 GScreateWindow(char* title)
{
	WNDCLASS wc;
	s32 exstyle,style,x,y,w,h;
	char *caption;

	if (GSwnd) GSdestroyWindow();

	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hbrBackground = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
	wc.hInstance     = hInstDLL;
	wc.lpfnWndProc   = GSwindowProc;
	wc.lpszClassName = NAME;
	wc.lpszMenuName  = NULL;
	wc.style         = CS_OWNDC;
	RegisterClass(&wc);

	if (config.video.fullscreen) {
		exstyle = WS_EX_APPWINDOW|WS_EX_TOPMOST;
		style   = WS_POPUP;
		x       = 0;
		y       = 0;
		w       = config.video.width;
		h       = config.video.height;
	} else {
		exstyle = 0;
		style   = WS_OVERLAPPED|WS_SYSMENU;
		x       = GetSystemMetrics(SM_CXSCREEN)/2 - config.video.width/2;
		y       = GetSystemMetrics(SM_CYSCREEN)/2 - config.video.height/2;
		w       = config.video.width  + GetSystemMetrics(SM_CXFIXEDFRAME)*2;
		h       = config.video.height + GetSystemMetrics(SM_CYFIXEDFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
	}

	caption = (char*)malloc(strlen(title)+strlen(NAME)+4);
	sprintf(caption, "%s * %s", title, NAME);

	GSwnd = CreateWindowEx(exstyle,NAME,caption,style,x,y,w,h,NULL,NULL,wc.hInstance,NULL);
	free(caption);
	UnregisterClass(NAME,wc.hInstance);

	if (!GSwnd) return -1;

	ShowWindow(GSwnd, SW_SHOWNORMAL);
	UpdateWindow(GSwnd);

	return 0;
}

void GSdestroyWindow()
{
	DestroyWindow(GSwnd);
	GSwnd = NULL;
}

s32 GSshowWindow()
{
	ShowWindow(GSwnd, SW_SHOWNORMAL);
	UpdateWindow(GSwnd);

	if (config.video.fullscreen)
		return GSfullscreenGL();

	return 0;
}
void GShideWindow()
{
	ShowWindow(GSwnd, SW_HIDE);

	if (config.video.fullscreen) {
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
}

// OpenGL management

HDC   hdc = NULL;
HGLRC hrc = NULL;

s32   font;

s32 GScreateGL()
{
	PIXELFORMATDESCRIPTOR pfd;
	s32                   pixelformat;
	HFONT                 hFont;
	const GLubyte         *extstrGL;
	WORD ramp[3*256];

	hdc = GetDC(GSwnd);
	if (!hdc) return -1;

	DescribePixelFormat(hdc, GetPixelFormat(hdc), sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_DRAW_TO_WINDOW;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits   = (unsigned char)config.video.bpp;
	pfd.cDepthBits   = (unsigned char)config.video.bpp;
	pfd.cStencilBits = 0;
	pfd.cAccumBits   = 0;
	pfd.cAuxBuffers  = 0;

//	if (config.video.fullscreen) pfd.dwFlags |= PFD_SWAP_COPY;

	pixelformat = 0;
	if (!(pixelformat = ChoosePixelFormat(hdc, &pfd))) return -1;
	if (!SetPixelFormat(hdc, pixelformat, &pfd))       return -1;

	if (!(hrc = wglCreateContext(hdc))) return -1;
	wglMakeCurrent(hdc,hrc);

	// Build font

	font  = glGenLists(96);
	hFont = CreateFont(-14,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE|DEFAULT_PITCH,"Courier New");
	SelectObject(hdc,hFont);
	wglUseFontBitmaps(hdc,32,96,font);

	// Load OpenGL extensions

	extstrGL = glGetString(GL_EXTENSIONS);
	InitBlendColor();
	InitBlendEquation();

	// Default OpenGL values

	glDrawBuffer(GL_BACK);

	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glClearColor(0,0,0,1);
	glClearDepth(0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glDepthRange(0,1);

	glViewport(0,0,config.video.width,config.video.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,config.video.width,config.video.height,0,-1,1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	UpdateWindow(GSwnd);

	return 0;
}

void GSdestroyGL()
{
	if (config.video.fullscreen) {
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
	glDeleteLists(font,96);
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(hrc);
	ReleaseDC(GSwnd,hdc);
}

s32 GSfullscreenGL()
{
	DEVMODE settings;

	ShowCursor(FALSE);

	memset(&settings,0,sizeof(settings));
	settings.dmSize       = sizeof(settings);
	settings.dmPelsWidth  = config.video.width;
	settings.dmPelsHeight = config.video.height;
	settings.dmBitsPerPel = config.video.bpp;
	settings.dmFields     = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

	if (config.video.force75hz) {
		settings.dmDisplayFrequency = 75;
		settings.dmFields |= DM_DISPLAYFREQUENCY;
	}

	if (ChangeDisplaySettings(&settings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) return -1;

	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers(hdc);
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers(hdc);

	return 0;
}

void GSprint(const char* format, ...)
{
	char text[256];
	va_list ap;

	if (format == NULL) return;

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(0,0,0,1);
	glRecti(0,0,config.video.width,14);

	glColor4f(0,0,1,1);
	glPixelZoom(1,1);
	glRasterPos2f(0,10);

	va_start(ap,format);
	vsprintf(text,format,ap);
	va_end(ap);

	glListBase(font-32);
	glCallLists(strlen(text),GL_UNSIGNED_BYTE,text);

	glPopMatrix();
	glPopAttrib();
}

// Vsync

void GSupdateGL(s32 render)
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (!render) return;

	fpsc2++;

	if (config.itf.menu_visible) {
		switch (config.itf.menu_index) {
			case 0:
				GSprint("%s * fps=%d framec=%d fps2=%d t=%d fbfix=%d log=%x",NAME,fps,framec,fps2,tCount+1,config.fixes.fbfix,config.log.logon);
				break;
			case 1:
				GSprint("alpha a=%d b=%d c=%d d=%d fix=%d",alpha->a,alpha->b,alpha->c,alpha->d,alpha->fix);
				break;
			case 2: // display & config.video
				GSprint("display=%dx%d - config[%dx%d]",display->w,display->h,config.video.width,config.video.height);
				break;
			case 3: // texture buffer status
				if (config.itf.texture_view == -1) {
					GSprint("tCount=%d texture_view=%d",tCount,config.itf.texture_view);
				} else {
					switch (config.itf.texture_info) {
						case 0:
							GSprint("tCount=%d texture_view=%d name=%d tbp0=%x tbw=%x",tCount,config.itf.texture_view,textures[config.itf.texture_view].name,textures[config.itf.texture_view].tbp0,textures[config.itf.texture_view].tbw);
							break;
						case 1:
							GSprint("tCount=%d texture_view=%d tw=%d th=%d tcc=%x tfx=%x",tCount,config.itf.texture_view,textures[config.itf.texture_view].tw,textures[config.itf.texture_view].th,textures[config.itf.texture_view].tcc,textures[config.itf.texture_view].tfx);
							break;
						case 2:
							GSprint("tCount=%d texture_view=%d psm=%x csm=%x cpsm=%x",tCount,config.itf.texture_view,textures[config.itf.texture_view].psm,textures[config.itf.texture_view].csm,textures[config.itf.texture_view].cpsm);
							break;
						case 3:
							GSprint("tCount=%d texture_view=%d cbp=%x csa=%x cld=%x",tCount,config.itf.texture_view,textures[config.itf.texture_view].cbp,textures[config.itf.texture_view].csa,textures[config.itf.texture_view].cld);
							break;
						default:
							GSprint("tCount=%d texture_view=%d",tCount,config.itf.texture_view);
							break;
					}
				}
				break;
		}
	}

	if (config.itf.texture_view > -1) {
		ShowTexture();
	}

//	glFlush();

	SwapBuffers(hdc);

	if (snap_frame) {
		GSsnapFrame();
		GSsnapVram();
		snap_frame = 0;
	}

	glClearDepth(0);
	glClear(GL_DEPTH_BUFFER_BIT);
}

// OpenGL extensions management

void CALLBACK default_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
}

void CALLBACK default_glBlendEquation(GLenum mode)
{
}

void APIENTRY InitBlendColor()
{
	void *extproc;

	extproc = (void *) wglGetProcAddress("glBlendColor");

	if (extproc == NULL) {
		glBlendColor = (void *)default_glBlendColor;
		return;
	}

	glBlendColor = extproc;

	glBlendColor(1.f,1.f,1.f,1.f);
}

void APIENTRY InitBlendEquation()
{
	void *extproc;

	extproc = (void *) wglGetProcAddress("glBlendEquation");

	if (extproc == NULL) {
		glBlendEquation = (void *)default_glBlendEquation;
		return;
	}

	glBlendEquation = extproc;

	glBlendEquation(GL_FUNC_ADD);
}

// Extended funcs
// All the others stuff

void CALLBACK GSkeyEvent(keyEvent *ev)
{
	if (ev->event != KEYPRESS) return;

	switch (ev->key) {
		case VK_DELETE:
			config.fixes.fbfix++;
			if (config.fixes.fbfix > 2)
				config.fixes.fbfix = 0;
			break;

		case VK_INSERT:
			config.itf.menu_visible = 1-config.itf.menu_visible;
			break;

		case VK_PRIOR:
			if (config.itf.menu_visible) {
				config.itf.menu_index++;
				if (config.itf.menu_index > 3)
					config.itf.menu_index = 0;
			}
			break;

		case VK_NEXT:
			if (config.itf.menu_visible) {
				config.itf.menu_index--;
				if (config.itf.menu_index < 0)
					config.itf.menu_index = 3;
			}
			break;

		case VK_HOME:
			config.log.logon = 1-config.log.logon;
			break;

		case VK_END:
			if (config.itf.menu_visible && config.itf.menu_index == 3) {
				config.itf.texture_info++;
				if (config.itf.texture_info > 3)
					config.itf.texture_info = 0;
			}
			break;

		case VK_F11:
			if (config.itf.texture_view > -1)
				config.itf.texture_view--;
			break;

		case VK_F12:
			if (config.itf.texture_view < tCount)
				config.itf.texture_view++;
			break;
	}
}

// Configure part

POINT *videoMode = NULL;
s32    vmCount   = 0;

void GSconfigureDefaultFullscreenMode(HWND hWnd)
{
	int index = 0;

	if (!videoMode) {
		SendDlgItemMessage(hWnd,IDC_FULLSCREEN_SCREEN,CB_SETCURSEL,0,0);
		return;
	}

	for (;index<vmCount;index++) {
		if (videoMode[index].x == 640 && videoMode[index].y == 480) {
			SendDlgItemMessage(hWnd,IDC_FULLSCREEN_SCREEN,CB_SETCURSEL,index,0);
			break;
		}
	}
}

void GSconfigureInitFullscreenMode(HWND hWnd)
{
	DEVMODE devMode;
	int i=0,j;
	char c[128];

	devMode.dmSize        = sizeof(devMode);
	devMode.dmDriverExtra = 0;

	while (EnumDisplaySettings(NULL,i++,&devMode)) {
		if (videoMode)
			for (j=0; j<vmCount; j++) {
				if (videoMode[j].x == (signed)devMode.dmPelsWidth && 
					videoMode[j].y == (signed)devMode.dmPelsHeight) {
					devMode.dmPelsWidth = 0;
					break;
				}
			}
		if (devMode.dmPelsWidth < 320 || devMode.dmPelsHeight < 240) continue;

		sprintf(c,"%dx%d",devMode.dmPelsWidth,devMode.dmPelsHeight);
		SendDlgItemMessage(hWnd,IDC_FULLSCREEN_SCREEN,CB_ADDSTRING,0,(LPARAM)c);

		vmCount++;
		videoMode = (POINT*)realloc(videoMode,sizeof(POINT)*vmCount);

		videoMode[vmCount-1].x  = devMode.dmPelsWidth;
		videoMode[vmCount-1].y = devMode.dmPelsHeight;
	}

	for (i=0; i<vmCount; i++) {
		if (videoMode[i-1].x == config.video.width && videoMode[i-1].y == config.video.height) {
			SendDlgItemMessage(hWnd,IDC_FULLSCREEN_SCREEN,CB_SETCURSEL,i-1,0);
			break;
		}
	}
	if (i == vmCount) GSconfigureDefaultFullscreenMode(hWnd);
}

void GSconfigureGetFullscreenMode(HWND hWnd)
{
	int index;

	if (!videoMode) return;

	index = SendDlgItemMessage(hWnd,IDC_FULLSCREEN_SCREEN,CB_GETCURSEL,0,0);

	config.video.width  = videoMode[index].x;
	config.video.height = videoMode[index].y;
}

void GSconfigureDestroyFullscreenMode()
{
	free(videoMode);
	videoMode = NULL;
	vmCount = 0;
}

BOOL CALLBACK GSconfigureLoggingProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			CheckDlgButton(hWnd,IDC_ISOC_FUNCS,config.log.isoc);
			CheckDlgButton(hWnd,IDC_GIF,config.log.gif);
			CheckDlgButton(hWnd,IDC_IDT,config.log.idt);
			CheckDlgButton(hWnd,IDC_PRIM,config.log.prim);
			CheckDlgButton(hWnd,IDC_REG,config.log.registers);
			CheckDlgButton(hWnd,IDC_PREG,config.log.pregisters);
			CheckDlgButton(hWnd,IDC_TEXTURES,config.log.textures);
			CheckDlgButton(hWnd,IDC_TEXTURES_BUFFER,config.log.texturesbuffer);
			return TRUE;
		case WM_CLOSE:
			EndDialog(hWnd,0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					config.log.isoc           = IsDlgButtonChecked(hWnd,IDC_ISOC_FUNCS);
					config.log.gif            = IsDlgButtonChecked(hWnd,IDC_GIF);
					config.log.idt            = IsDlgButtonChecked(hWnd,IDC_IDT);
					config.log.prim           = IsDlgButtonChecked(hWnd,IDC_PRIM);
					config.log.registers      = IsDlgButtonChecked(hWnd,IDC_REG);
					config.log.pregisters     = IsDlgButtonChecked(hWnd,IDC_PREG);
					config.log.textures       = IsDlgButtonChecked(hWnd,IDC_TEXTURES);
					config.log.texturesbuffer = IsDlgButtonChecked(hWnd,IDC_TEXTURES_BUFFER);
					EndDialog(hWnd,1);
					break;
				case IDCANCEL:
					EndDialog(hWnd,0);
					break;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK GSconfigureProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG: {
			char c[8];

			if (config.video.fullscreen) CheckDlgButton(hWnd, IDC_FULLSCREEN, 1);
			else                         CheckDlgButton(hWnd, IDC_WINDOWED, 1);

			GSconfigureInitFullscreenMode(hWnd);

			sprintf(c,"%u",config.video.width);
			SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENX,WM_SETTEXT,0,(LPARAM)c);
			SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENX,EM_LIMITTEXT,4,0);
			sprintf(c,"%u",config.video.height);
			SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENY,WM_SETTEXT,0,(LPARAM)c);
			SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENY,EM_LIMITTEXT,4,0);

			SendDlgItemMessage(hWnd,IDC_BPP,CB_ADDSTRING,0,(LPARAM)"16 bpp");
			SendDlgItemMessage(hWnd,IDC_BPP,CB_ADDSTRING,0,(LPARAM)"32 bpp");
			SendDlgItemMessage(hWnd,IDC_BPP,CB_SETCURSEL,(config.video.bpp==32),0);

			CheckDlgButton(hWnd, IDC_FORCE75HZ, config.video.force75hz);

			SendDlgItemMessage(hWnd,IDC_RATIO,CB_ADDSTRING,0,(LPARAM)"1:1 - faster with some cards");
			SendDlgItemMessage(hWnd,IDC_RATIO,CB_ADDSTRING,0,(LPARAM)"Stretch to full window size");
			SendDlgItemMessage(hWnd,IDC_RATIO,CB_ADDSTRING,0,(LPARAM)"Scale to window size, keep ps2 ratio");
			SendDlgItemMessage(hWnd,IDC_RATIO,CB_SETCURSEL,config.video.ratio,0);

			CheckDlgButton(hWnd, IDC_FBFIX0, config.fixes.fbfix == 0);
			CheckDlgButton(hWnd, IDC_FBFIX1, config.fixes.fbfix == 1);
			CheckDlgButton(hWnd, IDC_FBFIX2, config.fixes.fbfix == 2);
			CheckDlgButton(hWnd, IDC_MISSING_STUFF, config.fixes.missing_stuff);
			CheckDlgButton(hWnd, IDC_DISABLE_BLEND, config.fixes.disable_blend);
			CheckDlgButton(hWnd, IDC_DISABLE_IDT_ONSCREEN, config.fixes.disable_idt_onscreen);

			CheckDlgButton(hWnd, IDC_FPSLIMIT_ON, config.speed.limit_on);

			sprintf(c, "%u", config.speed.limit_to);
			SendDlgItemMessage(hWnd,IDC_FPSLIMIT_TO,WM_SETTEXT,0,(LPARAM)c);
			SendDlgItemMessage(hWnd,IDC_FPSLIMIT_TO,EM_LIMITTEXT,4,0);

			CheckDlgButton(hWnd,IDC_LOGON,config.log.logon);

			CheckDlgButton(hWnd, IDC_SHOWSTATUS, config.itf.menu_visible);

			CheckDlgButton(hWnd, IDC_ANTIALIASING, config.video.antialiasing);
			CheckDlgButton(hWnd, IDC_FILTERING, config.video.filtering);

			return TRUE; }
		case WM_CLOSE:
			EndDialog(hWnd,0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					char c[8];

					config.video.fullscreen = IsDlgButtonChecked(hWnd,IDC_FULLSCREEN);
					if (config.video.fullscreen) {
						GSconfigureGetFullscreenMode(hWnd);
					} else {
						SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENX,WM_GETTEXT,5,(LPARAM)c);
						config.video.width = atoi(c);
						SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENY,WM_GETTEXT,5,(LPARAM)c);
						config.video.height = atoi(c);
					}
					GSconfigureDestroyFullscreenMode();
					config.video.bpp        = (SendDlgItemMessage(hWnd,IDC_BPP,CB_GETCURSEL,0,0) == 1) ? 32 : 16;
					config.video.force75hz  = IsDlgButtonChecked(hWnd,IDC_FORCE75HZ);
					config.video.ratio      = SendDlgItemMessage(hWnd,IDC_RATIO,CB_GETCURSEL,0,0);
					config.log.logon        = IsDlgButtonChecked(hWnd,IDC_LOGON);
					     if (IsDlgButtonChecked(hWnd,IDC_FBFIX0)) config.fixes.fbfix = 0;
					else if (IsDlgButtonChecked(hWnd,IDC_FBFIX1)) config.fixes.fbfix = 1;
					else                                          config.fixes.fbfix = 2;
					config.fixes.missing_stuff = IsDlgButtonChecked(hWnd,IDC_MISSING_STUFF);
					config.fixes.disable_blend = IsDlgButtonChecked(hWnd,IDC_DISABLE_BLEND);
					config.fixes.disable_idt_onscreen = IsDlgButtonChecked(hWnd,IDC_DISABLE_IDT_ONSCREEN);
					config.speed.limit_on   = IsDlgButtonChecked(hWnd,IDC_FPSLIMIT_ON);
					SendDlgItemMessage(hWnd,IDC_FPSLIMIT_TO,WM_GETTEXT,5,(LPARAM)c);
					config.speed.limit_to = atoi(c);
					config.itf.menu_visible = IsDlgButtonChecked(hWnd,IDC_SHOWSTATUS);
					config.video.antialiasing = IsDlgButtonChecked(hWnd,IDC_ANTIALIASING);
					config.video.filtering = IsDlgButtonChecked(hWnd,IDC_FILTERING);

					EndDialog(hWnd,1);
					break; }
				case IDCANCEL:
					EndDialog(hWnd,0);
					break;
				case IDC_DEFAULT:
					CheckDlgButton(hWnd, IDC_FULLSCREEN, 0);
					CheckDlgButton(hWnd, IDC_WINDOWED, 1);
					GSconfigureDefaultFullscreenMode(hWnd);
					SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENX,WM_SETTEXT,0,(LPARAM)"640");
					SendDlgItemMessage(hWnd,IDC_WINDOWED_SCREENY,WM_SETTEXT,0,(LPARAM)"480");
					SendDlgItemMessage(hWnd,IDC_BPP,CB_SETCURSEL,1,0);
					CheckDlgButton(hWnd, IDC_FORCE75HZ, 0);
					SendDlgItemMessage(hWnd,IDC_RATIO,CB_SETCURSEL,1,0);
					CheckDlgButton(hWnd,IDC_LOGON,0);
					CheckDlgButton(hWnd,IDC_FBFIX0,0);
					CheckDlgButton(hWnd,IDC_FBFIX1,0);
					CheckDlgButton(hWnd,IDC_FBFIX2,1);
					CheckDlgButton(hWnd,IDC_MISSING_STUFF,0);
					CheckDlgButton(hWnd,IDC_DISABLE_BLEND,0);
					CheckDlgButton(hWnd,IDC_DISABLE_IDT_ONSCREEN,0);
					CheckDlgButton(hWnd,IDC_SHOWSTATUS,0);
					CheckDlgButton(hWnd,IDC_FPSLIMIT_ON,1);
					SendDlgItemMessage(hWnd,IDC_FPSLIMIT_TO,WM_SETTEXT,0,(LPARAM)"60");
					CheckDlgButton(hWnd,IDC_ANTIALIASING,0);
					CheckDlgButton(hWnd,IDC_FILTERING,0);
					break;
				case IDC_LOGGING:
					DialogBox(hInstDLL,MAKEINTRESOURCE(IDD_GSTARIS_LOGGING),hWnd,GSconfigureLoggingProc);
					break;
			}
			break;
	}
	return FALSE;
}

void CALLBACK GSconfigure()
{
	config_load();
	if (DialogBox(hInstDLL,MAKEINTRESOURCE(IDD_GSTARIS_CONFIG),GetActiveWindow(),GSconfigureProc)) {
		config_save();
		config_changed = 1;
		if (already_opened)
			MessageBox(GetActiveWindow(),"You should restart the emulator in order to prevent bugs because of the changes.","GStaris Warning", MB_OK);
	}

	GSconfigureDestroyFullscreenMode();
}

BOOL CALLBACK GSaboutProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			return TRUE;
		case WM_CLOSE:
			EndDialog(hWnd,0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd,0);
					break;
			}
	}
	return FALSE;
}

void CALLBACK GSabout()
{
	DialogBox(hInstDLL,MAKEINTRESOURCE(IDD_GSTARIS_ABOUT),GetActiveWindow(),GSaboutProc);
}
