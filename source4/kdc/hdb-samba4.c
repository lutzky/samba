/*
 * Copyright (c) 1999-2001, 2003, PADL Software Pty Ltd.
 * Copyright (c) 2004-2009, Andrew Bartlett <abartlet@samba.org>.
 * Copyright (c) 2004, Stefan Metzmacher <metze@samba.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of PADL Software  nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PADL SOFTWARE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL PADL SOFTWARE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "includes.h"
#include "auth/auth.h"
#include "auth/credentials/credentials.h"
#include "dsdb/samdb/samdb.h"
#include "param/param.h"
#include "kdc/kdc.h"
#include "kdc/db-glue.h"

static krb5_error_code hdb_samba4_open(krb5_context context, HDB *db, int flags, mode_t mode)
{
	if (db->hdb_master_key_set) {
		krb5_error_code ret = HDB_ERR_NOENTRY;
		krb5_warnx(context, "hdb_samba4_open: use of a master key incompatible with LDB\n");
		krb5_set_error_message(context, ret, "hdb_samba4_open: use of a master key incompatible with LDB\n");
		return ret;
	}

	return 0;
}

static krb5_error_code hdb_samba4_close(krb5_context context, HDB *db)
{
	return 0;
}

static krb5_error_code hdb_samba4_lock(krb5_context context, HDB *db, int operation)
{
	return 0;
}

static krb5_error_code hdb_samba4_unlock(krb5_context context, HDB *db)
{
	return 0;
}

static krb5_error_code hdb_samba4_rename(krb5_context context, HDB *db, const char *new_name)
{
	return HDB_ERR_DB_INUSE;
}

static krb5_error_code hdb_samba4_store(krb5_context context, HDB *db, unsigned flags, hdb_entry_ex *entry)
{
	return HDB_ERR_DB_INUSE;
}

static krb5_error_code hdb_samba4_remove(krb5_context context, HDB *db, krb5_const_principal principal)
{
	return HDB_ERR_DB_INUSE;
}

static krb5_error_code hdb_samba4_fetch(krb5_context context, HDB *db,
				 krb5_const_principal principal,
				 unsigned flags,
				 hdb_entry_ex *entry_ex)
{
	struct samba_kdc_db_context *kdc_db_ctx;

	kdc_db_ctx = talloc_get_type_abort(db->hdb_db,
					   struct samba_kdc_db_context);

	return samba_kdc_fetch(context, kdc_db_ctx, principal, flags, entry_ex);
}

static krb5_error_code hdb_samba4_firstkey(krb5_context context, HDB *db, unsigned flags,
					hdb_entry_ex *entry)
{
	struct samba_kdc_db_context *kdc_db_ctx;

	kdc_db_ctx = talloc_get_type_abort(db->hdb_db,
					   struct samba_kdc_db_context);

	return samba_kdc_firstkey(context, kdc_db_ctx, entry);
}

static krb5_error_code hdb_samba4_nextkey(krb5_context context, HDB *db, unsigned flags,
				   hdb_entry_ex *entry)
{
	struct samba_kdc_db_context *kdc_db_ctx;

	kdc_db_ctx = talloc_get_type_abort(db->hdb_db,
					   struct samba_kdc_db_context);

	return samba_kdc_nextkey(context, kdc_db_ctx, entry);
}

static krb5_error_code hdb_samba4_destroy(krb5_context context, HDB *db)
{
	struct samba_kdc_db_context *kdc_db_ctx;

	kdc_db_ctx = talloc_get_type_abort(db->hdb_db,
					   struct samba_kdc_db_context);

	if (kdc_db_ctx) {
		talloc_free(kdc_db_ctx->samdb);
		kdc_db_ctx->samdb = NULL;
	}

	talloc_free(db);
	return 0;
}

static krb5_error_code
hdb_samba4_check_constrained_delegation(krb5_context context, HDB *db,
					hdb_entry_ex *entry,
					krb5_const_principal target_principal)
{
	struct samba_kdc_db_context *kdc_db_ctx;

	kdc_db_ctx = talloc_get_type_abort(db->hdb_db,
					   struct samba_kdc_db_context);

	return samba_kdc_check_constrained_delegation(context, kdc_db_ctx,
						      entry,
						      target_principal);
}

static krb5_error_code
hdb_samba4_check_pkinit_ms_upn_match(krb5_context context, HDB *db,
				     hdb_entry_ex *entry,
				     krb5_const_principal certificate_principal)
{
	struct samba_kdc_db_context *kdc_db_ctx;

	kdc_db_ctx = talloc_get_type_abort(db->hdb_db,
					   struct samba_kdc_db_context);

	return samba_kdc_check_pkinit_ms_upn_match(context, kdc_db_ctx,
						   entry,
						   certificate_principal);
}

/* This interface is to be called by the KDC and libnet_keytab_dump,
 * which is expecting Samba calling conventions.
 * It is also called by a wrapper (hdb_samba4_create) from the
 * kpasswdd -> krb5 -> keytab_hdb -> hdb code */

NTSTATUS hdb_samba4_create_kdc(struct samba_kdc_base_context *base_ctx,
			       krb5_context context, struct HDB **db)
{
	struct samba_kdc_db_context *kdc_db_ctx;
	struct auth_session_info *session_info;
	NTSTATUS nt_status;

	*db = talloc(base_ctx, HDB);
	if (!*db) {
		krb5_set_error_message(context, ENOMEM, "malloc: out of memory");
		return NT_STATUS_NO_MEMORY;
	}

	(*db)->hdb_master_key_set = 0;
	(*db)->hdb_db = NULL;
	(*db)->hdb_capability_flags = 0;

#if 1
	/* we would prefer to use system_session(), as that would
	 * allow us to share the samdb backend context with other parts of the
	 * system. For now we can't as we need to override the
	 * credentials to set CRED_DONT_USE_KERBEROS, which would
	 * break other users of the system_session */
	DEBUG(0,("FIXME: Using new system session for hdb\n"));
	nt_status = auth_system_session_info(*db, base_ctx->lp_ctx, &session_info);
	if (!NT_STATUS_IS_OK(nt_status)) {
	       return nt_status;
	}
#else
	session_info = system_session(kdc_db_ctx->lp_ctx);
	if (session_info == NULL) {
		return NT_STATUS_INTERNAL_ERROR;
	}
#endif

	/* The idea here is very simple.  Using Kerberos to
	 * authenticate the KDC to the LDAP server is higly likely to
	 * be circular.
	 *
	 * In future we may set this up to use EXERNAL and SSL
	 * certificates, for now it will almost certainly be NTLMSSP_SET_USERNAME
	*/

	cli_credentials_set_kerberos_state(session_info->credentials,
					   CRED_DONT_USE_KERBEROS);

	kdc_db_ctx = talloc_zero(*db, struct samba_kdc_db_context);
	if (kdc_db_ctx == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
	kdc_db_ctx->ev_ctx = base_ctx->ev_ctx;
	kdc_db_ctx->lp_ctx = base_ctx->lp_ctx;
	kdc_db_ctx->ic_ctx = lp_iconv_convenience(base_ctx->lp_ctx);

	/* Setup the link to LDB */
	kdc_db_ctx->samdb = samdb_connect(kdc_db_ctx, base_ctx->ev_ctx,
					  base_ctx->lp_ctx, session_info);
	if (kdc_db_ctx->samdb == NULL) {
		DEBUG(1, ("hdb_samba4_create: Cannot open samdb for KDC backend!"));
		return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
	}

	(*db)->hdb_db = kdc_db_ctx;

	(*db)->hdb_dbc = NULL;
	(*db)->hdb_open = hdb_samba4_open;
	(*db)->hdb_close = hdb_samba4_close;
	(*db)->hdb_fetch = hdb_samba4_fetch;
	(*db)->hdb_store = hdb_samba4_store;
	(*db)->hdb_remove = hdb_samba4_remove;
	(*db)->hdb_firstkey = hdb_samba4_firstkey;
	(*db)->hdb_nextkey = hdb_samba4_nextkey;
	(*db)->hdb_lock = hdb_samba4_lock;
	(*db)->hdb_unlock = hdb_samba4_unlock;
	(*db)->hdb_rename = hdb_samba4_rename;
	/* we don't implement these, as we are not a lockable database */
	(*db)->hdb__get = NULL;
	(*db)->hdb__put = NULL;
	/* kadmin should not be used for deletes - use other tools instead */
	(*db)->hdb__del = NULL;
	(*db)->hdb_destroy = hdb_samba4_destroy;

	(*db)->hdb_auth_status = NULL;
	(*db)->hdb_check_constrained_delegation = hdb_samba4_check_constrained_delegation;
	(*db)->hdb_check_pkinit_ms_upn_match = hdb_samba4_check_pkinit_ms_upn_match;

	return NT_STATUS_OK;
}

static krb5_error_code hdb_samba4_create(krb5_context context, struct HDB **db, const char *arg)
{
	NTSTATUS nt_status;
	void *ptr;
	struct samba_kdc_base_context *base_ctx;

	if (sscanf(arg, "&%p", &ptr) != 1) {
		return EINVAL;
	}
	base_ctx = talloc_get_type_abort(ptr, struct samba_kdc_base_context);
	/* The global kdc_mem_ctx and kdc_lp_ctx, Disgusting, ugly hack, but it means one less private hook */
	nt_status = hdb_samba4_create_kdc(base_ctx, context, db);

	if (NT_STATUS_IS_OK(nt_status)) {
		return 0;
	}
	return EINVAL;
}

/* Only used in the hdb-backed keytab code
 * for a keytab of 'samba4&<address>', to find
 * kpasswd's key in the main DB, and to
 * copy all the keys into a file (libnet_keytab_export)
 *
 * The <address> is the string form of a pointer to a talloced struct hdb_samba_context
 */
struct hdb_method hdb_samba4 = {
	.interface_version = HDB_INTERFACE_VERSION,
	.prefix = "samba4",
	.create = hdb_samba4_create
};
