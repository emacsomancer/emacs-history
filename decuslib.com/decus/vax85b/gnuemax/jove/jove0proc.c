Y
/* 
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
   
   jove_proc.c 
 
   This file contains procedures to handle the shell to buffer commands 
   and buffer to shell commands.  It isn't especially neat, but I think 
   it is understandable. */ 
 
#include "jove.h" 
 
#include <sys/file.h> 
#include <signal.h> 
 
extern 		errno, sys_nerr; 
extern char	*sys_errlist[]; 
 
/* There should be a command to specify your own format for parsing.  It 
   shouldn't be a problem.  Right now, v7 C compiler error messages are 
   the same as messages from "{grep, fgrep, egrep} -n" and they go under 
   the parse-C-errors command, and v7 LINT error messages are under the 
   parse-LINT-errors command.  Cerrfmt and lerrfmt are search strings to 
   use for C-errors and LINT-errors respectively; \1 extracts the file name 
   and \2 extracts the line number. */ 
 
static char 
	*cerrfmt = "\\([^:]*\\):\\([0-9][0-9]*\\):", 
	*lerrfmt = "\"\\([^:]*\\)\", line \\([0-9][0-9]*\\):", 
	*Shflags = DFLT_SHFLAGS; 
 
char	*DefShell; 
 
struct error { 
	Buffer		*er_buf;	/* Buffer error is in */ 
	Line		*er_mess,	/* Actual error message */ 
			*er_text;	/* Actual error */ 
	int		er_char;	/* char pos of error */ 
	struct error	*er_next;	/* List of error */ 
}; 
 
struct error	*cur_error = 0, 
		*errorlist = 0; 
Buffer		*perr_buf = 0;	/* Buffer with error messages */ 
 
int	MakeAll = 0,		/* Not make -k */ 
	WtOnMk = 1;		/* Write the modified files when we make */ 
 
/* Add an error to the end of the list of errors.  This is used for 
   parse-{C,LINT}-errors and for the spell-buffer command */ 
 
struct error * 
AddError(newerror, errline, buf, line, charpos) 
struct error	*newerror; 
Line	*errline, 
	*line; 
Buffer	*buf; 
{ 
	if (newerror) { 
		newerror->er_next = (struct error *) 
				emalloc(sizeof (struct error)); 
		newerror = newerror->er_next; 
	} else { 
		if (errorlist)		/* Free up old errors */ 
			ErrFree(); 
		cur_error = newerror = errorlist = (struct error *) 
				emalloc(sizeof (struct error)); 
	} 
	newerror->er_buf = buf; 
	newerror->er_text = line; 
	newerror->er_char = charpos; 
	newerror->er_next = 0; 
	newerror->er_mess = errline; 
 
	return newerror; 
} 
 
CParse() 
{ 
	ErrParse(cerrfmt); 
} 
 
LintParse() 
{ 
	ErrParse(lerrfmt); 
} 
 
XParse() 
{ 
	char	*sstr; 
 
	sstr = ask(cerrfmt, "%s(type ^R for default) ", FuncName()); 
	ErrParse(sstr); 
} 
 
/* Parse for {C,LINT} errors (or anything that matches fmtstr) in the 
   current buffer.  Set up for the next-error command.  This is neat 
   because this will work for any kind of output that prints a file 
   name and a line number on the same line. */ 
 
ErrParse(fmtstr) 
char	*fmtstr; 
{ 
	Bufpos	*bp; 
	char	fname[100], 
		lineno[10]; 
	int	lnum, 
		last_lnum = 1; 
	struct error	*ep = 0; 
	Buffer	*buf, 
		*lastb = 0; 
	Line	*err_line;	 
 
	ErrFree();		/* This is important! */ 
	ToFirst(); 
	perr_buf = curbuf; 
	/* Find a line with a number on it */ 
	while (bp = dosearch(fmtstr, 1, 1)) { 
		SetDot(bp); 
		putmatch(1, fname, sizeof fname); 
		putmatch(2, lineno, sizeof lineno); 
		buf = do_find((Window *) 0, fname); 
		if (buf != lastb) { 
			lastb = buf; 
			last_lnum = 1; 
			err_line = buf->b_first; 
		} 
		lnum = atoi(lineno); 
		if (lnum == last_lnum)	/* One error per line is nicer */ 
			continue; 
		err_line = next_line(err_line, lnum - last_lnum); 
		if (err_line == 0) 
			continue;	/* Find_file must have failed ... */ 
		ep = AddError(ep, curline, buf, err_line, 0); 
		last_lnum = lnum; 
	} 
	ShowErr(); 
	exp = 1; 
} 
 
/* Free up all the errors */ 
 
ErrFree() 
{ 
	register struct error	*ep; 
 
	for (ep = errorlist; ep; ep = ep->er_next) 
		free((char *) ep); 
	errorlist = cur_error = 0; 
} 
 
/* Internal next error sets cur_error to the next error, taking the 
   argument count, supplied by the user, into consideration. */ 
 
char	errbounds[] = "You're at the %s error", 
	noerrs[] = "No errors!"; 
 
nexterror() 
{ 
	register int	i; 
 
	if (cur_error == 0) 
		complain(noerrs); 
	if (cur_error->er_next == 0) 
		complain(errbounds, "last"); 
 
	for (i = 0; i < exp; i++) 
		if (cur_error->er_next == 0) 
			break; 
		else 
			cur_error = cur_error->er_next; 
} 
 
/* Same as next error internal, except it goes in the opposite direction */ 
 
preverror() 
{ 
	struct error	*newe = 0, 
			*ep = errorlist; 
	register int	i; 
 
	if (cur_error == 0) 
		complain(noerrs); 
	if (cur_error == errorlist) 
		complain(errbounds, "first"); 
 
	for (i = 0; i < exp; i++) { 
		if (ep->er_next == cur_error) 
			break; 
		ep = ep->er_next; 
	} 
	if (i != exp) { 
		cur_error = errorlist; 
		return; 
	} 
	for (newe = errorlist; ep; ep = ep->er_next, newe = newe->er_next) 
		if (ep == cur_error) 
			break; 
	cur_error = newe; 
} 
 
NextError() 
{ 
	ToError(1); 
} 
 
PrevError() 
{ 
	ToError(0); 
} 
 
/* Go the the next error, if there is one.  Put the error buffer in 
   one window and the buffer with the error in another window. 
   It checks to make sure that the error actually exists. */ 
 
ToError(forward) 
{ 
	forward ? nexterror() : preverror();		/* Setup cur_error */ 
	exp = 1; 
	if (!inlist(perr_buf->b_first, cur_error->er_mess)) { 
		ErrFree(); 
		complain("Error buffer has been fotzed with"); 
	} 
	for (;;) { 
		if (inlist(cur_error->er_buf->b_first, cur_error->er_text)) 
			break; 
		/* If we get here, the line that contains the error has been 
		   deleted.  So we keep moving in the right direction until 
		   we find an error that hasn't been deleted. */ 
 
		forward ? nexterror() : preverror(); 
	} 
 
	ShowErr(); 
} 
 
/* Show the current error, i.e. put the line containing the error message 
   in one window, and the buffer containing the actual error in another 
   window. */ 
 
ShowErr() 
{ 
	Window	*err_wind, 
		*buf_wind; 
 
	if (cur_error == 0) 
		return; 
	err_wind = windlook(perr_buf->b_name); 
	buf_wind = windlook(cur_error->er_buf->b_name); 
 
	if (err_wind && !buf_wind) { 
		SetWind(err_wind); 
		pop_wind(cur_error->er_buf->b_name, 0, -1); 
		buf_wind = curwind; 
	} else if (!err_wind && buf_wind) { 
		SetWind(buf_wind); 
		pop_wind(perr_buf->b_name, 0, -1); 
		err_wind = curwind; 
	} else if (!err_wind && !buf_wind) { 
		pop_wind(perr_buf->b_name, 0, -1); 
		err_wind = curwind; 
		pop_wind(cur_error->er_buf->b_name, 0, -1); 
		buf_wind = curwind; 
	} 
 
	/* Put the current error message at the top of its Window */ 
	SetWind(err_wind); 
	SetLine(cur_error->er_mess); 
	SetTop(curwind, (curwind->w_line = cur_error->er_mess)); 
 
	/* Now go to the the line with the error in the other Window */ 
	SetWind(buf_wind); 
	DotTo(cur_error->er_text, cur_error->er_char); 
} 
 
/* 
   Run make or compile buffer file, first writing all the modified buffers 
   (if the WtOnMk flag is non-zero), parse the errors, and go to the first 
   error. 
*/ 
 
MakeErrors() 
{ 
	Window	*old = curwind; 
	int	status; 
	char	*args, **args_list, 
#ifndef VMUNIX 
		*errformat = cerrfmt; 
#else 
		*errformat = lerrfmt; 
#endif 
	extern	char **make_args(); 
 
	if (WtOnMk) 
		WtModBuf(); 
	if (exp_p) { 
		args = ask((char *)0, ": shell "); 
		status = UnixToBuf("make", 1, 1, 1, 
				   DefShell, DefShell, Shflags, args, 0); 
		errformat = cerrfmt; 
	} else if (makefile_exists()) { 
		if (MakeAll) 
			status = UnixToBuf("make", 1, 1, 1, 
					   "/bin/make", "Make", "-k", 0); 
		else { 
			args = ask((char *)-1, ": make "); 
			args_list = ((int)args == -1) ? 
				(char **)0 : 
				make_args("Make", args, 0); 
			status = UnixToBuf("make", 3, 1, 1, 
					   "/bin/make", args_list, 0); 
		} 
	} else { 
		extern char *Compiler, *CmpPath, *CmpFlags; 
		char *ComFlags; 
		if (Compiler) { 
			ComFlags = (CmpFlags ? CmpFlags : (char *)-1); 
			args = ask(ComFlags, Compiler); 
			args_list = ((int)args == -1) ? 
				make_args(CmpPath, 0, curbuf->b_fname) : 
				make_args(CmpPath, args, curbuf->b_fname); 
			status = UnixToBuf("make", 3, 1, 1, 
					   CmpPath, args_list, 0); 
			errformat = cerrfmt; 
		} else { 
			args = curbuf->b_fname; 
			if (args == 0) 
				args = "[None]"; 
			complain(sprint("Don't know how to compile %s", args)); 
		} 
	} 
	com_finish(status, "make"); 
 
	ErrParse(errformat); 
 
	if (!cur_error) 
		SetWind(old); 
} 
 
SpelBuffer() 
{ 
	char	*Spell = "Spell", 
		com[100]; 
	Window	*savewp = curwind; 
 
	WtModBuf(); 
	ignore(sprintf(com, "spell %s", curbuf->b_fname)); 
	ignore(UnixToBuf(Spell, 1, 1, 1, DefShell, DefShell, Shflags, com, 0)); 
	SetWind(savewp); 
	SpelParse(Spell); 
} 
 
SpelWords() 
{ 
	char	com[100], 
		*buftospel; 
	Buffer	*wordsb = curbuf; 
 
	ignore(sprintf(com, "%s(in buffer) ", FuncName())); 
	if ((buftospel = getbexist(com)) == 0) 
		return; 
	SetBuf(do_select(curwind, buftospel)); 
	SpelParse(wordsb->b_name); 
} 
 
SpelParse(bname) 
char	*bname; 
{ 
	Buffer	*buftospel, 
		*wordsb; 
	char	wordspel[100]; 
	Bufpos	*bp; 
	struct error	*ep = 0; 
 
	ErrFree();		/* This is important! */ 
 
	buftospel = curbuf; 
	wordsb = buf_exists(bname); 
	perr_buf = wordsb;	/* This is important (buffer containing 
				   error messages) */ 
	SetBuf(wordsb); 
	ToFirst(); 
	f_mess("Finding misspelled words ... "); 
	while (!lastp(curline)) { 
		ignore(sprintf(wordspel, "\\b%s\\b", linebuf)); 
		SetBuf(buftospel); 
		ToFirst(); 
		while (bp = dosearch(wordspel, 1, 1)) { 
			SetDot(bp); 
			ep = AddError(ep, wordsb->b_dot, buftospel, 
					  curline, curchar); 
		} 
		SetBuf(wordsb); 
		NextLine(); 
	} 
	add_mess("Done"); 
	SetBuf(buftospel); 
	ShowErr(); 
} 
 
/* Make a buffer name given the command `command', i.e. "fgrep -n foo *.c" 
   will return the buffer name "fgrep".  */ 
 
char * 
MakeName(command) 
char	*command; 
{ 
	static char	bufname[50]; 
	char	*cp = bufname, c; 
 
	while ((c = *command++) && (c == ' ' || c == '\t')) 
		; 
	do 
		*cp++ = c; 
	while ((c = *command++) && (c != ' ' && c != '\t')); 
	*cp = 0; 
	cp = rindex(bufname, '/'); 
	if (cp) 
		strcpy(bufname, cp + 1); 
	return bufname; 
} 
 
char	ShcomBuf[100] = {0};	/* I hope ??? */ 
 
ShToBuf() 
{ 
	char	bufname[100]; 
 
	strcpy(bufname, ask((char *) 0, "Buffer: ")); 
	DoShell(bufname, ask(ShcomBuf, "Command: ")); 
} 
 
ShellCom() 
{ 
	strcpy(ShcomBuf, ask(ShcomBuf, FuncName())); 
	DoShell(MakeName(ShcomBuf), ShcomBuf); 
} 
 
/* Run the shell command into `bufname'.  Empty the buffer except when we 
   give a numeric argument, in which case it inserts the output at the 
   current position in the buffer.  */ 
 
DoShell(bufname, command) 
char	*bufname, 
	*command; 
{ 
	Window	*savewp = curwind; 
	int	status; 
 
	exp = 1; 
	status = UnixToBuf(bufname, 1, 1, !exp_p, DefShell, DefShell, Shflags, 
			command, 0); 
	com_finish(status, command); 
	SetWind(savewp); 
} 
 
com_finish(status, com) 
char	*com; 
{ 
	s_mess("\"%s\" completed %ssuccessfully", com, status ? "un" : NullStr); 
} 
 
dowait(pid, status) 
int	pid, 
	*status; 
{ 
#ifndef PROCS 
 
	int	rpid; 
 
	while ((rpid = wait(status)) != pid) 
		; 
#else 
 
#ifdef JOVE4.2 
#	include <sys/wait.h> 
#else 
#	include <wait.h> 
#endif 
 
	union wait	w; 
	int	rpid; 
 
	for (;;) { 
#ifndef VMUNIX 
		rpid = wait2(&w.w_status, 0); 
#else 
		rpid = wait3(&w.w_status, 0, 0); 
#endif 
		if (rpid == pid) { 
			if (status) 
				*status = w.w_status; 
			break; 
		} else 
			kill_off(rpid, w); 
	} 
#endif PROCS 
} 
 
/* Run the command to bufname, erase the buffer if clobber is non-zero, 
   and redisplay if disp is non-zero.  Leaves current buffer in `bufname' 
   and leaves any windows it creates lying around.  It's up to the caller 
   to fix everything up after we're done.  (Usually there's nothing to 
   fix up.) */ 
 
/* VARARGS3 */ 
 
UnixToBuf(bufname, sync, disp, clobber, func, args) 
char	*bufname, 
	*func; 
{ 
	int	p[2], 
		pid; 
	char	buff[LBSIZE]; 
	extern int	ninbuf; 
 
	message(sprint("Starting up %s ...", func)); 
	pop_wind(bufname, clobber, clobber ? PROCESS : FILE); 
	if (disp) 
		redisplay();		 
	exp = 1; 
 
	dopipe(p); 
	pid = fork(); 
	if (pid == -1) { 
		pclose(p); 
		complain("Cannot fork"); 
	} 
	if (pid == 0) { 
		ignore(close(0)); 
		ignore(open("/dev/null", 0)); 
		ignore(close(1)); 
		ignore(close(2)); 
		ignore(dup(p[1])); 
		ignore(dup(p[1])); 
		pclose(p); 
		if (sync == 3) 
			execv(func, (char **) args); 
		else 
			execv(func, (char **) &args); 
		ignore(write(1, "Execl failed", 12)); 
		_exit(1); 
	} else { 
		int	(*oldint)() = signal(SIGINT, SIG_IGN); 
		int	status = 0; 
 
#ifdef PROCS 
		sighold(SIGCHLD); 
#endif 
 
		ignore(close(p[1])); 
		io = p[0]; 
		if (sync) { 
			message("Chugging along..."); 
			if (disp) 
				redisplay(); 
			while (getfline(buff) != EOF) { 
				ins_str(buff); 
				LineInsert(); 
				/* Ninbuf set by getfLine */ 
				if (ninbuf <= 0) { 
					message("Really chugging along now..."); 
					if (disp) 
						redisplay(); 
				} 
			} 
		} 
		if (disp) 
			UpdateMesg(); 
		IOclose(); 
		ignorf(signal(SIGINT, oldint)); 
		if (sync) 
			dowait(pid, &status); 
#ifdef PROCS 
		sigrelse(SIGCHLD); 
#endif 
		return status; 
	} 
	return 0; 
} 
 
/* Send a region to shell.  Now we can beautify C and sort programs */ 
 
RegToShell() 
{ 
	char	com[100]; 
	Mark	*m = CurMark(); 
 
	strcpy(com, ask((char *) 0, "%s(through command) ", FuncName())); 
	if (!exp_p) { 
		exp_p = 1;	/* So it doesn't delete the region */ 
		exp = 0; 
	} 
	if (inorder(m->m_line, m->m_char, curline, curchar)) 
		RegToUnix(curbuf->b_name, 1, 1, m->m_line, m->m_char, 
					curline, curchar, com); 
	else 
		RegToUnix(curbuf->b_name, 1, 1, curline, curchar, m->m_line, 
					m->m_char, com); 
	message("Done"); 
} 
 
/* Writes the region to a tmp file and then run the command with input 
   from that file */ 
 
RegToUnix(bufname, sync, replace, line1, char1, line2, char2, func) 
char	*bufname; 
Line	*line1, 
	*line2; 
char	*func; 
{ 
	char	*fname = mktemp(TMPFILE); 
	char	com[100]; 
 
	if ((io = creat(fname, 0644)) == -1) 
		complain(IOerr("create", fname)); 
	putreg(line1, char1, line2, char2); 
	IOclose(); 
	if (replace) 
		DelReg(); 
	ignore(sprintf(com, "%s < %s", func, fname)); 
	ignore(UnixToBuf(bufname, sync, 0, 0, DefShell, DefShell, Shflags, com, 0)); 
	ignore(unlink(fname)); 
} 
 
 
#ifdef SENDMAIL 
/* 
 *	Primitive version of sending mail for now... 
 */ 
 
static char	*mailbp = (char *)0; 
 
SendMail() 
{ 
	mailbp = curbuf->b_name; 
	SetBuf(do_select(curwind, MAILBUFFER)); 
	if (!exp_p) { 
		initlist(curbuf); 
		ins_str("To: "); 
		LineInsert(); 
		ins_str("Subject: "); 
		LineInsert(); 
		LineInsert(); 
		ToFirst(); 
		Eol(); 
	} 
	SetUnmodified(curbuf); 
	TextMode(); 
	Recur(); 
	exit_sendmail(); 
} 
 
 
AbortSendmail() 
{ 
	SetUnmodified(curbuf); 
	if (RecDepth) 
		Leave(); 
	else 
		complain("Not in recursive edit?"); 
} 
 
 
static 
exit_sendmail() 
{ 
	Mark	*m; 
 
	if (curbuf->b_modified && strcmp(curbuf->b_name,MAILBUFFER) == 0) { 
		ToLast(); 
		LineInsert(); 
		SetMark(); 
		ToFirst(); 
		m = CurMark(); 
		ignore(Reg2Unix(curbuf->b_name, curline, curchar, 
			 m->m_line, m->m_char, 
			 SENDMAIL)); 
		message("Mail queued."); 
		SetUnmodified(curbuf); 
	} else { 
		message("Sendmail aborted."); 
	} 
	SetBuf(do_select(curwind, (mailbp ? mailbp : "Main"))); 
	mailbp = (char *)0; 
} 
 
 
/* feeds a region to a process, but doesn't wait for any output */ 
/* VARARGS3 */ 
 
Reg2Unix(bufname, line1, char1, line2, char2, func, args) 
char	*bufname; 
Line	*line1, 
	*line2; 
char	*func; 
{ 
	char	*fname = mktemp(TMPFILE); 
	int	pid; 
	int	i; 
 
	pop_wind(bufname, 0, PROCESS); 
	exp = 1; 
	if ((io = creat(fname, 0600)) == -1) 
		complain(IOerr("create", fname)); 
	message("Forking off..."); 
	pid = fork(); 
	if (pid == -1) { 
		complain("Cannot fork"); 
	} 
	if (pid == 0) { 
		sigset(SIGTSTP, SIG_IGN); 
		sigset(SIGTTIN, SIG_IGN); 
		sigset(SIGTTOU, SIG_IGN); 
		for (i = SIGHUP; i <= SIGQUIT; i++) 
			sigset(i, SIG_IGN); 
		putreg(line1, char1, line2, char2); 
		IOclose(); 
		ignore(close(0)); 
		ignore(open(fname, O_RDONLY)); 
		ignore(unlink(fname)); 
		ignore(lseek(0, 0, 0)); 
		ignore(close(1)); 
		ignore(open("/dev/null", 0)); 
		execv(func, (char **) &args); 
		ignore(write(2, "Execl failed", 12)); 
		_exit(1); 
	} 
	return (pid); 
} 
#endif 
 
#ifdef PROCS 
KillUnixProcess() 
{ 
	int pid = AskInt(FuncName(), "%d"); 
	if (pid) { 
		if (kill(pid, SIGKILL)) 
			complain(sys_errlist[errno]); 
	} 
} 
#endif 
