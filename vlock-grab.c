/* vlock-grab.c -- console grabbing routine for vlock,
 *                 the VT locking program for linux
 *
 * This program is copyright (C) 2007 Frank Benkstein, and is free
 * software which is freely distributable under the terms of the
 * GNU General Public License version 2, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/vt.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "vlock.h"

/* Grab a the current and run the program given by argv+1.  Console switching
 * is forbidden as long as the program is running.
 *
 * CAP_SYS_TTY_CONFIG is needed for the locking to succeed.
 */
int main(int argc, char **argv) {
  int consfd = -1;
  struct vt_stat vtstat;
  int pid;
  int status;

  if (argc < 2) {
    fprintf(stderr, "usage: %s child\n", *argv);
    exit (111);
  }

  /* XXX: add optional PAM check here */

  /* open the virtual console directly */
  if ((consfd = open(CONSOLE, O_RDWR)) < 0) {
    perror("vlock-grab: cannot open virtual console");
    exit (1);
  }

  /* get the virtual console status */
  if (ioctl(consfd, VT_GETSTATE, &vtstat) < 0) {
    perror("vlock-grab: virtual console is not a virtual console");
    exit (1);
  }

  /* globally disable virtual console switching */
  if (ioctl(consfd, VT_LOCKSWITCH) < 0) {
    perror("vlock-grab: could not disable console switching");
    exit (1);
  }

  pid = fork();

  if (pid == 0) {
    /* child */

    /* drop privleges */
    setuid(getuid());

    /* run child */
    execvp(*(argv+1), argv+1);
    perror("vlock-grab: exec failed");
    _exit(127);
  } else if (pid < 0) {
    perror("vlock-grab: could not create child process");
  }

  if (pid > 0 && waitpid(pid, &status, 0) < 0) {
    perror("vlock-grab: child process missing");
    pid = -1;
  }

  /* globally enable virtual console switching */
  if (ioctl(consfd, VT_UNLOCKSWITCH) < 0) {
    perror("vlock-grab: could not enable console switching");
    exit (1);
  }

  /* exit with the exit status of the child or 200+signal if
   * it was killed */
  if (pid > 0) {
    if (WIFEXITED(status)) {
      exit (WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      exit (200+WTERMSIG(status));
    }
  }

  return 0;
}
