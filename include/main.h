#ifndef __MAIN_H__
# define __MAIN_H__
# ifdef NODEBUG
#  undef TRACE
# endif
# ifdef _main_
#	define CONST
# else
#	define CONST const
# endif

#include <stddef.h>
#include <X11/Xmd.h>


#define RID_MASK_BITS    18
#define RID_MASK         ((1uL << (RID_MASK_BITS)) -1)
#define RID_Base(id)     ((id) >>  RID_MASK_BITS)
#define RID_Match(b,r)   ((b) == RID_Base(r))

#define ROOT_WINDOW     0x8000uL // alien AES handles are ORed to this
#define DFLT_VISUAL     0x0100uL // further visuals IDs start from here
#define DFLT_COLORMAP   0x0200uL

#define PADD_BITS   16


struct s_PXY;        typedef struct s_PXY      * p_PXY;
struct s_GRECT;      typedef struct s_GRECT    * p_GRECT;

struct s_GC;         typedef struct s_GC       * p_GC;
struct s_FONt;       typedef struct s_FONT     * p_FONT;
struct s_FONTABLE;   typedef union  u_FONTABLE {
	struct s_FONTABLE * p;
	p_GC                Gc;
	p_FONT              Font;
} p_FONTABLE;
struct s_PIXMAP;     typedef struct s_PIXMAP   * p_PIXMAP;
struct s_WINDOW;     typedef struct s_WINDOW   * p_WINDOW;
struct s_DRAWABLE;   typedef union  u_DRAWABLE {
	struct s_DRAWABLE * p;
	p_PIXMAP            Pixmap;
	p_WINDOW            Window;
} p_DRAWABLE;
struct s_CLIENT;     typedef struct s_CLIENT   * p_CLIENT;
struct s_PROPERTY;   typedef struct s_PROPERTY * p_PROPERTY;
struct s_CURSOR;     typedef struct s_CURSOR   * p_CURSOR;


extern CONST CARD32 MAIN_TimeStamp;
extern CONST CARD16 MAIN_KeyButMask;
extern CONST p_PXY  MAIN_PointerPos;

extern long MAIN_FDSET_wr, MAIN_FDSET_rd;

void    MainSetMove  (BOOL onNoff);
void    MainSetWatch (const p_GRECT area, BOOL leaveNenter);
#define MainClrWatch(void) MainSetWatch (NULL, 0)


BOOL WindButton       (CARD16 prev_mask, int count);
void WindPointerWatch (BOOL movedNreorg);
void WindPointerMove  (const p_PXY mouse);


static inline void WindUpdate (onNoff) {
	extern int   wind_update (int);
	extern short _MAIN_Wupdt;
	if      (onNoff)      { wind_update (1); _MAIN_Wupdt++; }
	else if (_MAIN_Wupdt) { wind_update (0); _MAIN_Wupdt--; }
}


# undef CONST
#endif __MAIN_H__