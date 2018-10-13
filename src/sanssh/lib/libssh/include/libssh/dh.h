/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef DH_H_
#define DH_H_

#include "config.h"

#include "libssh/crypto.h"

int ssh_dh_generate_e(ssh_session session);
int ssh_dh_generate_f(ssh_session session);
int ssh_dh_generate_x(ssh_session session);
int ssh_dh_generate_y(ssh_session session);

int ssh_dh_init(void);
void ssh_dh_finalize(void);

ssh_string ssh_dh_get_e(ssh_session session);
ssh_string ssh_dh_get_f(ssh_session session);
int ssh_dh_import_f(ssh_session session,ssh_string f_string);
int ssh_dh_import_e(ssh_session session, ssh_string e_string);

int ssh_dh_import_pubkey_blob(ssh_session session, ssh_string pubkey_blob);
int ssh_dh_import_next_pubkey_blob(ssh_session session, ssh_string pubkey_blob);

int ssh_dh_build_k(ssh_session session);
int ssh_client_dh_init(ssh_session session);
int ssh_client_dh_reply(ssh_session session, ssh_buffer packet);

ssh_key ssh_dh_get_current_server_publickey(ssh_session session);
int ssh_dh_get_current_server_publickey_blob(ssh_session session,
                                             ssh_string *pubkey_blob);
ssh_key ssh_dh_get_next_server_publickey(ssh_session session);
int ssh_dh_get_next_server_publickey_blob(ssh_session session,
                                          ssh_string *pubkey_blob);

int ssh_make_sessionid(ssh_session session);
/* add data for the final cookie */
int ssh_hashbufin_add_cookie(ssh_session session, unsigned char *cookie);
int ssh_hashbufout_add_cookie(ssh_session session);
int ssh_generate_session_keys(ssh_session session);

#endif /* DH_H_ */
