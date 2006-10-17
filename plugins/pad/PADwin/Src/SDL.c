/*  PADwin
 *  Copyright (C) 2002-2004  PADwin Team
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
#include <SDL/SDL.h>
#include "PAD.h"

SDL_Surface *surf;


static void _PADupdate() {
/*	int key;

    while (XPending(GSdsp)) {
		XNextEvent(GSdsp, &E);
		switch (E.type) {
	    	case KeyPress:
				key = XLookupKeysym((XKeyEvent *)&E, 0);
				_KeyPress(key);
				break;

		    case KeyRelease:
				key = XLookupKeysym((XKeyEvent *)&E, 0);
				_KeyRelease(key);
				break;

	    	case FocusIn:
				XAutoRepeatOff(GSdsp);
				break;

	    	case FocusOut:
				XAutoRepeatOn(GSdsp);
				break;
		}
    }*/
}

static s32  _PADopen(void *pDsp) {
	surf = (SDL_Surface*)pDsp;

	return 0;
}

static void _PADclose() {
}


PADdriver PADsdl = {
	"sdl",
	_PADopen,
	_PADclose,
	_PADupdate,
};
