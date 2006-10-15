#ifndef __PATCH_H__
#define __PATCH_H__

//
// Defines
//
#define MAX_PATCH 1024 

#define IFIS(x,str) if(!strnicmp(x,str,sizeof(str)-1))

#define GETNEXT_PARAM() \
	while ( *param && ( *param != ',' ) ) param++; \
   if ( *param ) param++; \
	while ( *param && ( *param == ' ' ) ) param++; \
	if ( *param == 0 ) { SysPrintf( _( "Not enough params for inicommand\n" ) ); return; }

//
// Typedefs
//
typedef void (*PATCHTABLEFUNC)( char * text1, char * text2 );

typedef struct
{
   char           * text;
   int            code;
   PATCHTABLEFUNC func;
} PatchTextTable;

typedef struct
{
   int enabled;
   int type;
   int cpu;
   int placetopatch;
   u32 addr;
   u64 data;
} IniPatch;

//
// Function prototypes
//
void patchFunc_comment( char * text1, char * text2 );
void patchFunc_gametitle( char * text1, char * text2 );
void patchFunc_patch( char * text1, char * text2 );

void inifile_trim( char * buffer );

//
// Variables
//
extern PatchTextTable commands[];

extern PatchTextTable dataType[];

extern PatchTextTable cpuCore[];

extern IniPatch patch[ MAX_PATCH ];
extern int patchnumber;


void applypatch( int place );
void inifile_read( char * name );
void inifile_command( char * cmd );
void resetpatch( void );

#endif /* __PATCH_H__ */

