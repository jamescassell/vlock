/* vlock.c -- main routine for vlock, the VT locking program for linux
 *
 * This program is copyright (C) 1994 Michael K. Johnson, and is free
 * software, which is freely distributable under the terms of the
 * GNU public license, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

/* RCS log:
 * $Log: vlock.c,v $
 * Revision 1.2  1994/03/13  17:28:56  johnsonm
 * Now using SIGUSR{1,2} correctly to announce VC switches.  Fixed a few
 * other minor bugs.
 *
 * Revision 1.1  1994/03/13  16:28:16  johnsonm
 * Initial revision
 *
 */


#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <termios.h>
#include <pwd.h>
#include <signal.h>
#include <sys/vt.h>
#include <sys/kd.h>
#include <linux/keyboard.h>
#include "vlock.h"
#include "version.h"


static char rcsid[] = "$Id: vlock.c,v 1.3 1994/03/15 18:27:33 johnsonm Exp $";

/* Option globals */
  /* This determines whether the default behavior is to lock only the */
  /* current VT or all of them.  0 means current, 1 means all. */
  int o_lock_all = 0;
  /* Pattern lists are not yet really supported, but we'll put in the */
  /* infrastructure for when they are. */
  Pattern *o_pattern_list = (Pattern *)0;

/* Other globals */
  struct vt_mode ovtm;
  struct termios oterm;
  int vfd;

int main(int argc, char **argv) {

  static struct option long_options[] = { /* For parsing long arguments */
    {"current", 0, &o_lock_all, 0},
    {"all", 0, &o_lock_all, 1},
    {"pattern", required_argument, 0, O_PATTERN},
    {"version", no_argument, 0, O_VERSION},
    {"help", no_argument, 0, O_HELP},
    {0, 0, 0, 0},
  };
  int option_index; /* Unused */
  int c;
  struct vt_mode vtm;

  /* First we parse all the command line arguments */
  while ((c = getopt_long(argc, argv, "acvhp:",
			  long_options, &option_index)) != -1) {
    switch(c) {
    case 'c':
      o_lock_all = 0;
      break;
    case 'a':
      o_lock_all = 1;
      break;
    case 'v':
    case O_VERSION:
      fprintf(stderr, VERSION);
      exit(0);
      break;
    case 'h':
    case O_HELP:
      print_help(0);
      break;
    case 'p':
    case O_PATTERN:
      fprintf(stderr, "The pattern list option is not yet supported.\n");
      break;
    case '?':
      print_help(1);
      break;
    }
  }

  /* Now we have parsed the options, and can get on with life */
  /* get the file descriptor (should check this...) */
  vfd = open("/dev/console", O_RDWR);
  /* First we will set process control of VC switching; if this fails, */
  /* then we know that we aren't on a VC, and will print a message and */
  /* exit.   If it doesn't fail, it gets the current VT status... */
  c = ioctl(vfd, VT_GETMODE, &vtm);
  if (c < 0) {
    fprintf(stderr, "This tty is not a VC (virtual console), and cannot be locked.\n\n");
    print_help(1);
  }

  /* Now set the signals so we can't be summarily executed or stopped, */
  /* and handle SIGUSR{1,2} and SIGCHLD */
  mask_signals();

  ovtm = vtm; /* Keep a copy around to restore at appropriate times */
  vtm.mode = VT_PROCESS;
  vtm.relsig = SIGUSR1; /* handled by release_vt() */
  vtm.acqsig = SIGUSR2; /* handled by acquire_vt() */
  ioctl(vfd, VT_SETMODE, &vtm);

  printf("Your TTY is now locked.  Please enter the password to unlock.\n");
  printf("%s's password:", getpwuid(getuid())->pw_name);

  set_terminal();

  /* I'd like to use getchar() here for testing now, but I can't; */
  /* signals aren't being handled until that system call returns. */
  get_password();

  /* Now, get_password will fork the child process and return, and    */
  /* signal_die() will get the SIGCHLD.  In the meantime, we need to  */
  /* make a way for all signals to get through as soon as they come.  */
  /* signal_die() will be responsible for exiting if a SIGCHLD is     */
  /* received from the password-getting child exiting.  That child is */
  /* responsible not to die until it gets a proper password.          */
  while(1)
    pause();

}

/*
 * Local Variables: ***
 * mode:C ***
 * eval:(turn-on-auto-fill) ***
 * End: ***
 */
