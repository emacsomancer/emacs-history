/* terminal control module for DEC VT100's */ 
 
/*		Copyright (c) 1981,1980 James Gosling 
   Enhancements copyright (c) 1984 Fen Labalme and jpershing@bbn. 
   Distributed by Fen Labalme, with permission from James Gosling. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but there is no warranty of any sort, and no contributor accepts 
responsibility for the consequences of using this program 
or for whether it serves any particular purpose. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on any copy 
distributed.  */ 
 
 
 
/* This is a somewhat primitive driver for the DEC VT100 terminal.  The 
   terminal is driven in so-called "ansi" mode, using jump scroll.  It is 
   assumed to have the Control-S misfeature disabled (although this 
   shouldn't get in the way -- it does anyway).  Specific optimization left 
   to be done are (1) deferral of setting the window until necessary (as 
   the escape sequence to do this is expensive) and (2) being more clever 
   about optimizing motion (as the direct-cursor-motion sequence is also 
   quite verbose).  Also, something needs to be done about putting the 
   terminal back into slow-scroll mode if that's the luser's preference (or 
   perhaps having EMACS itself use slow-scroll mode [lose, lose]). 
*/ 
 
#include <stdio.h> 
#include "config.h" 
#include "Trm.h" 
#include "cm.h" 
 
static int curHL, WindowSize; 
extern int InverseVideo; 
 
static 
HLmode (on) { 
    if (curHL == on) 
	return; 
    printf (on ? "\033[7m" : "\033[m"); 
    pad (1, 2000L); 
    curHL = on; 
} 
 
/* DEC sez pad=30, but what do they know? */ 
static 
inslines (n) { 
    printf ("\033[%d;%dr\033[%dH", curY + 1, WindowSize, curY + 1); 
    while (--n >= 0) { 
	printf ("\033M"); 
	pad (WindowSize - curY, 2000L);	/* 2ms per line */ 
    } 
    printf ("\033[r"); 
    cmat (0, 0); 
} 
 
static 
dellines (n) { 
    printf ("\033[%d;%dr\033[%dH", curY + 1, WindowSize, WindowSize); 
    while (--n >= 0) { 
	printf ("\n"); 
	pad (WindowSize - curY, 2000L); 
    } 
    printf ("\033[r"); 
    cmat (0, 0); 
} 
 
static 
writechars (start, end) 
register char *start, *end; 
{ 
    cmplus (end - start + 1); 
    while (start <= end) 
	putchar (*start++); 
} 
 
static 
update_end () 
{ 
  HLmode (0); 
} 
 
static 
blanks (n) 
register n; 
{ 
    cmplus (n); 
    while (--n >= 0) 
	putchar (' '); 
} 
 
static 
topos (row, column) { 
    cmgoto (row - 1, column - 1); 
} 
 
/* Ins/del costs are 14 + X*lines + 2_ms_pad*lines */ 
static 
init (BaudRate, tabok) { 
    tt.t_ILov = 14; 
    tt.t_DLov = 14; 
    tt.t_ILnov = 2; 
    tt.t_DLnov = 1; 
    tt.t_ILnpf = BaudRate / 5000.; 
    tt.t_DLnpf = tt.t_ILnpf; 
    UseTabs = tabok; 
    ScreenCols = tt.t_width; 
    cmcostinit (); 
} 
 
static 
reset () 
{ 
  /* ansi, !margn, no attr, jump, abs origin, clr screen, alt kpd*/ 
  printf ("\033<\033[r\033[m\033[?4;6l\033[2J\033="); /* Whew! */ 
  if (InverseVideo) printf ("\033[?5h"); /* Use inverse video */ 
  else printf ("\033[?5l"); 
  pad (1, 60000L); 
#ifdef SET_COLUMNS 
  /* set to 80 or 132 columns */ 
  printf (tt.t_width <= 80 ? "\033[?3l" : "\033[?3h"); 
  pad (1, 150000L); 
#endif 
  WindowSize = 24; 
  curHL = 0; 
  cmat (0, 0); 
} 
 
static 
cleanup () { 
    HLmode (0); 
} 
 
static 
wipeline () { 
    printf ("\033[K"); 
    pad (1, 10000L); 
} 
 
static 
wipescreen () { 
    printf ("\033[2J"); 
    pad (1, 100000L); 
} 
 
static 
window (n) { 
    if (n <= 0 || n > 24) 
	n = 24; 
    WindowSize = n; 
} 
 
/* Visible Bell for DT80/1 -ACT */ 
static 
flash () { 
    printf (InverseVideo ? "\033[?5l" : "\033[?5h"); 
    pad (1, 40000L); 
    printf (InverseVideo ? "\033[?5h" : "\033[?5l"); 
} 
 
TrmVT100 (term) 
char *term; 
{ 
    char tbuf[1024];		/* ACT Try for termcap's co# */ 
 
    tt.t_INSmode = NoOperation; 
    tt.t_HLmode = HLmode; 
    tt.t_inslines = inslines; 
    tt.t_dellines = dellines; 
    tt.t_blanks = blanks; 
    tt.t_init = init; 
    tt.t_cleanup = cleanup; 
    tt.t_wipeline = wipeline; 
    tt.t_wipescreen = wipescreen; 
    tt.t_topos = topos; 
    tt.t_reset = reset; 
    tt.t_delchars = 0; 
    tt.t_writechars = writechars; 
    tt.t_UpdateEnd = update_end; 
    tt.t_window = window; 
    tt.t_flash = flash; 
    tt.t_ICov = MissingFeature; 
    tt.t_DCov = MissingFeature; 
    tt.t_length = 24; 
    tt.t_width = 80; 
    if (term && tgetent (tbuf, term) > 0) 
	if ((tt.t_width = tgetnum ("co")) < 0) 
	    tt.t_width = 80; 
 
    Wcm_clear (); 
    Up = "1\033M"; 
    Down = "\n"; 
    Left = "\b"; 
    Right = "1\033[C"; 
    Home = "1\033[H"; 
    CR = "\r"; 
    Tab = "3\t"; 
    TabWidth = 8; 
    AbsPosition = "5\033[%i%d;%dH"; 
    MagicWrap = 1; 
    ScreenRows = tt.t_length; 
    ScreenCols = tt.t_width; 
#ifdef VT100_INVERSE 
    InverseVideo = 1; 
#endif 
    Wcm_init (); 
} 
