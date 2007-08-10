/* auth-pam.c -- PAM authentification routine for vlock,
 *               the VT locking program for linux
 *
 * This program is copyright (C) 2007 Frank Benkstein, and is free
 * software which is freely distributable under the terms of the
 * GNU General Public License version 2, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 *
 * The conversation functions (conversation, prompt, prompt_echo_off)
 * were inspired by/copied from openpam's openpam_ttyconv.c:
 *
 * Copyright (c) 2002-2003 Networks Associates Technology, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <security/pam_appl.h>

#include "vlock.h"

static int conversation(int num_msg, const struct pam_message **msg, struct
    pam_response **resp, void __attribute__((__unused__)) *appdata_ptr) {
  int i;
  struct pam_response *aresp;

  if (num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG)
    return PAM_CONV_ERR;

  if ((aresp = calloc(num_msg, sizeof *aresp)) == NULL)
    return PAM_BUF_ERR;

  for (i = 0; i < num_msg; i++) {
    switch (msg[i]->msg_style) {
      case PAM_PROMPT_ECHO_OFF:
        aresp[i].resp = prompt_echo_off(msg[i]->msg);
        if (aresp[i].resp == NULL)
          goto fail;
        break;
      case PAM_PROMPT_ECHO_ON:
        aresp[i].resp = prompt(msg[i]->msg);
        if (aresp[i].resp == NULL)
          goto fail;
        break;
      case PAM_TEXT_INFO:
      case PAM_ERROR_MSG:
        {
          int msg_len = strlen(msg[i]->msg);
          fputs(msg[i]->msg, stderr);
          if (msg_len > 0 && msg[i]->msg[msg_len - 1] != '\n')
            fputc('\n', stderr);
          break;
        }
        break;
      default:
        goto fail;
    }
  }

  *resp = aresp;
  return PAM_SUCCESS;

fail:
 for (i = 0; i < num_msg; ++i)
    if (aresp[i].resp != NULL) {
      memset(aresp[i].resp, 0, strlen(aresp[i].resp));
      free(aresp[i].resp);
    }

  memset(aresp, 0, num_msg * sizeof *aresp);
  free(aresp);
  *resp = NULL;

  return PAM_CONV_ERR;
}

int auth(const char *user) {
  pam_handle_t *pamh;
  int pam_status;
  int pam_end_status;
  struct pam_conv pamc = {
    conversation,
    NULL,
  };

  /* initialize pam */
  pam_status = pam_start("vlock", user, &pamc, &pamh);

  if (pam_status != PAM_SUCCESS) {
    fprintf(stderr, "vlock-auth: %s\n", pam_strerror(pamh, pam_status));
    goto end;
  }

  /* put the username before the password prompt */
  fprintf(stderr, "%s's ", user); fflush(stderr);
  /* authenticate the user */
  pam_status = pam_authenticate(pamh, 0);

  if (pam_status != PAM_SUCCESS) {
    fprintf(stderr, "vlock-auth: %s\n", pam_strerror(pamh, pam_status));
  }

end:
  /* finish pam */
  pam_end_status = pam_end(pamh, pam_status);

  if (pam_end_status != PAM_SUCCESS) {
    fprintf(stderr, "vlock-auth: %s\n", pam_strerror(pamh, pam_end_status));
  }

  return (pam_status == PAM_SUCCESS);
}
