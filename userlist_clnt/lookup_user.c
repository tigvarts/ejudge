/* -*- mode: c -*- */

/* Copyright (C) 2006-2016 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "userlist_clnt/private.h"

int
userlist_clnt_lookup_user(
        struct userlist_clnt *clnt,
        unsigned char const *login,
        int contest_id,
        int *p_user_id,
        unsigned char **p_name)
{
  struct userlist_pk_do_login *out = 0;
  struct userlist_pk_login_ok *in = 0;
  unsigned char *login_ptr, *name_ptr;
  int r, login_len;
  size_t out_size = 0, in_size = 0;
  void *void_in = 0;

  if (!login || !*login) return -ULS_ERR_INVALID_LOGIN;
  if ((login_len = strlen(login)) > 255) return -ULS_ERR_INVALID_SIZE;
  out_size = sizeof(*out) + login_len;
  out = alloca(out_size);
  memset(out, 0, out_size);
  login_ptr = out->data;
  out->request_id = ULS_LOOKUP_USER;
  out->contest_id = contest_id;
  out->login_length = login_len;
  out->password_length = 0;
  strcpy(login_ptr, login);
  if ((r = userlist_clnt_send_packet(clnt, out_size, out)) < 0) return r;
  if ((r = userlist_clnt_read_and_notify(clnt, &in_size, &void_in)) < 0)
    return r;
  in = void_in;
  if (in->reply_id != ULS_LOGIN_OK && in->reply_id != ULS_LOGIN_COOKIE) {
    r = in->reply_id;
    goto cleanup;
  }
  if (in_size < sizeof(*in)) {
    r = -ULS_ERR_PROTOCOL;
    goto cleanup;
  }
  login_ptr = in->data;
  if (strlen(login_ptr) != in->login_len) {
    r = -ULS_ERR_PROTOCOL;
    goto cleanup;
  }
  name_ptr = login_ptr + in->login_len + 1;
  if (strlen(name_ptr) != in->name_len) {
    r = -ULS_ERR_PROTOCOL;
    goto cleanup;
  }
  if (in_size != sizeof(*in) + in->login_len + in->name_len) {
    r = -ULS_ERR_PROTOCOL;
    goto cleanup;
  }
  if (p_user_id) *p_user_id = in->user_id;
  if (p_name) *p_name = xstrdup(name_ptr);

  r = in->reply_id;
 cleanup:
  xfree(in);
  return r;
}
