#include <windows.h>
#include <iostream>
#include <sstream>
using namespace std;

#include "tinyxml.h"

extern "C" {
#include "../ps2etypes.h"
}

static int gPass = 0;
static int gFail = 0;

//
// This file demonstrates some basic functionality of TinyXml.
// Note that the example is very contrived. It presumes you know
// what is in the XML file. But it does test the basic operations,
// and show how to add and remove nodes.
//

extern "C" typedef struct
{
   int    type;
   int    cpu;
   int    placetopatch;
   u32    addr;
   u32    data;
} IniPatch;

extern "C" typedef struct {
	HWND hWnd;           // Main window handle
	HINSTANCE hInstance; // Application instance
	HMENU hMenu;         // Main window menu
	HANDLE hConsole;
} AppData;

extern "C" extern IniPatch patch[ 1024 ];
extern "C" extern int patchnumber;

#ifdef __WIN32__
extern "C" extern AppData gApp;
#endif

extern "C" void SysPrintf(char *fmt, ...);

extern "C" int LoadPatch(char *patchfile);

int LoadGroup(TiXmlNode *group);

int LoadPatch(char *crc)
{
	char pfile[256];
	sprintf(pfile,"patches\\%s.xml",crc);

	patchnumber=0;

	TiXmlDocument doc( pfile );
	bool loadOkay = doc.LoadFile();
	if ( !loadOkay )
	{
		SysPrintf("XML Patch Loader: Could not load file '%s'. Error='%s'.\n", pfile, doc.ErrorDesc() );
		return -1;
	}

	TiXmlNode *root = doc.FirstChild("GAME");
	if(!root)
	{
		SysPrintf("XML Patch Loader: Root node is not GAME, invalid patch file.\n");
		return -1;
	}

	TiXmlElement *rootelement = root->ToElement();

	const char *title=rootelement->Attribute("title");
	if(title)
		SysPrintf("XML Patch Loader: Game Title: %s\n",title);

	int result=LoadGroup(root);
	if(result) {
		patchnumber=0;
		return result;
	}

#ifdef __WIN32__
	if (gApp.hConsole) 
	{
		if(title)
			SetConsoleTitle(title);
		else
			SetConsoleTitle("<No Title>");
	}
		
#endif

	return 0;
}


int LoadGroup(TiXmlNode *group)
{

	TiXmlElement *groupelement = group->ToElement();

	const char *gtitle=groupelement->Attribute("title");
	if(gtitle)
		SysPrintf("XML Patch Loader: Group Title: %s\n",gtitle);

	const char *enable=groupelement->Attribute("enabled");
	if(enable)
	{
		if(strcmp(enable,"false")==0)
		{
			SysPrintf("XML Patch Loader: Group is disabled.\n");
			return 0;
		}
	}

	TiXmlNode *comment = group->FirstChild("COMMENT");
	if(comment)
	{
		TiXmlElement *cmelement = comment->ToElement();
		const char *comment = cmelement->GetText();
		if(comment)
			SysPrintf("XML Patch Loader: Group Comment:\n%s\n---\n",comment);
	}

	TiXmlNode *cpatch = group->FirstChild("PATCH");
	while(cpatch)
	{
		TiXmlElement *celement = cpatch->ToElement();
		if(!celement)
		{
			SysPrintf("XML Patch Loader: ERROR: Couldn't convert node to element.\n" );
			return -1;
		}


		const char *ptitle=celement->Attribute("title");
		const char *penable=celement->Attribute("enabled");
		const char *applymode=celement->Attribute("applymode");
		const char *place=celement->Attribute("place");
		const char *address=celement->Attribute("address");
		const char *size=celement->Attribute("size");
		const char *value=celement->Attribute("value");

		if(ptitle) {
			SysPrintf("XML Patch Loader: Patch title: %s\n", ptitle);
		}

		bool penabled=true;
		if(penable)
		{
			if(strcmp(penable,"false")==0)
			{
				SysPrintf("XML Patch Loader: Patch is disabled.\n");
				penabled=false;
			}
		}

		if(penabled)
		{
			if(!applymode) applymode="frame";
			if(!place) place="EE";
			if(!address) {
				SysPrintf("XML Patch Loader: ERROR: Patch doesn't contain an address.\n");
				return -1;
			}
			if(!value) {
				SysPrintf("XML Patch Loader: ERROR: Patch doesn't contain a value.\n");
				return -1;
			}
			if(!size) {
				SysPrintf("XML Patch Loader: WARNING: Patch doesn't contain the size. Trying to deduce from the value size.\n");
				switch(strlen(value))
				{
					case 8:
					case 7:
					case 6:
					case 5:
						size="32";
						break;
					case 4:
					case 3:
						size="16";
						break;
					case 2:
					case 1:
						size="8";
						break;
					case 0:
						size="0";
						break;
					default:
						size="64";
						break;
				}
			}

			if(strcmp(applymode,"startup")==0)
			{
				patch[patchnumber].placetopatch=0;
			} else
			if(strcmp(applymode,"vsync")==0)
			{
				patch[patchnumber].placetopatch=1;
			} else
			{
				SysPrintf("XML Patch Loader: ERROR: Invalid applymode attribute.\n");
				patchnumber=0;
				return -1;
			}
			
			if(strcmp(place,"EE")==0)
			{
				patch[patchnumber].cpu=1;
			} else
			if(strcmp(place,"IOP")==0)
			{
				patch[patchnumber].cpu=2;
			} else
			{
				SysPrintf("XML Patch Loader: ERROR: Invalid place attribute.\n");
				patchnumber=0;
				return -1;
			}

			if(strcmp(size,"64")==0)
			{
				patch[patchnumber].type=4;
			} else
			if(strcmp(size,"32")==0)
			{
				patch[patchnumber].type=3;
			} else
			if(strcmp(size,"16")==0)
			{
				patch[patchnumber].type=2;
			} else
			if(strcmp(size,"8")==0)
			{
				patch[patchnumber].type=1;
			} else
			{
				SysPrintf("XML Patch Loader: ERROR: Invalid size attribute.\n");
				patchnumber=0;
				return -1;
			}

			sscanf( address, "%X", &patch[ patchnumber ].addr );
			sscanf( value, "%I64X", &patch[ patchnumber ].data );

			patchnumber++;

		}

		cpatch = cpatch->NextSibling("PATCH");
	}

	cpatch = group->FirstChild("GROUP");
	while(cpatch) {
		int result=LoadGroup(cpatch);
		if(result) return result;
		cpatch = cpatch->NextSibling("GROUP");
	}

	return 0;
}
