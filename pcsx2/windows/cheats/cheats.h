#ifndef CHEATS_H_INCLUDED
#define CHEATS_H_INCLUDED

typedef enum ebool
{
	false,
	true
} bool;

typedef union eany
{
	s8  vs8;
	u8  vu8;
	s16 vs16;
	u16 vu16;
	s32 vs32;
	u32 vu32;
	s64 vs64;
	u64 vu64;
} any;

typedef struct eresult
{
	u32 address;
	any oldval;
} result;

extern HINSTANCE pInstance;
extern bool FirstShow;

void AddCheat(HINSTANCE hInstance, HWND hParent);
void ShowFinder(HINSTANCE hInstance, HWND hParent);
void ShowCheats(HINSTANCE hInstance, HWND hParent);

void AddPatch(int Source, int Address, int Size, int Unsigned, any *data);

#endif//CHEATS_H_INCLUDED
