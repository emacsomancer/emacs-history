/* Lisp object printing and output streams. 
   Copyright (C) 1985 Richard M. Stallman. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but without any warranty.  No author or distributor 
accepts responsibility to anyone for the consequences of using it 
or for whether it serves any particular purpose or works at all, 
unless he says so in writing. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on all copies.  */ 
 
 
#include "config.h" 
#include <stdio.h> 
#undef NULL 
#include "lisp.h" 
 
#ifndef standalone 
#include "buffer.h" 
#include "window.h" 
#include "process.h" 
#endif 
 
Lisp_Object Vstandard_output, Qstandard_output; 
 
/* Low level output routines for charaters and strings */ 
 
/* Lisp functions to do output using a stream 
 must have the stream in a variable called printcharfun 
 and must start with PRINTPREPARE and end with PRINTFINISH. 
 Use PRINTCHAR to output one character, 
 or call strout to output a block of characters. 
 Also, each one must have the declarations 
   struct buffer *old = bf_cur; 
   int old_dot = -1, start_dot; 
   Lisp_Object original; 
*/  
 
#define PRINTPREPARE \ 
   original = printcharfun; \ 
   if (NULL (printcharfun)) printcharfun = Qt; \ 
   if (XTYPE (printcharfun) == Lisp_Buffer) \ 
     { if (XBUFFER (printcharfun) != bf_cur) SetBfp (XBUFFER (printcharfun)); \ 
       printcharfun = Qnil;}\ 
   if (XTYPE (printcharfun) == Lisp_Marker) \ 
     { if (XMARKER (original)->buffer != bf_cur) \ 
         SetBfp (XMARKER (original)->buffer); \ 
       old_dot = dot; \ 
       SetDot (marker_position (printcharfun)); \ 
       start_dot = dot; \ 
       printcharfun = Qnil;} 
 
#define PRINTFINISH \ 
   if (XTYPE (original) == Lisp_Marker) \ 
     Fset_marker (original, make_number (dot), Qnil); \ 
   if (old_dot >= 0) \ 
     SetDot ((old_dot >= start_dot ? dot - start_dot : 0) + old_dot); \ 
   if (old != bf_cur) \ 
     SetBfp (old) 
 
#define PRINTCHAR(ch) printchar (ch, printcharfun) 
 
/* Buffer for output destined for minibuffer */ 
static char printbuf[MScreenWidth + 1]; 
/* Index of first unused element of above */ 
static int printbufidx; 
 
static void 
printchar (ch, fun) 
     unsigned char ch; 
     Lisp_Object fun; 
{ 
  Lisp_Object ch1; 
 
#ifndef standalone 
  if (EQ (fun, Qnil)) 
    { 
      QUIT; 
      InsCStr (&ch, 1); 
      return; 
    } 
  if (EQ (fun, Qt)) 
    { 
      if (minibuf_message != printbuf) 
	minibuf_message = printbuf, printbufidx = 0; 
      if (printbufidx < sizeof printbuf - 1) 
	printbuf[printbufidx++] = ch; 
      printbuf[printbufidx] = 0; 
      return; 
    } 
#endif standalone 
 
  XFASTINT (ch1) = ch; 
  call1 (fun, ch1); 
} 
 
static void 
strout (ptr, size, printcharfun) 
     char *ptr; 
     int size; 
     Lisp_Object printcharfun; 
{ 
  int i = 0; 
 
  if (EQ (printcharfun, Qnil)) 
    { 
      InsCStr (ptr, size >= 0 ? size : strlen (ptr)); 
      return; 
    } 
  if (EQ (printcharfun, Qt)) 
    { 
      if (minibuf_message != printbuf) 
	minibuf_message = printbuf, printbufidx = 0; 
      i = size >= 0 ? size : strlen (ptr); 
      if (i > sizeof printbuf - printbufidx - 1) 
	i = sizeof printbuf - printbufidx - 1; 
      bcopy (ptr, &printbuf[printbufidx], i); 
      printbufidx += i; 
      printbuf[printbufidx] = 0; 
      return; 
    } 
  if (size >= 0) 
    while (i < size) 
      PRINTCHAR (ptr[i++]); 
  else 
    while (ptr[i]) 
      PRINTCHAR (ptr[i++]); 
} 
 
DEFUN ("write-char", Fwrite_char, Swrite_char, 1, 2, 0, 
  "Output character CHAR to stream STREAM.\n\ 
STREAM defaults to the value of standard-output (which see).") 
  (ch, printcharfun) 
     Lisp_Object ch, printcharfun; 
{ 
  struct buffer *old = bf_cur; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  CHECK_NUMBER (ch, 0); 
  PRINTPREPARE; 
  PRINTCHAR (XINT (ch)); 
  PRINTFINISH; 
  return ch; 
} 
 
write_string (data, size) 
     char *data; 
     int size; 
{ 
  struct buffer *old = bf_cur; 
  Lisp_Object printcharfun; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  printcharfun = Vstandard_output; 
 
  PRINTPREPARE; 
  strout (data, size, printcharfun); 
  PRINTFINISH; 
} 
 
write_string_1 (data, size, printcharfun) 
     char *data; 
     int size; 
     Lisp_Object printcharfun; 
{ 
  struct buffer *old = bf_cur; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  PRINTPREPARE; 
  strout (data, size, printcharfun); 
  PRINTFINISH; 
} 
 
 
#ifndef standalone 
 
temp_output_buffer_setup (bufname) 
    char *bufname; 
{ 
  struct buffer *old = bf_cur; 
  Lisp_Object buf; 
 
  Fset_buffer (Fget_buffer_create (build_string (bufname))); 
 
  Ferase_buffer (); 
 
  XSETTYPE (buf, Lisp_Buffer); 
  XSETBUFFER (buf, bf_cur); 
  specbind (Qstandard_output, buf); 
 
  SetBfp (old); 
} 
 
Lisp_Object 
internal_with_output_to_temp_buffer (bufname, function, args) 
     char *bufname; 
     Lisp_Object (*function) (); 
     Lisp_Object args; 
{ 
  int count = specpdl_ptr - specpdl; 
  Lisp_Object buf, val; 
 
  temp_output_buffer_setup (bufname); 
  buf = Vstandard_output; 
 
  val = function (args); 
 
  temp_output_buffer_show (buf); 
 
  unbind_to (count); 
  return val; 
} 
 
DEFUN ("with-output-to-temp-buffer", Fwith_output_to_temp_buffer, Swith_output_to_temp_buffer, 
       1, UNEVALLED, 0, 
  "Binding  standard-output  to buffer named BUFNAME, execute BODY then display the buffer.\n\ 
The buffer is cleared out initially, and marked as unmodified when done.\n\ 
All output done by BODY is inserted in that buffer by default.\n\ 
It is displayed in another window, but not selected.\n\ 
The value of the last form in BODY is returned.") 
  (args) 
     Lisp_Object args; 
{ 
  struct gcpro gcpro1; 
  Lisp_Object name; 
  int count = specpdl_ptr - specpdl; 
  Lisp_Object buf, val; 
 
  GCPRO1(args); 
  name = Feval (Fcar (args)); 
  UNGCPRO; 
 
  temp_output_buffer_setup (XSTRING (name)->data); 
  buf = Vstandard_output; 
 
  val = Fprogn (args); 
 
  temp_output_buffer_show (buf); 
 
  unbind_to (count); 
  return val; 
} 
#endif standalone 
 
static void print (); 
 
DEFUN ("terpri", Fterpri, Sterpri, 0, 1, 0, 
  "Output a newline to STREAM (or value of standard-output).") 
  (printcharfun) 
     Lisp_Object printcharfun; 
{ 
  struct buffer *old = bf_cur; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  if (NULL (printcharfun)) 
    printcharfun = Vstandard_output; 
  PRINTPREPARE; 
  PRINTCHAR ('\n'); 
  PRINTFINISH; 
  return Qt; 
} 
 
DEFUN ("prin1", Fprin1, Sprin1, 1, 2, 0, 
  "Output the printed representation of OBJECT, any Lisp object.\n\ 
Quoting characters are used, to make output that  read  can handle\n\ 
whenever this is possible.\n\ 
Output stream is STREAM, or value of standard-output (which see).") 
  (obj, printcharfun) 
     Lisp_Object obj, printcharfun; 
{ 
  struct buffer *old = bf_cur; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  if (NULL (printcharfun)) 
    printcharfun = Vstandard_output; 
  PRINTPREPARE; 
  print (obj, printcharfun, 1); 
  PRINTFINISH; 
  return obj; 
} 
 
DEFUN ("princ", Fprinc, Sprinc, 1, 2, 0, 
  "Output the printed representation of OBJECT, any Lisp object.\n\ 
No quoting characters are used; no delimiters are printed around\n\ 
the contents of strings.\n\ 
Output stream is STREAM, or value of standard-output (which see).") 
  (obj, printcharfun) 
     Lisp_Object obj, printcharfun; 
{ 
  struct buffer *old = bf_cur; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  if (NULL (printcharfun)) 
    printcharfun = Vstandard_output; 
  PRINTPREPARE; 
  print (obj, printcharfun, 0); 
  PRINTFINISH; 
  return obj; 
} 
 
DEFUN ("print", Fprint, Sprint, 1, 2, 0, 
  "Output the printed representation of OBJECT, with newline before and space after.\n\ 
Quoting characters are used, to make output that  read  can handle\n\ 
whenever this is possible.\n\ 
Output stream is STREAM, or value of standard-output (which see).") 
  (obj, printcharfun) 
     Lisp_Object obj, printcharfun; 
{ 
  struct buffer *old = bf_cur; 
  int old_dot = -1; 
  int start_dot; 
  Lisp_Object original; 
 
  if (NULL (printcharfun)) 
    printcharfun = Vstandard_output; 
  PRINTPREPARE; 
  PRINTCHAR ('\n'); 
  print (obj, printcharfun, 1); 
  PRINTCHAR ('\n'); 
  PRINTFINISH; 
  return obj; 
} 
 
static void 
print (obj, printcharfun, escapeflag) 
     Lisp_Object obj, printcharfun; 
     int escapeflag; 
{ 
  char buf[30]; 
 
  switch (XTYPE (obj)) 
    { 
    case Lisp_Int: 
      sprintf (buf, "%d", XINT (obj)); 
      strout (buf, -1, printcharfun); 
      break; 
 
    case Lisp_String: 
      if (!escapeflag) 
	strout (XSTRING (obj)->data, XSTRING (obj)->size, printcharfun); 
      else 
	{ 
	  int i; 
	  unsigned char *p = XSTRING (obj)->data; 
 
	  PRINTCHAR ('\"'); 
	  for (i = XSTRING (obj)->size; i > 0; i--) 
	    { 
	      unsigned char c = *p++; 
	      if (c == '\"' || c == '\\') 
		PRINTCHAR ('\\'); 
	      PRINTCHAR (c); 
	    } 
	  PRINTCHAR ('\"'); 
	} 
      break; 
 
    case Lisp_Symbol: 
      { 
	int i, confusing; 
	unsigned char *p = XSYMBOL (obj)->name->data; 
	unsigned char *end = p + XSYMBOL (obj)->name->size; 
 
	if (p != end && (*p == '-' || *p == '+')) p++; 
        while (p != end && *p >= '0' && *p <= '9') 
	  p++; 
        confusing = (end == p); 
 
	p = XSYMBOL (obj)->name->data; 
	while (p != end) 
	  { 
	    char c = *p++; 
	    if (escapeflag) 
	      { 
		if (c == '\"' || c == '\\' || c == '\'' || c == ';' || c == '#' || 
		    c == '(' || c == ')' || c == ',' || c =='.' || c == '`' || 
		    c == '[' || c == ']' || c == '?' || c <= 040 || confusing) 
		  PRINTCHAR ('\\'), confusing = 0; 
	      } 
	    PRINTCHAR (c); 
	  } 
      } 
      break; 
 
    case Lisp_Cons: 
      PRINTCHAR ('('); 
      { 
	int i = 0; 
	while (LISTP (obj)) 
	  { 
	    if (i++) 
	      PRINTCHAR (' '); 
	    print (Fcar (obj), printcharfun, escapeflag, 0); 
	    obj = Fcdr (obj); 
	  } 
      } 
      if (!NULL (obj)) 
	{ 
	  strout (" . ", 3, printcharfun); 
	  print (obj, printcharfun, escapeflag, 0); 
	} 
      PRINTCHAR (')'); 
      break; 
 
    case Lisp_Vector: 
      PRINTCHAR ('['); 
      { 
	register int i; 
	Lisp_Object tem; 
	for (i = 0; i < XVECTOR (obj)->size; i++) 
	  { 
	    if (i) PRINTCHAR (' '); 
	    tem = XVECTOR (obj)->contents[i]; 
	    print (tem, printcharfun, escapeflag, 0); 
	  } 
      } 
      PRINTCHAR (']'); 
      break; 
 
#ifndef standalone 
    case Lisp_Buffer: 
      if (escapeflag) 
	{ 
	  strout ("#<buffer ", -1, printcharfun); 
	  strout (XSTRING (XBUFFER (obj)->name)->data, -1, printcharfun); 
	  PRINTCHAR ('>'); 
	} 
      else 
	strout (XSTRING (XBUFFER (obj)->name)->data, -1, printcharfun); 
      break; 
 
    case Lisp_Process: 
      if (escapeflag) 
	{ 
	  strout ("#<process ", -1, printcharfun); 
	  strout (XSTRING (XPROCESS (obj)->name)->data, -1, printcharfun); 
	  PRINTCHAR ('>'); 
	} 
      else 
	strout (XSTRING (XPROCESS (obj)->name)->data, -1, printcharfun); 
      break; 
 
    case Lisp_Window: 
      strout ("#<window ", -1, printcharfun); 
      sprintf (buf, "%d", XWINDOW (obj)->sequence_number.i); 
      strout (buf, -1, printcharfun); 
      if (!NULL (XWINDOW (obj)->buffer)) 
	{ 
	  strout (" on ", -1, printcharfun); 
	  strout (XSTRING (XBUFFER (XWINDOW (obj)->buffer)->name)->data, 
		  -1, printcharfun); 
	} 
      PRINTCHAR ('>'); 
      break; 
 
    case Lisp_Marker: 
      strout ("#<marker at ", -1, printcharfun); 
      sprintf (buf, "%d", XMARKER (obj)->bufpos); 
      strout (buf, -1, printcharfun); 
      strout (" in ", -1, printcharfun); 
      if (!(XMARKER (obj)->buffer)) 
	strout ("no buffer", -1, printcharfun); 
      else 
	strout (XSTRING (XMARKER (obj)->buffer->name)->data, -1, printcharfun); 
      PRINTCHAR ('>'); 
      break; 
#endif standalone 
 
    case Lisp_Subr: 
      strout ("#<subr ", -1, printcharfun); 
      strout (XSUBR (obj)->symbol_name, -1, printcharfun); 
      PRINTCHAR ('>'); 
      break; 
    } 
} 
 
void 
syms_of_print () 
{ 
  DefLispVar ("standard-output", &Vstandard_output, 
    "Function print uses by default for outputting a character.\n\ 
This may be any function of one argument.\n\ 
It may also be a buffer (output is inserted before dot)\n\ 
or a marker (output is inserted and the marker is advanced)\n\ 
or the symbol t (output appears in the minibuffer line)."); 
  Vstandard_output = Qt; 
  Qstandard_output = intern ("standard-output"); 
  staticpro (&Qstandard_output); 
 
  defsubr (&Sprin1); 
  defsubr (&Sprinc); 
  defsubr (&Sprint); 
  defsubr (&Sterpri); 
  defsubr (&Swrite_char); 
#ifndef standalone 
  defsubr (&Swith_output_to_temp_buffer); 
#endif standalone 
} 
