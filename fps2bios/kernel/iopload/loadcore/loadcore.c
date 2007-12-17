
#include <tamtypes.h>

#include "ksysmem.h"
#include "kloadcore.h"
#include "iopdebug.h"
#include "iopelf.h"
#include "romdir.h"
#include "irx.h"		// for IMPORT_MAGIC

u32 free;
u32 sysmem_00;
u32 modules_count;
u32	module_index;
u32 *place;	// addr of 8 * 31 = 8 * linesInIopbtconf	//place[2][31]
u32	bootmodes[16];
u32 bootmodes_size;
int debug=1;

struct _let {
	struct export *first;
	struct export *last;
	struct export *mda;
} let;


u32 bm_end;

void _start(struct init *init);

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("loadcore: " fmt, ## args)

///////////////////////////////////////////////////////////////////////
void retonly(){}

///////////////////////////////////////////////////////////////////////
void RegisterBootMode(struct bootmode *b) {
	int i;

	_dprintf("%s\n", __FUNCTION__);
	if (((b->len + 1) * 4) < (16-bootmodes_size)) {
		u32 *p = &bootmodes[bootmodes_size];
		for (i=0; i<b->len + 1; i++) p[i]=((u32*)b)[i];
		p[i]=0;
		bootmodes_size+= b->len + 1;
	}
}

///////////////////////////////////////////////////////////////////////
int *QueryBootMode(int id) {
	u32 *b;

	for (b = (u32*)&bootmodes[0]; *b; b += ((struct bootmode*)b)->len + 1)
		if (id == ((struct bootmode*)b)->id)
			return b;
	return NULL;
}

///////////////////////////////////////////////////////////////////////
int  match_name(struct export *src, struct export *dst){
	return (*(int*)(src->name+0) == *(int*)(dst->name+0)) &&
	       (*(int*)(src->name+4) == *(int*)(dst->name+4));
}

///////////////////////////////////////////////////////////////////////
int  match_version_major(struct export *src, struct export *dst){
	return ((src->version>>8) - (dst->version>>8));
}

///////////////////////////////////////////////////////////////////////
int  match_version_minor(struct export *src, struct export *dst){
	return ((unsigned char)src->version - (unsigned char)dst->version);
}

///////////////////////////////////////////////////////////////////////
int  fix_imports(struct import *imp, struct export *exp){
    func	*ef;
    struct func_stub *fs;
    int  count=0, ordinal;

	for (ef=exp->func; *ef; ef++){
		count++;		//count number of exported functions
	}
	_dprintf("%s (%d functions)\n", __FUNCTION__, count);
	
	for (fs=imp->func; fs->jr_ra; fs++) {
		if ((fs->addiu0 >> 26) != INS_ADDIU)	break;

		ordinal = fs->addiu0 & 0xFFFF;
		if (ordinal < count) {
//			_dprintf("%s linking ordinal %d to %x\n", __FUNCTION__, ordinal, exp->func[ordinal]);
			fs->jr_ra=(((u32)exp->func[ordinal]>>2) & 0x3FFFFFF) | INS_J;
		} else {
			fs->jr_ra=INS_JR_RA;
		}
	}

	imp->flags |=FLAG_IMPORT_QUEUED;
	return 0;
}

///////////////////////////////////////////////////////////////////////
// Check the structure of the import table.
// Return 0 if a bad or empty import table is detected.
// Return a non-zero value if a valid import table is detected.
//
int check_import_table(struct import* imp){
    struct func_stub *f;

	if (imp->magic != IMPORT_MAGIC)
		return 0;
	for (f=imp->func; f->jr_ra; f++){
		if (f->addiu0 >> 26 != INS_ADDIU)
			return 0;
		if ((f->jr_ra!=INS_JR_RA) && (f->jr_ra>26!=INS_JR))
			return 0;
	}
	if (f->addiu0)
		return 0;
	return (imp->func < f);
}

///////////////////////////////////////////////////////////////////////[OK]

// Return 0 if successful
int  link_client(struct import *imp){
	struct export *e;

//	_dprintf("%s\n", __FUNCTION__);
	for (e=let.first; e; e=(struct export*)e->magic_link) {
/*		if (debug > 0){
			// Zero terminate the name before printing it
			char ename[9], iname[9];
			*(int*)ename = *(int*)e->name;   *(int*)(ename+4) = *(int*)(e->name+4);  ename[8] = 0;
			*(int*)iname = *(int*)imp->name; *(int*)(iname+4) = *(int*)(imp->name+4);iname[8] = 0;
			
			__printf("loadcore: %s: %s, %s\n", __FUNCTION__, ename, iname);
		}
*/
		if (!(e->flags & FLAG_NO_AUTO_LINK)){	
            if ( match_name(e, imp)){
		        if (match_version_major(e, imp)==0) {
					fix_imports(imp, e);
					imp->next=e->next;
					e->next=imp;
					FlushIcache();
					return 0;
				} else {
//					_dprintf("%s: version does not match\n", __FUNCTION__);					
				}
			} else {
//				_dprintf("%s: name does not match\n", __FUNCTION__);	
			}
		} else {
//			_dprintf("%s: e->flags bit 0 is 0\n", __FUNCTION__);	
		}
	}
	_dprintf("%s: FAILED to find a match\n", __FUNCTION__);
	return -1;
}

///////////////////////////////////////////////////////////////////////[OK]
int  findFixImports(u32 *addr, int size) {
	struct import *p;
	int i;

	_dprintf("%s: %x, %d\n", __FUNCTION__, addr, size);
	for (i=0; i<size/4; i++, addr++) {
		p = (struct import *)addr;
		if ((p->magic == IMPORT_MAGIC) &&
			check_import_table(p) &&
		    ((p->flags & 7) == 0) &&
		    link_client(p)) {
			restoreImports(p, size);
			return -1;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////[OK]
int  unlink_client(struct import *i1, struct import *i2){
	struct import *i, *tmp;

	if (i1->next == i2){
		i1->next=i2->next;
		return 0;
	}
	for (i = i1->next; i->next;) {
		tmp=i->next;
		if (tmp==i2) {
			i->next=tmp->next;
			tmp->next=NULL;
			return 0;
		}
		i=tmp;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////[OK]
void restore_imports(struct import* imp){
    struct func_stub *f;

	for (f=imp->func; (f->jr_ra) && ((f->addiu0 >> 26) == INS_ADDIU); f++)
		f->jr_ra=INS_JR_RA;
}

///////////////////////////////////////////////////////////////////////[OK]
int  restoreImports(void *addr, int size) {
    struct export *e;
    struct export *i;
    struct export *tmp;
    void 		*limit = addr + (size & ~0x3);
	
	for (e = (struct export*)let.first; e; e=(struct export*)e->magic_link){
		for (i = e->next; i; i=i->next) {
			if ((i >= addr) && (i < limit)) {
				if (unlink_client(e, i))
					return -1;
				i->flags &= ~0x7;
				restore_imports(i);                      
			}
		}
		if ((e >= addr) && (e < limit))
			ReleaseLibraryEntries(e);
	}
/*
	for (i=let.mda; i->next; )
		if ((i->next >= addr) && (i->next < limit)){
			i->next->flags &= ~0x7;
			restore_imports(i->next);
			tmp = i->next->next;
			i->next->next=NULL;
			i->next=tmp;
		}else
			i=i->next;
*/
	return 0;
}

///////////////////////////////////////////////////////////////////////
int  SetNonAutoLinkFlag(struct export *e){
	return (e->flags |= FLAG_NO_AUTO_LINK);
}

///////////////////////////////////////////////////////////////////////
int  UnsetNonAutoLinkFlag(struct export *e){
	return (e->flags &= ~FLAG_NO_AUTO_LINK);
}

///////////////////////////////////////////////////////////////////////
int  registerFunc(int (*function)(int *, int), int a1, int *result){
/*	int x;
	int r;

	if (place==NULL){
		x=1;
		r=function(&x, 0);
		if (result)	*result=r;
		return 0;
	}
	place[0]=function + a1 & 3;
	place[1]=$gp;
	place[2]=0;
	place+=2;
	return 1;*/
}

///////////////////////////////////////////////////////////////////////
void linkModule(struct imageInfo *ii){
	struct imageInfo *p;

	for (p=&sysmem_00; p->next && (p->next < ii); p=p->next);

	ii->next=p->next;	
	p->next=ii;

	ii->index=module_index++;
	modules_count++;
}

///////////////////////////////////////////////////////////////////////
void unlinkModule(struct imageInfo *ii){
    struct imageInfo *p;

	if (ii == NULL) return;

	for (p=&sysmem_00; p->next; p=p->next) {
		if (p->next == ii){
			p->next=p->next->next;
			modules_count--;
			return;
		}
	}
}

///////////////////////////////////////////////////////////////////////
int RegisterLibraryEntries(struct export *es){
    struct export *p;
	struct export *plast;
	struct export *pnext;
	struct export *tmp;
	struct export *snext;

	if ((es == NULL) || (es->magic_link != EXPORT_MAGIC))
		return -1;

	if (debug > 0){
		// Zero terminate the name before printing it
		char ename[9];
		*(int*)ename = *(int*)es->name;
		*(int*)(ename+4) = *(int*)(es->name+4);
		ename[8] = 0;
		
		__printf("loadcore: %s: %s, %x\n", __FUNCTION__, ename, es->version);
	}
		
		
	plast=NULL;
	for (p=(struct export*)let.first; p; p=(struct export*)p->magic_link) {
		if (match_name(es, p) == 0 ||
			match_version_major(es, p) == 0) continue;

		if (match_version_minor(es, p) == 0)
			return -1;
		_dprintf("%s: found match\n", __FUNCTION__);
		pnext = p->next;
		p->next = NULL;
			
		for (tmp = pnext; tmp; ) {
			if (tmp->flags & FLAG_NO_AUTO_LINK) {
				pnext->magic_link = (u32)tmp;
				pnext = tmp->next;
				tmp->next = NULL;
			} else {
				tmp->next=plast;
				plast=tmp;
			}
			tmp=tmp->next;
		}
	}
/*
	for (tmp = let.mda; tmp->next; tmp=tmp->next) { //free
		if ((match_name(es, tmp->next)) &&
		    (match_version_major(es, tmp->next)==0)){
			_dprintf("%s: freeing module\n", __FUNCTION__);
			snext=tmp->next->next;
			tmp->next->next=plast;
			plast=tmp->next;
			tmp->next=snext;
		} else tmp=tmp->next;
	}
*/
	es->next=0;
	while (plast) {
		snext=plast->next;
		fix_imports(plast, es);
		plast->next=es->next;
		es->next=plast;
		plast=snext;
	}
	es->flags &= ~FLAG_NO_AUTO_LINK;
	es->magic_link=let.first;
	let.first=es;
	FlushIcache();

	return 0;
}

///////////////////////////////////////////////////////////////////////
int  ReleaseLibraryEntries(struct export *e) {
	struct export *n, *p, *next, *prev;

	p = let.first;
	while ((p) && (p!=e)){
		prev=p;
		p=(struct export*)prev->magic_link;
	}
	if (p != e)
		return -1;

	n = e->next;
	e->next = 0;

	prev->magic_link = e->magic_link;
	e->magic_link    = EXPORT_MAGIC;

	while (n) {
		next = n->next;
		if (link_client(n)){
			restore_imports(n);
			n->flags=(n->flags & ~2) | 4;
			n->next = let.mda->next;
			free=n;
		}
		n=next;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
int  RegisterNonAutoLinkEntries(struct export *e) {
	if ((e == NULL) || (e->magic_link != EXPORT_MAGIC)){
		return -1;
	}
	e->flags |= FLAG_NO_AUTO_LINK;
	e->magic_link = let.first;	// --add as first
	let.first = e;				// /
	FlushIcache();

	return 0;
}

///////////////////////////////////////////////////////////////////////
int  QueryLibraryEntryTable(struct export *e){
	struct export *p = let.first;
	while (p){
		if ((match_name(p, e)) && (match_version_major(p, e)==0)){
			return p->func;
		}
		p=p->magic_link;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
struct export* GetLibraryEntryTable(){
	return (struct export*)&let;
}

///////////////////////////////////////////////////////////////////////
void _FlushIcache() {
	u32 status;
	u32 s1450;
	u32 s1578;
	u32 icache;
	u32 *p;

	__asm__ ("mfc0 %0, $12\n" : "=r"(status) : );

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	s1450 = *(int*)0xBF801450;
	*(int*)0xBF801450&= ~1;
	*(int*)0xBF801450;

	s1578 = *(int*)0xBF801578;
	*(int*)0xBF801578 = 0;
	*(int*)0xBF801578;

	icache = *(int*)0xFFFE0130;
	*(int*)0xFFFE0130 = 0xC04;
	*(int*)0xFFFE0130;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0x10000));

	for (p=0; p<(u32*)0x400; p+=4)	// 4KB instruction cache
		*p=0;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	*(int*)0xFFFE0130 = icache;
	*(int*)0xFFFE0130;
	*(int*)0xBF801578 = s1578;
	*(int*)0xBF801578;
	*(int*)0xBF801450 = s1450;
	*(int*)0xBF801450;

	__asm__ ("mtc0 %0, $12\n" : : "r"(status) );
}

void FlushIcache() {
	__asm__ (
		"la   $26, %0\n"
		"lui  $27, 0xA000\n"
		"or   $26, $27\n"
		"jr   $26\n"
		"nop\n"
		: : "i"(_FlushIcache)
	);
}

///////////////////////////////////////////////////////////////////////
void _FlushDcache() {
	u32 status;
	u32 s1450;
	u32 s1578;
	u32 icache;
	u32 *p;

	__asm__ ("mfc0 %0, $12\n" : "=r"(status) : );

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	s1450 = *(int*)0xBF801450;
	*(int*)0xBF801450&= ~1;
	*(int*)0xBF801450;

	s1578 = *(int*)0xBF801578;
	*(int*)0xBF801578 = 0;
	*(int*)0xBF801578;

	icache = *(int*)0xFFFE0130;
	*(int*)0xFFFE0130 = 0xC4;
	*(int*)0xFFFE0130;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0x10000));

	for (p=0; p<(u32*)0x100; p+=4)	// 1KB data cache
		*p=0;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	*(int*)0xFFFE0130 = icache;
	*(int*)0xFFFE0130;
	*(int*)0xBF801578 = s1578;
	*(int*)0xBF801578;
	*(int*)0xBF801450 = s1450;
	*(int*)0xBF801450;

	__asm__ ("mtc0 %0, $12\n" : : "r"(status) );
}

void FlushDcache(){
	__asm__ (
		"la   $26, %0\n"
		"lui  $27, 0xA000\n"
		"or   $26, $27\n"
		"jr   $26\n"
		"nop\n"
		: : "i"(_FlushDcache)
	);
}

///////////////////////////////////////////////////////////////////////
void _SetIcache(u32 val) {
	u32 status;

	__asm__ ("mfc0 %0, $12\n" : "=r"(status) : );

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	*(int*)0xFFFE0130 = val;
	*(int*)0xFFFE0130;

	__asm__ ("mtc0 %0, $12\n" : : "r"(status) );
}

void SetIcache(u32 val){
	__asm__ (
		"la   $26, %0\n"
		"lui  $27, 0xA000\n"
		"or   $26, $27\n"
		"jr   $26\n"
		"nop\n"
		: : "i"(_SetIcache)
	);
}


//////////////////////////////entrypoint///////////////////////////////
struct export loadcore_stub={
	EXPORT_MAGIC,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"loadcore",
	(func)_start,	// entrypoint
	(func)retonly,
	(func)retonly,
	(func)GetLibraryEntryTable,
	(func)FlushIcache,
	(func)FlushDcache,
	(func)RegisterLibraryEntries,
	(func)ReleaseLibraryEntries,
	(func)findFixImports,
	(func)restoreImports,
	(func)RegisterNonAutoLinkEntries,
	(func)QueryLibraryEntryTable,
	(func)QueryBootMode,
	(func)RegisterBootMode,
	(func)SetNonAutoLinkFlag,
	(func)UnsetNonAutoLinkFlag,
	(func)linkModule,
	(func)unlinkModule,
	(func)retonly,
	(func)retonly,
	(func)registerFunc,
	(func)SetIcache,
/*	(func)ReadModuleHeader,
	(func)LoadModule,
	(func)findImageInfo,*/
	0
};

void loadcore_start(struct init *init) {
    struct export *sysmem;
	struct rominfo ri;
	void (*entry)();
	u32 offset;
	u32 status = 0x401;
    int bm;
	int i;

	_dprintf("%s\n", __FUNCTION__);

	// Write 0x401 into the co-processor status register
	// This enables interrupts generally, and disables (masks) them all except hardware interrupt 0
	__asm__ (
		"mtc0 %0, $12\n"
		: : "r"(status)
	);

	for (i=0; i<17; i++){
		bootmodes[i]=0;
	}
	bootmodes_size=0;

	let.first = init->sysmem;
	let.last  = init->sysmem;
	sysmem = init->sysmem;
	sysmem->magic_link = 0;
	modules_count	= 2;	//sysmem + loadcore
	module_index	= 3;	//next module will be the 3rd

	bm = 0x00040000;
	RegisterBootMode(&bm);

	RegisterLibraryEntries(&loadcore_stub);

	_dprintf("loading modules\n");

	offset = init->offset;
	for (i=0; init->moduleslist[i] != NULL; i++) {
		_dprintf("loading module %s: at offset %x\n", init->moduleslist[i], offset);
		if (romdirGetFile(init->moduleslist[i], &ri) == NULL) {
			_dprintf("error loading module %s\n", init->moduleslist[i]);
		}
		entry = (void (*)())loadElfFile(init->moduleslist[i], offset);
		if (findFixImports(offset, ri.fileSize) == -1) {
			_dprintf("failed to fix imports to module %s\n", init->moduleslist[i]);
		} else {
			_dprintf("executing %s entry at %p\n", init->moduleslist[i], entry);
			entry();
		}
		offset+= (ri.fileSize + 15) & ~0xf;
	}

	__printf("%x; %x; %x\n", *(u32*)0x1f801070, *(u32*)0x1f801074, *(u32*)0x1f801078);

	_dprintf("modules loaded ok\n");
}

//////////////////////////////entrypoint///////////////////////////////
void _start(struct init *init) {
	*(int*)0xFFFE0130 = 0x1e988;
	__asm__ (
		"addiu $26, $0, 0\n"
		"mtc0  $26, $12\n"
		"move  $fp, %0\n"
		"move  $sp, %0\n"
		: : "r"((init->memsize << 20) - 0x40)
	);
	__asm__ (
		"j     loadcore_start\n"
		"nop\n"
	);
}

