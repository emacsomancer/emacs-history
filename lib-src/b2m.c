/*
 * b2m - a filter for Babyl -> Unix mail files
 *
 * usage:	b2m < babyl > mailbox
 *
 * I find this useful whenever I have to use a
 * system which - shock horror! - doesn't run
 * Gnu emacs. At least now I can read all my
 * Gnumacs Babyl format mail files!
 *
 * it's not much but it's free!
 *
 *   Ed Wilkinson
 *   E.Wilkinson@massey.ac.nz
 *   Mon Nov 7 15:54:06 PDT 1988
 */

/* Serious bug: This program uses `gets', which is intrinsically
   unreliable--long lines will cause crashes.
   Someone should fix this program not to use `gets'.  */
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#ifdef MSDOS
#include <fcntl.h>
#endif

#include <../src/config.h>

/* BSD's strings.h does not declare the type of strtok.  */
extern char *strtok ();

#ifndef TRUE
#define TRUE  (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

int header = FALSE, printing;
time_t ltoday;
char from[256], labels[256], data[256], *p, *today;

main (argc, argv)
     int argc;
     char **argv;
{
#ifdef MSDOS
  _fmode = O_BINARY;		/* all of files are treated as binary files */
  (stdout)->_flag &= ~_IOTEXT;
  (stdin)->_flag &= ~_IOTEXT;
#endif
  if (argc >= 2 && strcmp (argv[1], "--help") == 0)
    {
      fprintf (stderr, "Usage: %s <babylmailbox >unixmailbox\n", argv[0]);
      exit (0);
    }
  ltoday = time (0);
  today = ctime (&ltoday);

  /* BUG!  Must not use gets in a reliable program!  */
  if (gets (data))
    {
      if (strncmp (data, "BABYL OPTIONS:", 14))
	{
	  fprintf (stderr, "%s: not a Babyl mailfile!\n", argv[0]);
	  exit (-1);
	}
      else
	printing = FALSE;
    }
  else
    exit (-1);
  if (printing)
    puts (data);

  while (gets (data))
    {

#if 0
      /* What was this for?  Does somebody have something against blank
	 lines?  */
      if (!strcmp (data, ""))
	exit (0);
#endif

      if (!strcmp (data, "*** EOOH ***") && !printing)
	{
	  printing = header = TRUE;
	  printf ("From %s %s", argv[0], today);
	  continue;
	}

      if (!strcmp (data, "\037\f"))
	{
	  /* save labels */
	  gets (data);
	  p = strtok (data, " ,\r\n\t");
	  strcpy (labels, "X-Babyl-Labels: ");

	  while (p = strtok (NULL, " ,\r\n\t"))
	    {
	      strcat (labels, p);
	      strcat (labels, ", ");
	    }

	  labels[strlen (labels) - 2] = '\0';
	  printing = header = FALSE;
	  continue;
	}

      if (!strlen (data) && header)
	{
	  header = FALSE;
	  if (strcmp (labels, "X-Babyl-Labels"))
	    puts (labels);
	}
    
      if (printing)
	puts (data);
    }
}
