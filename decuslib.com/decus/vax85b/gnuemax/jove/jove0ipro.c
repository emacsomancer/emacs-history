Y
#include "jove.h" 
#ifdef JOVE4.2 
#	include <sys/wait.h> 
#else 
#	include <wait.h> 
#endif 
#include <signal.h> 
#include <sys/ioctl.h> 
 
#ifdef VENIX 
#include <sgtty.h> 
#define killpg kill 
#endif 
 
typedef struct process	Process; 
 
#define DEAD	1	/* Dead but haven't informed user yet */ 
#define STOPPED	2	/* Job stopped */ 
#define RUNNING	3	/* Just running */ 
#define NEW	4	/* This process is brand new */ 
 
/* If process is dead, flags says how. */ 
#define EXITED	1 
#define KILLED	2 
 
int	 PMaxLines = 2000;	/* Max number of lines before we 
				   start deleting lines in chunks 
				   of PDELNUM */ 
#define PDELNUM		100	/* Number of lines to delete */ 
 
#define isdead(p)	(p == 0 || proc_state(p) == DEAD || p->p_toproc == -1) 
#define proc_buf(p)	(p->p_buffer->b_name) 
#define proc_cmd(p)	(p->p_name) 
#define proc_state(p)	(p->p_state) 
 
struct process { 
	Process	*p_next; 
	int	p_toproc,	/* read p_fromproc and write p_toproc */ 
		p_portpid,	/* Pid of child (the portsrv) */ 
		p_rpid,		/* Pid of real child i.e. not portsrv */ 
		p_nlines;	/* Lines in this buffer */ 
	Buffer	*p_buffer;	/* Add output to end of this buffer */ 
	Mark	*p_mark;	/* Marks end of last output */ 
	char	*p_name;	/* ... */ 
	char	p_state,	/* State */ 
		p_howdied,	/* Killed? or Exited? */ 
		p_reason,	/* If signaled, p_reason is the signal; else 
				   it is the the exit code */ 
		p_eof;		/* Received EOF, so can be free'd up */ 
	int	(*p_func)();	/* Function to call when process dies */ 
} *procs = 0, 
  *cur_proc = 0; 
 
static char	proc_prompt[30] = "% "; 
 
int	ProcInput, 
	ProcOutput, 
 
	NumProcs = 0; 
 
static char	*signames[] = { 
	0, 
	"Hangup", 
	"Interrupt",	 
	"Quit", 
	"Illegal instruction", 
	"Trace/BPT trap", 
	"IOT trap", 
	"EMT trap", 
	"Floating exception", 
	"Killed", 
	"Bus error", 
	"Segmentation violation", 
	"Bad system call", 
	"Broken pipe", 
	"Alarm clock", 
	"Terminated", 
	"Signal 16", 
	"Stopped (signal)", 
	"Stopped", 
	"Continued", 
	"Child exited", 
	"Stopped (tty input)", 
	"Stopped (tty output)", 
	"TTY input interrupt", 
	"Cputime limit exceeded", 
	"Filesize limit exceeded", 
}; 
 
static char * 
pstate(p) 
Process	*p; 
{ 
	switch (proc_state(p)) { 
	case NEW: 
		return "Pre-birth"; 
 
	case STOPPED: 
		return "Stopped"; 
 
	case RUNNING: 
		return "Running"; 
 
	case DEAD: 
		if (p->p_howdied == EXITED) { 
			if (p->p_reason == 0) 
				return "Done"; 
			return sprint("Exit %d", p->p_reason); 
		} 
		return signames[p->p_reason]; 
 
	default: 
		return "Unknown state"; 
	} 
} 
 
NoProcs() 
{ 
	register Process	*p; 
 
	for (p = procs; p; p = p->p_next) 
		if (!isdead(p)) 
			break; 
	if (p == 0) 
		return; 
	if (*ask("", "You have processes lying around; should I kill them? ") != 'y') 
		return; 
	for (; p; p = p->p_next) 
		if (!isdead(p)) 
			proc_kill(p, SIGKILL); 
} 
 
static Process * 
proc_exists(name) 
char	*name; 
{ 
	register Process	*p; 
 
	for (p = procs; p; p = p->p_next) 
		if (strcmp(proc_buf(p), name) == 0) { 
			ignore(pstate(p)); 
			if (p->p_eof) { 
				DealWDeath(); 
				return 0; 
			} 
			break; 
		} 
 
	return p; 
} 
 
static Process * 
proc_pid(pid) 
{ 
	register Process	*p; 
 
	for (p = procs; p; p = p->p_next) 
		if (p->p_portpid == pid) 
			break; 
	return p; 
} 
 
ProcPrompt() 
{ 
	char	*prompt; 
 
	prompt = ask(proc_prompt, FuncName()); 
	strncpy(proc_prompt, prompt, sizeof proc_prompt); 
} 
 
assign_p() 
{ 
	register Process	*p; 
 
	for (p = procs; p; p = p->p_next) 
		if (p->p_buffer == curbuf) { 
			cur_proc = p; 
			break; 
		} 
} 
 
pbuftiedp(b) 
Buffer	*b; 
{ 
	register Process	*p; 
 
	for (p = procs; p; p = p->p_next) 
		if (p->p_buffer == b) 
			complain("There is a process tied to %s.", proc_buf(p)); 
} 
 
procs_read() 
{ 
	static struct header { 
		int	pid; 
		int	nbytes; 
	} header; 
	int	n; 
	long	nbytes; 
 
#ifdef PROCS 
	sighold(INPUT_SIG); 
#endif 
	for (;;) { 
#ifdef VENIX 
		struct sgttyb sg; 
		ioctl(ProcInput, TIOCQCNT, &sg); 
		nbytes = sg.sg_ispeed; 
#else 
		ioctl(ProcInput, FIONREAD, (char *) &nbytes); 
#endif 
		if (nbytes < sizeof header) 
			break; 
		n = read(ProcInput, (char *) &header, sizeof header); 
		if (n != sizeof header) 
			finish(1); 
		read_proc(header.pid, header.nbytes); 
	} 
	redisplay(); 
#ifdef PROCS 
	sigrelse(INPUT_SIG); 
#endif 
} 
 
read_proc(pid, nbytes) 
int	pid; 
register int	nbytes; 
{ 
	register Process	*p; 
	int	n, 
		redisp = 0; 
	char	ibuf[512]; 
 
	if ((p = proc_pid(pid)) == 0) { 
		printf("\rPanic (!): Unknown pid(%d)", pid); 
		return; 
	} else if (proc_state(p) == NEW) { 
		int	rpid; 
		/* Pid of real child, not of portsrv */ 
 
		doread(ProcInput, (char *) &rpid, nbytes); 
		nbytes -= sizeof rpid; 
		p->p_rpid = rpid; 
		p->p_state = RUNNING; 
	} 
 
	if (nbytes == EOF) {		/* Okay to clean up this process */ 
		p->p_eof = 1; 
		NumProcs--;	/* As far as getch() in main is concerned */ 
		return; 
	} 
 
	while (nbytes > 0) { 
		n = min((sizeof ibuf) - 1, nbytes); 
		doread(ProcInput, ibuf, n); 
		ibuf[n] = 0;	/* Null terminate for convenience */ 
		nbytes -= n; 
		proc_rec(p, ibuf); 
	} 
} 
 
/* Process receive: receives the characters in buf, and appends them to 
   the buffer associated with p. */ 
 
static 
proc_rec(p, buf) 
register Process	*p; 
char	*buf; 
{ 
	Buffer	*saveb = curbuf; 
	Window	*w; 
	register char	*cp, 
			*tcp; 
	int	wasrt; 
 
	if (curwind->w_bufp == p->p_buffer) 
		w = curwind; 
	else 
		w = windbp(p->p_buffer);	/* Is this window visible? */ 
	SetBuf(p->p_buffer); 
	ToLast(); 
 
	exp_p = 0; 
	exp = 1; 
	for (cp = buf; *cp; cp = tcp) { 
		for (tcp = cp; *tcp != '\0' && *tcp != '\n'; ++tcp) 
			; 
		wasrt = (*tcp == '\n'); 
		if (wasrt) 
			*tcp++ = 0; 
		ins_str(cp); 
		if (wasrt) { 
			LineInsert(); 
			if (p->p_nlines++ >= PMaxLines) { 
				Line	*top = p->p_buffer->b_first; 
 
				lfreelist(reg_delete(top, 0, next_line(top, PDELNUM), 0)); 
				p->p_nlines -= PDELNUM; 
			} 
			if (w) { 
				w->w_line = curline; 
				w->w_char = curchar; 
				redisplay(); 
			} 
		} 
	} 
	if (w) { 
		w->w_line = curline; 
		w->w_char = curchar; 
	} 
	SetModified(curbuf); 
	MarkSet(p->p_mark, curline, curchar); 
	SetBuf(saveb); 
} 
 
proc_kill(p, sig) 
Process	*p; 
{ 
	if (p == 0) 
		return; 
	if (killpg(p->p_rpid, sig) == -1) 
		s_mess("Cannot kill %s", proc_buf(p)); 
} 
 
ProcInt() 
{ 
	proc_kill(cur_proc, SIGINT); 
} 
 
ProcQuit() 
{ 
	proc_kill(cur_proc, SIGQUIT); 
} 
 
#ifndef VENIX 
ProcStop() 
{ 
	proc_kill(cur_proc, SIGSTOP); 
	proc_state(cur_proc) = STOPPED; 
} 
 
ProcCont() 
{ 
	if (proc_state(cur_proc) == STOPPED) { 
		proc_kill(cur_proc, SIGCONT); 
		proc_state(cur_proc) = RUNNING; 
	} 
} 
#endif 
 
ProcKill() 
{ 
	proc_kill(cur_proc, SIGKILL); 
} 
 
static 
proc_close(p) 
Process	*p; 
{ 
	ignore(close(p->p_toproc)); 
	p->p_toproc = -1;	/* Writes will fail */ 
} 
 
/* Deal with a process' death.  proc_rec turns on the FREEUP bit when it 
   it gets the "EOF" from portsrv.  FREEUP'd processes get unlinked from 
   the list, and the proc stucture and proc_buf(p) get free'd up, here. */ 
 
static 
DealWDeath() 
{ 
	register Process	*p, 
				*next, 
				*prev = 0; 
	 
	for (p = procs; p; p = next) { 
		next = p->p_next; 
		if (!p->p_eof) { 
			prev = p; 
			continue; 
		} 
		if (cur_proc == p) 
			cur_proc = next ? next : prev; 
		proc_close(p); 
		free((char *) p->p_name); 
		free((char *) p); 
		if (prev) 
			prev->p_next = next; 
		else 
			procs = next; 
	} 
} 
 
ProcList() 
{ 
	register Process	*p; 
	char	*fmt = "%-15s  %-15s  %s"; 
	int	what;	 
 
	if (procs == 0) { 
		message("No subprocesses"); 
		return; 
	} 
	if (UseBuffers) 
		TellWBuffers("Process list", 0); 
	else 
		TellWScreen(0); 
 
	ignore(DoTell(fmt, "Buffer", "Status", "Command name")); 
	ignore(DoTell(fmt, "------", "------", "------- ----")); 
	for (p = procs; p; p = p->p_next) { 
		what = DoTell(fmt, proc_buf(p), pstate(p), p->p_name); 
		if (what == ABORT || what == STOP) 
			break; 
	} 
	DealWDeath(); 
	if (UseBuffers) { 
		ToFirst();		/* Go to the beginning of the file */ 
		NotModified(); 
	} 
	StopTelling(); 
} 
 
ip_mark(l, c) 
Line	*l; 
int	c; 
{ 
	if (isdead(cur_proc)) 
		return; 
	MarkSet(cur_proc->p_mark, l, c); 
} 
 
ProcNewline() 
{ 
	register Process	*p = cur_proc; 
 
	if (isdead(p)) 
		return; 
 
	if (eobp()) { 
		p->p_nlines++; 
		LineInsert(); 
		RegToProc(); 
		MarkSet(p->p_mark, curline, curchar); 
	} else { 
		Bol(); 
		if (LookingAt(proc_prompt, curchar)) 
			SetDot(dosearch(proc_prompt, 1, 1)); 
		SetMark(); 
		Eol(); 
		CopyRegion(); 
		ToLast(); 
		Yank(); 
		LineInsert(); 
		p->p_nlines++; 
		RegToProc(); 
	} 
} 
 
RegToProc() 
{ 
	register Process	*p = cur_proc; 
	register Mark	*mp = p->p_mark; 
 
	if (isdead(p) || p->p_buffer != curbuf || mp == 0) 
		return; 
 
	io = p->p_toproc; 
	if (inorder(mp->m_line, mp->m_char, curline, curchar)) 
		putreg(mp->m_line, mp->m_char, curline, curchar); 
	else 
		putreg(curline, curchar, mp->m_line, mp->m_char); 
	io = -1; 
} 
 
ProcSend() 
{ 
	extern char	ShcomBuf[]; 
	char	*com = ask(ShcomBuf, FuncName()); 
	Window	*procwind, 
		*oldwind = curwind; 
	char	combuf[100]; 
 
	if (isdead(cur_proc)) 
		return; 
	procwind = windlook(proc_buf(cur_proc)); 
	if (procwind == 0) { 
		pop_wind(proc_buf(cur_proc), 0, IPROCESS); 
		SetWind(oldwind); 
	} 
	proc_send(cur_proc, sprintf(combuf, "%s\n", com)); 
	ignore(proc_rec(cur_proc, combuf)); 
} 
 
static 
proc_send(p, buf) 
Process	*p; 
char	*buf; 
{ 
	int	n; 
 
	if (isdead(p)) 
		return; 
 
	n = strlen(buf); 
	if (write(p->p_toproc, buf, n) != n) 
		s_mess("Cannot write to process %s", proc_buf(p)); 
} 
 
IShell() 
{ 
	char	shell[30]; 
	int	number = 1; 
 
	do 
		ignore(sprintf(shell, "shell-%d", number++)); 
	while (proc_exists(shell)); 
 
	proc_strt(shell, "i-shell", "/bin/csh", "csh", "-i", 0); 
	SetWind(windlook(shell)); 
} 
 
Iprocess() 
{ 
	extern char	ShcomBuf[], 
			*MakeName(); 
	char	*command; 
 
	command = ask(ShcomBuf, FuncName()); 
	strcpy(ShcomBuf, command); 
	proc_strt(MakeName(command), command, "/bin/csh", "csh", "-c", command, 0); 
} 
 
proc_child() 
{ 
	union wait	w; 
	int	pid; 
 
#ifdef PROCS 
	sighold(SIGCHLD); 
#endif 
 
	for (;;) { 
#ifndef VMUNIX 
		pid = wait2(&w.w_status, (WNOHANG | WUNTRACED)); 
#else 
		pid = wait3(&w.w_status, (WNOHANG | WUNTRACED), 0); 
#endif 
		if (pid <= 0) 
			break; 
		kill_off(pid, w); 
	} 
 
#ifdef PROCS 
	sigrelse(SIGCHLD); 
#endif 
} 
 
kill_off(pid, w) 
int	pid; 
union wait	w; 
{ 
	Process	*child; 
	Buffer	*saveb = curbuf; 
 
	if ((child = proc_pid(pid)) == 0) 
		return; 
 
	if (WIFSTOPPED(w)) 
		child->p_state = STOPPED; 
	else { 
		child->p_state = DEAD; 
		if (WIFEXITED(w)) { 
			child->p_howdied = EXITED; 
			if (child->p_reason = w.w_retcode) { 
				SetBuf(child->p_buffer); 
				ins_str(pstate(child)); 
				SetBuf(saveb); 
			} 
		} else if (WIFSIGNALED(w)) { 
			child->p_reason = w.w_termsig; 
			child->p_howdied = KILLED; 
		} 
		s_mess("%s (in %s):    %s", 
					proc_cmd(child), 
					proc_buf(child), 
					pstate(child)); 
		redisplay();	/* In case we interrupted a getch() (We 
				   want this to happen right away.) */ 
		proc_close(child); 
	} 
} 
 
/* VARARGS2 */ 
 
static 
proc_strt(bufname, procname, cmd) 
char	*bufname, 
	*procname, 
	*cmd; 
{ 
	Window	*owind = curwind; 
	int	toproc[2], 
		pid; 
	Process	*newp; 
 
	pop_wind(bufname, 1, IPROCESS); 
 
	dopipe(toproc); 
 
	switch (pid = fork()) { 
	case -1: 
		pclose(toproc); 
		complain("Fork failed"); 
 
	case 0: 
	    { 
#ifdef PORTSRV 
	    	char	*args[25], 
	    		**cp, 
	    		foo[100]; 
	    	int	i; 
	    	char	*portserver = getenv("PORTSRV"); 
 
	    	if (! portserver) 
	    		portserver = PORTSRV; 
	    	args[0] = "portsrv"; 
	    	args[1] = sprintf(foo, "%d", ProcInput); 
	    	for (i = 0, cp = &cmd; cp[i] != 0; i++) 
	    		args[2 + i] = cp[i]; 
	    	args[2 + i] = 0; 
		dup2(toproc[0], 0); 
		dup2(ProcOutput, 1); 
		dup2(ProcOutput, 2); 
		pclose(toproc); 
		execv(portserver, args); 
		printf("Execl failed\n"); 
		_exit(1); 
#else 
	    	complain("No PORTSRV available..."); 
#endif 
	    } 
 
	default: 
#ifdef PROCS 
		sighold(SIGCHLD); 
#endif 
		cur_proc = newp = (Process *) emalloc(sizeof *newp); 
		newp->p_next = procs; 
		newp->p_state = NEW; 
		newp->p_mark = MakeMark(curline, curchar, !FLOATER); 
		SetMark(); 
 
		newp->p_func = (int (*)()) 0; 
		newp->p_name = copystr(procname); 
		procs = newp; 
		newp->p_portpid = pid; 
		newp->p_rpid = -1; 
		newp->p_buffer = curbuf; 
		newp->p_toproc = toproc[1]; 
		newp->p_reason = 0; 
		newp->p_eof = 0; 
		newp->p_nlines = 0; 
		NumProcs++; 
		ignore(close(toproc[0])); 
#ifdef PROCS 
		sigrelse(SIGCHLD); 
#endif 
	} 
	SetWind(owind); 
} 
 
pinit() 
{ 
	int	p[2]; 
 
	ignore(pipe(p)); 
	ProcInput = p[0]; 
	ProcOutput = p[1]; 
	ignorf(signal(INPUT_SIG, procs_read)); 
#ifdef PROCS 
#ifndef VENIX 
	sighold(INPUT_SIG);	/* Released during terminal read */ 
#endif 
#endif 
} 
 
