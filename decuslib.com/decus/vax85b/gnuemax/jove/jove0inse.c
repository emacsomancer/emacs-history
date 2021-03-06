/*R
   Jonathan Payne at Lincoln-Sudbury Regional High School 5/25/83 
   
   Insert routines: the routine to Yank from the kill buffer 
   and to insert lines, and characters into the buffer.  */ 
 
#include "jove.h" 
#include "termcap.h" 
 
extern char	*indexs(); 
char		*ToComment(); 
 
/* Make a newline after `after' or course in `buf' */ 
 
Line * 
listput(buf, after) 
Buffer	*buf; 
Line	*after; 
{ 
	Line	*newline = nbufline(); 
 
	if (after == 0) {	/* First line in this list */ 
		buf->b_first = buf->b_last = buf->b_dot = newline; 
		newline->l_next = newline->l_prev = 0; 
		return newline; 
	} 
	newline->l_prev = after; 
	newline->l_next = after->l_next; 
	after->l_next = newline; 
	if (newline->l_next) 
		newline->l_next->l_prev = newline; 
	else 
		if (buf) 
			buf->b_last = newline; 
	return newline; 
}	 
 
/* Divide the current line and move the current line to the next one */ 
 
LineInsert() 
{ 
	register int	num = exp; 
	char	newline[LBSIZE]; 
	Line	*newdot, 
		*olddot; 
	int	oldchar; 
 
	exp = 1; 
	olddot = curline; 
	oldchar = curchar; 
	strcpy(newline, &linebuf[curchar]); 
 
	newdot = curline; 
	while (num--) { 
		newdot = listput(curbuf, newdot);	/* Put after newdot */ 
		newdot->l_dline = NullLine; 
	} 
	linebuf[curchar] = '\0';	/* Shorten this line */ 
	SavLine(curline, linebuf); 
	makedirty(curline); 
	curline = newdot; 
	curchar = 0; 
	strcpy(linebuf, newline); 
	makedirty(curline); 
	SetModified(curbuf); 
	IFixMarks(olddot, oldchar, curline, curchar); 
}	 
 
whitesp(from, to) 
register char	*from, 
		*to; 
{ 
	char	c; 
	int	oldchar = curchar; 
 
	while ((c = *from++) && (c == ' ' || c == '\t')) 
		insert(c, to, curchar++, 1, LBSIZE); 
	SetModified(curbuf); 
	IFixMarks(curline, oldchar, curline, curchar); 
} 
 
SelfInsert() 
{ 
	int	i, 
		num; 
 
	if (MinorMode(OverWrite)) { 
		for (i = 0, num = exp, exp = 1; i < num; i++) { 
			int	pos = calc_pos(linebuf, curchar); 
 
			if (!eolp()) { 
				if (linebuf[curchar] == '\t') { 
					if ((pos + 1) == ((pos + tabstop) - (pos % tabstop))) 
						DelNChar(); 
				} else 
					DelNChar(); 
			} 
			Insert(LastKeyStruck); 
		} 
	} else 
		Insert(LastKeyStruck); 
 
	if (MinorMode(Fill) && (calc_pos(linebuf, curchar) > RMargin) && 
	    (LastKeyStruck != ' ')) 
		AtMargin(); 
} 
 
InsertC() 
{ 
	int	c; 
 
	c = (*Getchar)(); 
	if (c == CTL(J) || c == CTL(M)) 
		LineInsert(); 
	else if (c != CTL(@)) 
		Insert(c); 
} 
 
Insert(c) 
{ 
	SetModified(curbuf); 
	makedirty(curline); 
	insert(c, linebuf, curchar, exp, LBSIZE); 
	IFixMarks(curline, curchar, curline, curchar + exp); 
	curchar += exp; 
}	 
 
/* Tab in to the right place for mode */ 
 
Tab() 
{ 
	if (last_cmd == TABCMD) { 
		Insert('\t'); 
		this_cmd = TABCMD; 
		return; 
	} 
	switch (curbuf->b_major) { 
		case CMODE: 
			if (bolp()) { 
				DelWtSpace(); 
				c_indent(); 
			} 
			break; 
		case CLUMODE: 
		case EUCLIDMODE: 
		case PASCALMODE: 
		case ASMMODE: 
			if (bolp()) { 
				DelWtSpace(); 
				ind_prev(linebuf, curline, curchar); 
				makedirty(curline); 
				this_cmd = TABCMD; 
				return; 
			} 
			break; 
		case TEXT: 
		case FUNDMODE: 
		case TEXMODE: 
		case SCRIBEMODE: 
		default: ; 
	} 
	Insert('\t'); 
	this_cmd = TABCMD; 
} 
 
QuotChar() 
{ 
	int	c; 
 
	if ((c = (*Getchar)()) != CTL(@) && c != CTL(J)) 
		Insert(c); 
} 
 
/* Insert the paren.  If in C mode and c is a '}' then insert the 
   '}' in the "right" place for C indentation; that is indented  
   the same amount as the matching '{' is indented. */ 
 
int	PDelay = 5;	/* 1/2 a second */ 
 
DoParen() 
{ 
	Bufpos	*bp; 
	int	nx, 
		c = LastKeyStruck; 
 
	if (c != ')' && c != '}' && c != ']' && c != '>') { 
		Insert(c); 
		complain((char *) 0); 
	} 
 
	/* If we're not a blank line, BUT we're at the end of the line, 
	   we insert a newline BEFORE we indent the '}' to the right place. 
	   If we aren't at the end of the line, we skip the paren and  
	   just insert it. */ 
 
	if (c == '}' && MajorMode(CMODE)) { 
		if (!blnkp(linebuf)) { 
			if (eolp()) 
				LineInsert(); 
			else 
				goto skip; 
		} 
		Bol(); 
		DelWtSpace(); 
		c_indent(); 
	} 
skip:	 
	Insert(c); 
 
	if (MinorMode(ShowMatch)) { 
		redisplay(); 
		BackChar();	/* Back onto the ')' */ 
		if (NotInQuotes(linebuf, curchar)) { 
			if (bp = m_paren(c, curwind->w_top)) { 
				nx = in_window(curwind, bp->p_line); 
				if (nx != -1) { 
					Bufpos	b; 
 
					DOTsave(&b); 
					SetDot(bp); 
					SitFor(PDelay); 
					SetDot(&b); 
				} 
			} 
		} 
		ForChar(); 
	} 
} 
 
 
#define FILLPREFSIZ 128 
char	fill_prefix[FILLPREFSIZ] = {0}; 
 
static 
InComment(buf, c) 
	char *buf; 
	int c; 
{ 
	register int cmt_start, cmt_end, cmt_len; 
	register char *cp = buf; 
 
	cmt_len = strlen(CommentBegin); 
	while (cp = ToComment(cp)) { 
		cmt_start = (int)(cp - buf) + cmt_len; 
		if (c < cmt_start) 
			return(0); 
		if (*CommentEnd) 
			cmt_end = (int)(indexs(CommentEnd, cp + cmt_len) - buf); 
		else 
			cmt_end = strlen(buf); 
		if (c <= cmt_end) 
			return(cp - buf); 
	} 
	return(0); 
} 
 
AtMargin() 
{ 
	int cmt_start; 
 
	if (cmt_start = InComment(linebuf, curchar)) { 
		register int i, comment, n; 
		cmt_start = calc_pos(linebuf, cmt_start); 
		comment = cmt_start + strlen(CommentBegin); 
		n = curchar; 
		do { 
			i = calc_pos(linebuf, curchar); 
			BackWord(); 
		} while (i > RMargin && i > comment); 
		n -= curchar; 
		ins_str(CommentEnd); 
		LineInsert(); 
		set_column(linebuf, curchar, cmt_start); 
		ins_str(CommentBegin); 
		Insert(' '); 
		curchar += n; 
	} else { 
		if (fill_prefix[0] == '\0') { 
			DoJustify(curline, curchar, curline, strlen(linebuf), 1); 
		} else { 
			register int n = curchar; 
			BackWord(); 
			n -= curchar; 
			DelWtSpace(); 
			LineInsert(); 
			ins_str(fill_prefix); 
			curchar += n; 
		} 
	} 
} 
 
FillPrefix() 
{ 
	int fillpref = curchar; 
 
	if (curchar >= FILLPREFSIZ) 
		fillpref = FILLPREFSIZ - 1; 
	bcopy(linebuf, fill_prefix, fillpref); 
	fill_prefix[fillpref] = '\0'; 
	s_mess("Fill prefix = \"%s\"", fill_prefix); 
} 
 
LineAI() 
{ 
	DelWtSpace(); 
	Newline(); 
	if (!MinorMode(Indent)) { 
		ind_prev(linebuf, curline, curchar); 
	} 
}	 
 
IndentNewLine() 
{ 
	Eol(); 
	LineAI(); 
} 
 
Newline() 
{ 
	/* If there is more than 2 blank lines in a row then don't make 
	   a newline, just move down one. */ 
 
	register char c; 
 
	if (exp == 1 && eolp() && TwoBlank()) { 
		SetLine(curline->l_next); 
		return; 
	} 
	while (curchar > 0 && ((c = linebuf[curchar - 1]) == ' ' || c == '\t')) 
		DelPChar(); 
	LineInsert(); 
	if (MinorMode(Indent)) 
		ind_prev(linebuf, curline, curchar); 
} 
 
ins_str(str) 
register char	*str; 
{ 
	register char	c; 
	int	pos = curchar; 
 
	while (c = *str++) { 
		if (c == '\n') 
			continue; 
		insert(c, linebuf, curchar++, 1, LBSIZE); 
	} 
	IFixMarks(curline, pos, curline, curchar); 
	makedirty(curline); 
} 
 
OpenLine() 
{ 
	int	num = exp; 
 
	LineInsert();	/* Open the lines... */ 
	DoTimes(BackChar, num); 
} 
 
Bufpos * 
DoYank(fline, fchar, tline, tchar, atline, atchar, whatbuf) 
Line	*fline, *tline, *atline; 
Buffer	*whatbuf; 
{ 
	register Line	*newline; 
	static Bufpos	bp; 
	char	save[LBSIZE], 
		buf[LBSIZE]; 
	Line	*startline = atline; 
	int	startchar = atchar; 
 
	SetModified(curbuf); 
	lsave(); 
	ignore(getright(atline, genbuf)); 
	strcpy(save, &genbuf[atchar]); 
 
	ignore(getright(fline, buf)); 
	if (fline == tline) 
		buf[tchar] = '\0'; 
 
	linecopy(genbuf, atchar, &buf[fchar]); 
	atline->l_dline = putline(genbuf); 
	makedirty(atline); 
 
	fline = fline->l_next; 
	while (fline != tline->l_next) { 
		newline = listput(whatbuf, atline); 
		newline->l_dline = fline->l_dline; 
		makedirty(newline); 
		fline = fline->l_next; 
		atline = newline; 
		atchar = 0; 
	} 
 
	ignore(getline(atline->l_dline, genbuf)); 
	atchar += tchar; 
	linecopy(genbuf, atchar, save); 
	atline->l_dline = putline(genbuf); 
	makedirty(atline); 
	IFixMarks(startline, startchar, atline, atchar); 
	bp.p_line = atline; 
	bp.p_char = atchar; 
	this_cmd = YANKCMD; 
	getDOT();			/* Whatever used to be in linebuf */ 
	return &bp; 
} 
 
YankPop() 
{ 
	Line	*line, 
		*last; 
	Mark	*mp = CurMark(); 
	Bufpos	*dot; 
 
	if (last_cmd != YANKCMD) 
		complain("Yank something first!"); 
 
	lfreelist(reg_delete(mp->m_line, mp->m_char, curline, curchar)); 
 
	/* Now must find a recently killed region. */ 
 
	killptr--; 
	for (;;) { 
		if (killptr < 0) 
			killptr = NUMKILLS - 1; 
		if (killbuf[killptr]) 
			break; 
		killptr--; 
	} 
 
	this_cmd = YANKCMD; 
 
	line = killbuf[killptr]; 
	last = lastline(line); 
	dot = DoYank(line, 0, last, length(last), curline, curchar, curbuf); 
	MarkSet(CurMark(), curline, curchar); 
	SetDot(dot); 
} 
 
/* This is an attempt to reduce the amount of memory taken up by each line. 
   Without this each malloc of a line uses sizeof (line) + sizeof(HEADER) 
   where line is 3 words and HEADER is 1 word. 
   This is going to allocate memory in chucks of CHUNKSIZE * sizeof (line) 
   and divide each chuck into lineS.  A line is free in a chunk when its 
   line->l_dline == 0, so linefree sets dline to 0. */ 
 
#define CHUNKSIZE	200	/* This is reasonable ... believe me! */ 
 
struct chunk { 
	int	c_nlines;	/* Number of lines in this chunk 
				   (so they don't all have to be CHUNKSIZE long) */ 
	Line	*c_block;	/* Chunk of memory */ 
	struct chunk	*c_nextfree;	/* Next chunk of lines */ 
}; 
 
static struct chunk	*fchunk = 0; 
 
static Line	*ffline = 0;	/* First free line */ 
 
freeline(line) 
register Line	*line; 
{ 
	line->l_dline = 0; 
	line->l_next = ffline; 
	if (ffline) 
		ffline->l_prev = line; 
	line->l_prev = 0; 
	ffline = line; 
} 
 
lfreelist(first) 
register Line	*first; 
{ 
	if (first) 
		lfreereg(first, lastline(first)); 
} 
 
/* Append region from line1 to line2 onto the free list of lines */ 
 
lfreereg(line1, line2) 
register Line	*line1, 
		*line2; 
{ 
	register Line	*next, 
			*last = line2->l_next; 
 
	while (line1 != last) { 
		next = line1->l_next; 
		freeline(line1); 
		line1 = next; 
	} 
} 
 
newchunk() 
{ 
	register Line	*newline; 
	register int	i; 
	struct chunk	*f; 
	int	nlines = CHUNKSIZE; 
 
	f = (struct chunk *) malloc((unsigned) sizeof (struct chunk)); 
	if (f == 0) 
		return 0; 
	while (nlines > 0) { 
		f->c_block = (Line *) malloc((unsigned) (sizeof (Line) * nlines)); 
		if (f->c_block != 0) 
			break; 
		nlines /= 2; 
	} 
	if (nlines <= 0) 
		return 0; 
 
	f->c_nlines = nlines; 
	for (i = 0, newline = f->c_block; i < nlines; newline++, i++) 
		freeline(newline); 
	f->c_nextfree = fchunk; 
	fchunk = f; 
	return 1; 
} 
 
/* New BUFfer LINE */ 
 
Line * 
nbufline() 
{ 
	register Line	*newline; 
 
	if (ffline == 0) {	/* No free list */ 
		if (newchunk() == 0) 
			complain("out of lines"); 
	} 
	newline = ffline; 
	ffline = ffline->l_next; 
	if (ffline) 
		ffline->l_prev = 0; 
	return newline; 
} 
 
/* Remove the free lines, in chunk c, from the free list because they are 
   no longer free. */ 
 
remfreelines(c) 
register struct chunk	*c; 
{ 
	register Line	*lp; 
	register int	i; 
 
	for (lp = c->c_block, i = 0; i < c->c_nlines; i++, lp++) { 
		if (lp->l_prev) 
			lp->l_prev->l_next = lp->l_next; 
		else 
			ffline = lp->l_next; 
		if (lp->l_next) 
			lp->l_next->l_prev = lp->l_prev; 
	} 
} 
 
/* This is used to garbage collect the chunks of lines when malloc fails 
   and we are NOT looking for a new buffer line.  This goes through each 
   chunk, and if every line in a given chunk is not allocated, the entire 
   chunk is `free'd by "free()". */ 
 
GCchunks() 
{ 
	register struct chunk	*cp; 
	struct chunk	*prev = 0, 
			*next = 0; 
	register int	i; 
	register Line	*newline; 
 
	f_mess("Garbage collecting ..."); 
 
 	for (cp = fchunk; cp; cp = next) { 
		for (i = 0, newline = cp->c_block; i < cp->c_nlines; newline++, i++) 
			if (newline->l_dline != 0) 
				break; 
 
 		next = cp->c_nextfree; 
 
		if (i == cp->c_nlines) {		/* Unlink it!!! */ 
			if (prev) 
				prev->c_nextfree = cp->c_nextfree; 
			else 
				fchunk = cp->c_nextfree; 
			remfreelines(cp); 
			free((char *) cp->c_block); 
			free((char *) cp); 
		} else 
			prev = cp; 
	} 
} 
 
#ifdef lint 
Ignorl(a) 
long	a; 
{ 
	a = a; 
} 
#	define ignorl(a)	Ignorl(a) 
#else 
#	define ignorl(a)	a 
#endif 
 
DateEdit() 
{ 
	time_t	now; 
	char	*cp; 
	extern char	*ctime(); 
 
	ignorl(time(&now)); 
	cp = ctime(&now); 
	*(cp + strlen(cp) - 1) = 0;		/* Get rid of \n */ 
	ToFirst(); 
	ins_str(CommentBegin); 
	if (exp_p) { 
		exp_p = 0; exp = 1; 
		ins_str(" $Header:$ "); ins_str(CommentEnd); 
		LineInsert(); ins_str(CommentBegin); 
		ins_str(" $Log:$ "); ins_str(CommentEnd); 
		LineInsert(); 
		return; 
	} 
	Insert(' '); 
	if (curbuf->b_fname == 0) 
		ins_str("[No file]"); 
	else 
		ins_str(curbuf->b_fname); 
	Insert(','); 
	Insert(' '); 
	ins_str(cp); 
	ins_str(", ed "); 
	ins_str(getenv("USER")); 
#ifdef JOVE4.2 
	{ 
		register char hostname[32]; 
		*hostname = (char)0; 
		gethostname(hostname, sizeof(hostname)); 
		if (*hostname) { 
			Insert('@'); 
			ins_str(hostname); 
		} 
	} 
#endif 
	Insert(' '); 
	ins_str(CommentEnd); 
	LineInsert(); 
} 
 
 
CenterLine() 
{ 
	register char *cp; 
	register int len; 
 
	Eol(); 
	DelWtSpace(); 
	Bol(); 
	DelWtSpace(); 
	len = RMargin - strlen(linebuf); 
	len /= 2; 
	while (len > 0) { 
		if (len >= 8) { 
			Insert('\t'); 
			len -= 8; 
		} else { 
			Insert(' '); 
			len--; 
		} 
	} 
} 
 
 
char	CommentBegin[16], 
	CommentEnd[16]; 
int	CommentColumn = 40; 
 
char * 
ToComment(buf) 
	char *buf; 
{ 
	register char *cp = buf; 
 
	while (cp = indexs(CommentBegin, cp)) { 
		if (NotInQuotes(linebuf, (int)(cp - linebuf))) 
			break; 
	} 
	return(cp); 
} 
 
InsComment() 
{ 
	register char *cp; 
	register int i; 
	extern char *indexs(); 
 
	if (exp_p) { 
		CommentColumn = calc_pos(linebuf, curchar); 
		s_mess("Comment column = %d", CommentColumn); 
		return; 
	} 
	cp = ToComment(linebuf); 
	if (cp) { 
		curchar = (int)(cp - linebuf); 
		if (curchar) { 
			register char *c = linebuf; 
			while ((c < cp) && (*c == ' ' || *c == '\t')) { 
				c++; 
			} 
			if (c != cp) { 
				DelWtSpace(); 
				set_column(linebuf, curchar, CommentColumn); 
			} 
		} 
		curchar += strlen(CommentBegin); 
		while (linebuf[curchar] == ' ' || linebuf[curchar] == '\t') 
			curchar++; 
		return; 
	} 
	Eol(); 
	if (!blnkp(linebuf)) { 
		DelWtSpace(); 
		set_column(linebuf, curchar, CommentColumn); 
	} 
	ins_str(CommentBegin); 
	if (*CommentBegin) 
		Insert(' '); 
	i = strlen(CommentEnd); 
	if (i) { 
		Insert(' '); 
		ins_str(CommentEnd); 
		i++; 
	} 
	curchar -= i; 
} 
 
InsertNewComment() 
{ 
	char *cp; 
	int col; 
	cp = ToComment(linebuf); 
	col = cp ? calc_pos(linebuf, cp - linebuf) : CommentColumn; 
	Eol(); 
	LineInsert(); 
	if (col) set_column(linebuf, curchar, col); 
	if (*CommentBegin) { 
		ins_str(CommentBegin); 
		Insert(' '); 
	} 
	col = strlen(CommentEnd); 
	if (col) { 
		Insert(' '); 
		ins_str(CommentEnd); 
		col++; 
	} 
	curchar -= col; 
} 
 
static 
set_column(buf, cc, col) 
	char *buf; 
	int cc, col; 
{ 
	register int len, i, k; 
	register char *cp = buf; 
 
	len = calc_pos(buf, cc); 
	i = col - len; 
	if (i <= 0) { 
		Insert('\t'); 
		return; 
	} 
	while (i > 0) { 
		if (i < 8) { 
			Insert(' '); 
			i--; 
		} else { 
			Insert('\t'); 
			k = (8 - (len % 8)); 
			i -= k; 
			len += k; 
		} 
	} 
} 
