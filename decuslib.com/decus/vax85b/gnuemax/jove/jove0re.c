Y
/* 
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_re.c 
 
   Much of this code was taken from /usr/src/cmd/ed.c.  It has been 
   modified a lot and features have been added, but the basic algorithm 
   is the same. */ 
 
#include "jove.h" 
 
#define	CBRA		1	/* \( */ 
#define	CCHR		2	/* Normal comparisn */ 
#define	CDOT		4	/* Matches any character */ 
#define	CCL		6	/* [0-9] matches 0123456789 */ 
#define	NCCL		8	/* a '^' inside a [...] */ 
#define	CDOL		10	/* Match at end of the Line */ 
#define	CEOF		12	/* End of Line */ 
#define	CKET		14	/* \) */ 
#define	CBACK		16	/* \1 \2 \3 ... */ 
#define CIRC		18	/* At beginning of Line */ 
 
#define CWORD		20	/* A word character	\w */ 
#define CNWORD		22	/* Not a word character \W */ 
#define CBOUND		24	/* At a word boundary   \b */ 
#define CNBOUND		25	/* Not at a word boundary \B */ 
#define NCCHR		26	/* !(normal comparison) */ 
 
#define	STAR	01 
 
char	*loc1,		/* Beginning of found search */ 
	*loc2,		/* End of found search */ 
	*locs;		/* Where to start comparison */ 
 
#define	NBRA	5 
#define NALTS	10 
 
char	*braslist[NBRA], 
	*braelist[NBRA], 
	*alternates[NALTS]; 
int	nbra; 
 
#define SEARCHSIZE 100 
#define EOL	-1 
 
char	unmatched[] = "unmatched parens?"; 
char	toolong[] = "too long"; 
char	syntax[] = "syntax?"; 
 
char	*reptr,		/* Pointer to the expression to search for */ 
	rhsbuf[LBSIZE/2], 
	expbuf[ESIZE+4], 
	searchbuf[SEARCHSIZE]; 
int	dir, 
	REpeekc; 
 
int	CaseIgnore = 1;	/* non-zero when upper case matches lower case */ 
 
#define CharCom(a, b) (CaseIgnore ? (Upper(a) == Upper(b)) : (a == b)) 
 
REerror(str) 
char	*str; 
{ 
	complain("RE error: %s", str); 
} 
 
nextc() 
{ 
	int	c; 
 
	if (c = REpeekc) 
		REpeekc = 0; 
	else if (*reptr == 0) 
		c = EOL; 
	else 
		c = *reptr++; 
	return c; 
} 
 
QRepSearch() 
{ 
	replace(1, 0); 
} 
 
RepSearch() 
{ 
	replace(0, 0); 
} 
 
REQRepSearch() 
{ 
	replace(1, 1); 
} 
 
RERepSearch() 
{ 
	replace(0, 1); 
} 
 
/* Prompt for search and replacement string and do the substitution. 
   The point is saved and restored at the end. */ 
 
replace(query, re) 
{ 
	Mark	*save = MakeMark(curline, curchar, FLOATER); 
 
	REpeekc = 0; 
	reptr = ask(searchbuf[0] ? searchbuf : (char *) 0, FuncName()); 
	strcpy(searchbuf, reptr); 
	compile(EOL, re); 
			/* Compile search string complaining 
			   about errors before asking for  
			   the replacement string. */ 
	reptr = ask(NullStr, "%s%s with ", FuncName(), searchbuf); 
	compsub(EOL); 
		/* Compile replacement string */ 
	substitute(query); 
	ToMark(save); 
	DelMark(save); 
} 
 
/* Return a buffer location of where the search string in searchbuf is. 
   NOTE: this procedure used to munge all over linebuf, but doesn't 
   anymore (It uses genbuf instead!). */ 
 
Bufpos * 
dosearch(str, direction, magic) 
char	*str; 
{ 
	static Bufpos	ret; 
	Line	*a1; 
	int	fromchar; 
 
	dir = direction; 
	reptr = str;		/* Read from this string */ 
	REpeekc = 0; 
	compile(EOL, magic); 
 
	a1 = curline; 
	fromchar = curchar; 
	if (dir < 0) 
		fromchar--; 
	 
	if (fromchar >= strlen(linebuf) && dir > 0) 
		a1 = a1->l_next, fromchar = 0; 
	else if (fromchar < 0 && dir < 0) 
		a1 = a1->l_prev, fromchar = a1 ? length(a1) : 0; 
	for (;;) { 
		if (a1 == 0) 
			break; 
		if (charp()) { 
			a1 = curline; 
			break; 
		} 
		if (execute(fromchar, a1)) 
			break; 
		a1 = dir > 0 ? a1->l_next : a1->l_prev; 
		if (dir < 0 && !a1)	/* don't do the length if == 0 */ 
			break; 
		fromchar = dir > 0 ? 0 : length(a1); 
	} 
 
	if (a1) { 
		ret.p_line = a1; 
		ret.p_char = dir > 0 ? loc2 - genbuf : loc1 - genbuf; 
		return &ret; 
	} 
	return 0; 
} 
 
setsearch(str) 
char	*str; 
{ 
	strcpy(searchbuf, str); 
} 
 
ForSearch() 
{ 
	search(1, 0); 
} 
 
RevSearch() 
{ 
	search(0, 0); 
} 
 
REForSearch() 
{ 
	search(1, 1); 
} 
 
RERevSearch() 
{ 
	search(0, 1); 
} 
 
/* Prompts for a search string.  Complains if the string cannot be found. 
   Searches don't wrap line in ed/vi */ 
 
search(forward, re) 
{ 
	Bufpos	*newdot; 
 
	dir = forward ? 1 : -1; 
	reptr = ask(searchbuf[0] ? searchbuf : 0, FuncName()); 
 
	setsearch(reptr); 
	f_mess("%s", searchbuf); 
	newdot = dosearch(searchbuf, dir, re); 
	if (newdot == 0) 
		complain("Search for \"%s\" failed", searchbuf); 
	PushPntp(newdot->p_line); 
	SetDot(newdot); 
} 
 
/* Perform the substitution.  Both the replacement and search strings have  
   been compiled already.  This knows how to deal with query replace. */ 
 
substitute(query) 
{ 
	Line	*lp; 
	int	numdone = 0, 
		fromchar = curchar, 
		stop = 0; 
 
	dir = 1;	/* Always going forward */ 
	for (lp = curline; !stop && lp; lp = lp->l_next, fromchar = 0) { 
		while (!stop && execute(fromchar, lp)) { 
			DotTo(lp, loc2 - genbuf); 
			fromchar = curchar; 
			if (query) { 
 				message("Replace (Type '?' for help)? "); 
reswitch: 
				redisplay(); 
				switch (Upper(getchar())) { 
				case '.': 
					stop++;	/* Stop after this */ 
 
				case ' ': 
					break;	/* Do the replace */ 
 
				case '\177':	/* Skip this one */ 
					continue; 
 
				case 'R':	/* Allow user to move around */ 
				    { 
					Buffer	*oldbuf = curbuf; 
				    	Mark	*m; 
				    	char	pushexp[ESIZE + 4]; 
 
				    	fromchar = loc1 - genbuf; 
				    	m = MakeMark(curline, fromchar, FLOATER); 
				    	curchar = loc2 - genbuf; 
					message("Recursive edit ..."); 
				    	copynchar(pushexp, expbuf, ESIZE + 4); 
					Recur(); 
				    	copynchar(expbuf, pushexp, ESIZE + 4); 
					SetBuf(oldbuf); 
				    	ToMark(m); 
				    	DelMark(m); 
				    	fromchar = curchar; 
				    	lp = curline; 
					continue; 
				    } 
 
				case 'P':	/* Replace all */ 
					query = 0;	/* Replace all */ 
					break; 
 
				case '\n':	/* Stop this replace search */ 
				case '\r': 
					goto done; 
 
				default: 
					rbell(); 
				case '?': 
message("<SP> => yes, <DEL> => no, <CR> => stop, 'r' => recursive edit, 'p' => proceed"); 
					goto reswitch; 
				} 
 
			} 
			dosub(linebuf);		/* Do the substitution */ 
			numdone++; 
			SetModified(curbuf); 
			fromchar = curchar = loc2 - genbuf; 
			exp = 1; 
			makedirty(curline); 
			if (query) { 
				message(mesgbuf);	/* No blinking ... */ 
				redisplay();	/* Show the change */ 
			} 
			if (linebuf[fromchar] == 0) 
				break; 
		} 
	} 
done: 
	s_mess("%d substitution%s", numdone, numdone ? "s" : NullStr); 
} 
 
/* Compile the substitution string */ 
 
compsub(seof) 
{ 
	register int	c; 
	register char	*p; 
 
	p = rhsbuf; 
	for (;;) { 
		c = nextc(); 
		if (c == '\\') 
			c = nextc() | 0200; 
		if (c == seof) 
			break; 
		*p++ = c; 
		if (p >= &rhsbuf[LBSIZE/2]) 
			REerror(toolong); 
	} 
	*p++ = 0; 
} 
 
dosub(base) 
char	*base; 
{ 
	register char	*lp, 
			*sp, 
			*rp; 
	int	c; 
 
	lp = genbuf; 
	sp = base; 
	rp = rhsbuf; 
	while (lp < loc1) 
		*sp++ = *lp++; 
	while (c = *rp++ & 0377) { 
		if (c == '&') { 
			sp = place(LBSIZE, base, sp, loc1, loc2); 
			continue; 
		} else if (c & 0200 && (c &= 0177) >= '1' && c < nbra + '1') { 
			sp = place(LBSIZE, base, sp, braslist[c - '1'], braelist[c - '1']); 
			continue; 
		} 
		*sp++ = c & 0177; 
		if (sp >= &base[LBSIZE]) 
			REerror(toolong); 
	} 
	lp = loc2; 
	loc2 = genbuf + max(1, sp - base); 
			/* At least one character forward after the replace 
			   to prevent infinite number of replacements in the 
			   same place, e.g. replace "^" with "" */ 
	 
	while (*sp++ = *lp++) 
		if (sp >= &base[LBSIZE]) 
			len_error(ERROR); 
} 
 
char * 
place(size, base, sp, l1, l2) 
char	*base; 
register char	*sp, 
		*l1, 
		*l2; 
{ 
	while (l1 < l2) { 
		*sp++ = *l1++; 
		if (sp >= &base[size]) 
			len_error(ERROR); 
	} 
 
	return sp; 
} 
 
putmatch(which, buf, size) 
char	*buf; 
{ 
	*(place(size, buf, buf, braslist[which - 1], braelist[which - 1])) = 0; 
} 
 
/* Compile the string, that nextc knows about, into a sequence of 
   [TYPE][CHAR] where type says what kind of comparison to make 
   and CHAR is the character to compare with.  CHAR is actually 
   optional, like in the case of '.' there is no CHAR. */ 
 
compile(aeof, magic) 
{ 
	register int	xeof, 
			c; 
	register char	*ep; 
	char	*lastep, 
		bracket[NBRA], 
		*bracketp, 
		**alt = alternates; 
	int	cclcnt; 
 
	ep = expbuf; 
	*alt++ = ep; 
	xeof = aeof; 
	bracketp = bracket; 
 
	nbra = 0; 
	lastep = 0; 
	for (;;) { 
		if (ep >= &expbuf[ESIZE]) 
			REerror(toolong); 
		c = nextc(); 
		if (c == xeof) { 
			if (bracketp != bracket) 
				REerror(unmatched); 
			*ep++ = CEOF; 
			*alt = 0; 
			return; 
		} 
		if (c != '*') 
			lastep = ep; 
		if (!magic && index("*[.", c)) 
			goto defchar; 
		switch (c) { 
		case '\\': 
			if ((c = nextc()) == xeof) 
				complain("Expected a character after \\"); 
			if (!magic && index("()0123456789|wWbB", c)) 
				goto defchar; 
			switch (c) { 
			case '!': 
				*ep++ = NCCHR; 
				*ep++ = nextc(); 
				continue; 
 
			case 'w': 
				*ep++ = CWORD; 
				continue; 
 
			case 'W': 
				*ep++ = CNWORD; 
				continue; 
 
			case 'b': 
				*ep++ = CBOUND; 
				continue; 
 
			case 'B': 
				*ep++ = CNBOUND; 
				continue; 
 
			case '(': 
				if (nbra >= NBRA) 
					REerror(unmatched); 
				*bracketp++ = nbra; 
				*ep++ = CBRA; 
				*ep++ = nbra++; 
				continue; 
 
			case ')': 
				if (bracketp <= bracket) 
					REerror(unmatched); 
				*ep++ = CKET; 
				*ep++ = *--bracketp; 
				continue; 
			case '|':	/* Another alternative */ 
				if (alt >= &alternates[NALTS]) 
					REerror("Too many alternates"); 
				*ep++ = CEOF; 
				*alt++ = ep; 
				continue; 
 
			case '1': 
			case '2': 
			case '3': 
			case '4': 
			case '5': 
				*ep++ = CBACK; 
				*ep++ = c - '1'; 
				continue; 
 
			} 
			goto defchar; 
 
		case '.': 
			*ep++ = CDOT; 
			continue; 
 
		case '\n': 
			goto cerror; 
 
		case '*': 
			if (lastep == 0 || *lastep == CBRA || *lastep == CKET) 
				goto defchar; 
			*lastep |= STAR; 
			continue; 
 
		case '^': 
			if (ep != expbuf && ep[-1] != CEOF) 
				goto defchar; 
			*ep++ = CIRC; 
			continue; 
 
		case '$': 
			if (((REpeekc = nextc()) != xeof) && REpeekc != '\\') 
				goto defchar; 
			*ep++ = CDOL; 
			continue; 
 
		case '[': 
			*ep++ = CCL; 
			*ep++ = 0; 
			cclcnt = 1; 
			if ((c = nextc()) == '^') { 
				c = nextc(); 
				ep[-2] = NCCL; 
			} 
			do { 
				if (c == '\\') 
					c = nextc(); 
				if (c == '\n') 
					goto cerror; 
				if (c == xeof) 
					REerror("Missing ']'"); 
				*ep++ = c; 
				cclcnt++; 
				if (ep >= &expbuf[ESIZE]) 
					REerror(toolong); 
			} while ((c = nextc()) != ']'); 
			lastep[1] = cclcnt; 
			continue; 
 
		defchar: 
		default: 
			*ep++ = CCHR; 
			*ep++ = c; 
		} 
	} 
   cerror: 
	expbuf[0] = 0; 
	nbra = 0; 
	REerror(syntax); 
} 
 
LookingAt(expr, offset) 
char	*expr; 
{ 
	reptr = expr; 
	compile(EOL, 1); 
	return advance(linebuf + offset, expbuf); 
} 
 
/* Look for a match in `line' starting at `fromchar' from the beginning */ 
 
execute(fromchar, line) 
Line	*line; 
{ 
	register char	*p1, 
			c; 
 
	for (c = 0; c < NBRA; c++) { 
		braslist[c] = 0; 
		braelist[c] = 0; 
	} 
	ignore(getright(line, genbuf)); 
	locs = p1 = genbuf + fromchar; 
 
	if (dir < 0) 
		locs = genbuf; 
 
	/* fast check for first character */ 
 
	if (expbuf[0] == CCHR && !alternates[1]) { 
		c = expbuf[1]; 
		do { 
			if (!CharCom(*p1, c)) 
				continue; 
			if (advance(p1, expbuf)) { 
				loc1 = p1; 
				return 1; 
			} 
		} while (dir > 0 ? *p1++ : p1-- > genbuf); 
		return 0; 
	} 
	/* regular algorithm */ 
	do { 
		register char	**alt = alternates; 
 
		while (*alt) 
			if (advance(p1, *alt++)) { 
				loc1 = p1; 
				return 1; 
			} 
	} while (dir > 0 ? *p1++ : p1-- > genbuf); 
 
	return 0; 
} 
 
/* Does the actual comparison, lp is the buffer line and ep is the 
   expression pointer.  Returns 1 if it found a match or 0 if it 
   didn't. */ 
 
advance(lp, ep) 
register char	*ep, 
		*lp; 
{ 
	register char	*curlp; 
	int	i; 
 
	for (;;) switch (*ep++) { 
 
	case CIRC:	/* If we are not at the beginning 
			   of the line there isn't a 
			   match. */ 
 
		if (lp == genbuf) 
			continue; 
		return 0; 
 
	case CCHR:	/* Normal comparison, if they're the same, continue */ 
		if (CharCom(*ep++, *lp++)) 
			continue; 
		return 0; 
 
	case CDOT:	/* Any character matches.  Can only fail if we are 
			   at the end of the buffer line already */ 
		if (*lp++) 
			continue; 
		return 0; 
 
	case CDOL:	/* Only works if we are at the end of the Line */ 
		if (*lp == 0) 
			continue; 
		return 0; 
 
	case CEOF:	/* If we get here then we made it! */ 
		loc2 = lp; 
		return 1; 
 
	case CWORD:	/* Is this a word character? */ 
		if (*lp && isword(*lp)) { 
			lp++; 
			continue; 
		} 
		return 0; 
 
	case CNWORD:	/* Is this NOT a word character */ 
		if (*lp && !isword(*lp)) { 
			lp++; 
			continue; 
		} 
		return 0; 
 
	case CBOUND:	/* Is this at a word boundery */ 
		if (	(*lp == 0) ||		/* end of Line */ 
			(lp == genbuf) ||	/* Beginning of Line */ 
			(isword(*lp) != isword(*(lp - 1)))	) 
					/* Characters are of different 
					   type i.e one is part of a 
					   word and the other isn't. */ 
			continue; 
		return 0; 
 
	case CNBOUND:	/* Is this NOT a word boundery */ 
		if (	(*lp) &&	/* Not end of the Line */ 
			(isword(*lp) == isword(*(lp - 1)))	) 
			continue; 
		return 0; 
 
	case CCL:	/* Is this a member of [] */ 
		if (cclass(ep, *lp++, 1)) { 
			ep += *ep; 
			continue; 
		} 
		return 0; 
 
	case NCCL:	/* Is this NOT a member of [] */ 
		if (cclass(ep, *lp++, 0)) { 
			ep += *ep; 
			continue; 
		} 
		return 0; 
 
	case CBRA: 
		braslist[*ep++] = lp; 
		continue; 
 
	case CKET: 
		braelist[*ep++] = lp; 
		continue; 
 
	case CBACK: 
		if (braelist[i = *ep++] == 0) 
			goto badcase; 
		if (backref(i, lp)) { 
			lp += braelist[i] - braslist[i]; 
			continue; 
		} 
		return 0; 
 
	case CWORD | STAR:	/* Any number of word characters */ 
		curlp = lp; 
		while (*lp++ && isword(*(lp - 1))) 
			; 
		goto star; 
 
	case CNWORD | STAR: 
		curlp = lp; 
		while (*lp++ && !isword(*(lp - 1))) 
			; 
		goto star; 
 
	case CBACK | STAR: 
		curlp = lp; 
		while (backref(i, lp)) 
			lp += braelist[i] - braslist[i]; 
		while (lp >= curlp) { 
			if (advance(lp, ep)) 
				return 1; 
			lp -= braelist[i] - braslist[i]; 
		} 
		continue; 
 
	case CDOT | STAR:	/* Any number of any character */ 
		curlp = lp; 
		while (*lp++) 
			; 
		goto star; 
 
	case CCHR | STAR:	/* Any number of the current character */ 
		curlp = lp; 
		while (CharCom(*lp++, *ep)) 
			; 
		ep++; 
		goto star; 
 
	case CCL | STAR: 
	case NCCL | STAR: 
		curlp = lp; 
		while (cclass(ep, *lp++, ep[-1] == (CCL | STAR))) 
			; 
		ep += *ep; 
		goto star; 
 
	star: 
		do { 
			lp--; 
			if (lp < locs) 
				break; 
			if (advance(lp, ep)) 
				return 1; 
		} while (lp > curlp); 
		return 0; 
 
	default: 
badcase: 
		REerror("advance"); 
	} 
	/* NOTREACHED */ 
} 
 
backref(i, lp) 
register int	i; 
register char	*lp; 
{ 
	register char	*bp; 
 
	bp = braslist[i]; 
	while (*bp++ == *lp++) 
		if (bp >= braelist[i]) 
			return 1; 
	return 0; 
} 
 
cclass(set, c, af) 
register char	*set, 
		c; 
{ 
	register int	n; 
	char	*base; 
 
	if (c == 0) 
		return 0; 
	n = *set++; 
	base = set; 
	while (--n) { 
		if (*set == '-' && set > base) { 
			if (c >= *(set - 1) && c <= *(set + 1)) 
				return af; 
			set++; 
		} else if (*set++ == c) 
			return af; 
	} 
	return !af; 
} 
 
#define FOUND	1 
#define GOBACK	2 
 
static char	IncBuf[100], 
		*incp = 0; 
int	FirstInc = 0; 
jmp_buf	incjmp; 
 
static fSearchChar = '\023'; 
static rSearchChar = '\022'; 
 
IncFSearch() 
{ 
	fSearchChar = LastKeyStruck; 
	IncSearch(1); 
} 
 
IncRSearch() 
{ 
	rSearchChar = LastKeyStruck; 
	IncSearch(-1); 
} 
 
extern char	searchbuf[]; 
 
IncSearch(direction) 
{ 
	Bufpos	bp; 
	Line	*olddot = curline; 
 
	DOTsave(&bp); 
	switch (setjmp(incjmp)) { 
	case GOBACK: 
		SetDot(&bp); 
		break; 
 
	case 0: 
		IncBuf[0] = 0; 
		incp = IncBuf; 
		FirstInc = 1; 
		isearch(direction, 0, 0); 
		break; 
 
	case FOUND: 
	default: 
		PushPntp(olddot); 
		break; 
	} 
	message(IncBuf);	/* Show the search string */ 
	setsearch(IncBuf);	/* This is now the global string */ 
} 
 
/* A character with the 400 bit set means it was quoted by the 
   user.  A character with the 200 bit means that meta mode 
   exists and they really mean ESC <c & 0177> */ 
 
isearch(direction, failing, ctls) 
{ 
	int	c; 
	Bufpos	bp, 
		*newdot; 
	int	nextfail = failing;	/* Kind of a temp variable */ 
 
	DOTsave(&bp); 
	for (;;) { 
		message(NullStr); 
		if (failing) 
			add_mess("Failing "); 
		if (direction < 0) 
			add_mess("reverse-"); 
		add_mess("I-search: %s", IncBuf); 
 
		switch (c = (*Getchar)()) { 
		case '\177': 
		case CTL(H): 
			if (!ctls)	/* If we didn't repeated the command */ 
				if (incp > IncBuf) 
					*--incp = 0; 
			return; 
 
		case CTL(G): 
			longjmp(incjmp, GOBACK); 
			/* Easy way out! */ 
 
		case '\r': 
		case '\n': 
		case CTL([): 
			longjmp(incjmp, FOUND); 
 
		case '\\': 
			*incp++ = '\\'; 
			add_mess("\\"); 
			/* Fall into ... */ 
 
		case CTL(Q): 
		case CTL(^): 
			add_mess(""); 
			c = (*Getchar)() | 0400;	/* Tricky! */ 
 
		default: 
			if (c == fSearchChar || c == rSearchChar) { 
			if (FirstInc && incp) {	/* We have been here before */ 
				FirstInc = 0; 
				strcpy(IncBuf, searchbuf); 
				incp = IncBuf + strlen(IncBuf); 
				failing = 0; 
			} 
			/* If we are not failing, OR we are failing BUT we 
			   are changing direction, then allow another 
			   search */ 
			if (!failing || ((direction == -1 && c == fSearchChar) || 
					(direction == 1 && c == rSearchChar))) { 
				newdot = dosearch(IncBuf, c == rSearchChar ? -1 : 1, 0); 
				if (newdot) { 
					SetDot(newdot); 
					nextfail = 0; 
				} else 
					nextfail = 1; 
				isearch(c == rSearchChar ? -1 : 1, nextfail, 1); 
				nextfail = failing; 
			} else 
				rbell(); 
			break; 
			} 
			if (c & 0400) 
				c &= 0177; 
			else {	/* Check for legality */ 
				if ((c & 0200) || (c < ' ' && c != '\t')) { 
					ignore(PushBack(c)); 
					longjmp(incjmp, FOUND); 
				} 
			} 
			FirstInc = 0;	/* Not first time anymore! */ 
			*incp++ = c;	/* Always put in string */ 
			*incp = 0; 
			if (!failing) { 
				if (direction > 0 && CharCom(linebuf[curchar], c)) 
					ForChar(); 
				else if (direction < 0 && CharCom(linebuf[curchar + strlen(IncBuf) - 1], c)) 
					; 
				else if (!failing) { 
					newdot = dosearch(IncBuf, direction, 0); 
					if (newdot == 0) 
						nextfail++; 
					else 
						SetDot(newdot); 
				} 
			} 
			isearch(direction, nextfail, 0); 
			nextfail = failing; 
			/* Reset this to what it was, because if we 
			 * are here again, then we must have deleted 
			 * back. 
			 */ 
		} 
		SetDot(&bp); 
	} 
} 
 
/* C tags package.  */ 
 
/* string comparison function for tags package */ 
 
StrNEq(s1, s2, n) 
register char	*s1, *s2; 
register int	n; 
{ 
	while (n > 0 && *s1 && *s2 && CharCom(*s1, *s2)) { 
		n--; 
		s1++; 
		s2++; 
	} 
	return (n && *s2); 
} 
 
/* Find the line with the correct tagname at the beginning of the line. 
   then put the data into the struct, and then return a pointer to 
   the struct.  0 if an error. */ 
 
look_up(sstr, filebuf, name) 
register char	*name; 
char	*sstr, 
	*filebuf; 
{ 
	register int	namlen = strlen(name); 
	char	line[LBSIZE]; 
	register char	*cp; 
 
	if ((io = open("tags", 0)) == -1) { 
		message(IOerr("open", "tags")); 
		return 0; 
	} 
	while (getfline(line) != EOF) { 
		if (!CharCom(line[0],*name) || StrNEq(name, line, namlen)) 
			continue; 
		if (line[namlen] != '\t') 
			continue; 
		else { 
			char	*endp, 
				*newp; 
 
			if (cp = index(line, '\t')) 
				cp++; 
			else 
tagerr:				complain("Bad tag file format"); 
			if (endp = index(cp, '\t')) 
				*endp = 0; 
			else 
				goto tagerr; 
			strcpy(filebuf, cp); 
			if ((newp = index(endp + 1, '/')) || 
					(newp = index(endp + 1, '?'))) 
				newp++; 
			else 
				goto tagerr; 
			strcpy(sstr, newp); 
			sstr[strlen(sstr) - 1] = 0; 
			IOclose(); 
			return 1; 
		} 
	} 
	IOclose(); 
	return 0; 
} 
 
TagError(tag) 
char	*tag; 
{ 
	s_mess("tag: %s not found", tag); 
} 
 
/* 
 * Find_tag searches for the 'tagname' in the file "tags". 
 * The "tags" file is in the format generated by "/usr/bin/ctags". 
 */ 
 
find_tag(tagname) 
char	*tagname; 
{ 
	char	filebuf[50], 
		sstr[100]; 
	Bufpos	*bp; 
 
	if (look_up(sstr, filebuf, tagname) == 0) { 
		TagError(tagname); 
		return; 
	} 
 
	SetBuf(do_find(curwind, filebuf)); 
	ToFirst(); 
 
	if ((bp = dosearch(sprint(sstr, tagname), 1, 1)) == 0) 
		TagError(tagname); 
	else 
		SetDot(bp); 
} 
 
/* Called from user typing ^X t */ 
 
FindTag() 
{ 
	char	*tagname; 
 
	tagname = (char *) ask((char *) 0, FuncName()); 
	find_tag(tagname); 
} 
 
