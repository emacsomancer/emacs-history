#ifndef VENIXV
#define JOVE4.2			/* bsd4.2 */ 
#endif 
 
#define DESCRIBE	"/usr/local/lib/jove/describe.com" 
#define FINDCOM		"/usr/local/lib/jove/findcom" 
#define JOVERC		"/usr/local/lib/jove/Joverc" 
 
#define TMPFILE		"/tmp/joveXXXXXX" 
 
/* WARNING: some keys are bound to these functions.  edit keymap.txt */ 
/* and maybe func_defs.c if you change any of this */ 
 
#define WIRED_TERMS		/* Include code for wired terminals */ 
#define CHDIR			/* Change directory command and absolute pathnames */ 
#define XFILE			/* File commands - rename, delete */ 
#define MACROS			/* named/bound macros */ 
#define CASEREG			/* case region up/low */ 
#define COMMENT			/* comment handling */ 
#define INCSEARCH		/* incremental search */ 
#define SPELL			/* run speller on buffer */ 
 
#ifndef VENIX 
#define PROCS			/* PROCS only works with job control */ 
#endif 
 
#ifdef PROCS			/* for i-shell type commands */ 
#	define PORTSRV	"/usr/local/lib/jove/portsrv" 
#endif procs 
 
#ifndef VENIX 
#define JOB_CONTROL		/* If you have job stopping */ 
#endif 
 
#ifdef JOB_CONTROL 
#	define MENLO_JCL 
#	define fork vfork 
#endif job_control 
 
#ifdef OPTIONS 
				/* Include code for mail sending or receiving */ 
#define SENDMAIL "/usr/lib/sendmail","/usr/lib/sendmail","-t","-oee","-oi","-om",0 
#ifdef SENDMAIL 
#	define MAILBUFFER "Send Mail" 
#endif sendmail 
#define LSRHS_KLUDGERY		/* Probably ONLY if you are at LS */ 
#define ID_CHAR			/* Include code to IDchar */ 
#endif options 
 
#ifdef vax 
#	define VMUNIX		/* Virtual Memory UNIX */ 
#else 
/*#	define VMUNIX		/* When VMUNIX but not vax */ 
#endif vax 
 
#ifndef VMUNIX 
	typedef	short	disk_line; 
#	ifndef BUFSIZ 
#		define BUFSIZ	512 
#	endif 
#else 
	typedef	int	disk_line; 
#	ifndef BUFSIZ 
#		define BUFSIZ	1024 
#	endif 
#endif vmunix 
 
#ifdef JOVE4.2 
#	define MAXFILLEN	1025 
#	define GLOBBER			/* directory match on patterns */ 
#	ifdef GLOBBER 
#		define DIRLIST		/* "list-files" command */ 
#		define FCOMPLETE	/* filename completion */ 
#	endif 
#	define INPUT_SIG	SIGIO 
#	define sigset(s,d) signal(s,d) 
#	define sigsys(s,d) signal(s,d) 
#	define sighold(s)  sigblock(1<<((s)-1)) 
#	define sigrelse(s) sigsetmask(sigblock(0) & ~(1<<((s)-1))) 
#	define sigpaws(s)  sigpause(sigblock(0) & ~(1<<((s)-1))) 
#	ifdef sun 
#		define wait2(s,f)  wait3(s,f,0) 
#	endif 
#else 
#	define MAXFILLEN	256 
#	define INPUT_SIG	SIGIO? 
#endif jove4.2 
 
#define DFLT_INTRC	CTL(]) 
#define DFLT_MODE	0664	/* File will be created with this mode */ 
#define DFLT_SHFLAGS	"-c"	/* "-fc" means don't read .cshrc */ 
#define DFLT_SH		"/bin/csh" 
 
#define AS_PREFIX	"#"	/* AutoSave file name prefix */ 
#define AS_SUFFIX	"$"	/* AutoSave file name suffix */ 
