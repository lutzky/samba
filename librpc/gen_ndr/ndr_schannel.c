/* parser auto-generated by pidl */

#include "includes.h"
#include "../librpc/gen_ndr/ndr_schannel.h"

#include "librpc/gen_ndr/ndr_netlogon.h"
#include "librpc/gen_ndr/ndr_nbt.h"
static enum ndr_err_code ndr_push_schannel_bind_3(struct ndr_push *ndr, int ndr_flags, const struct schannel_bind_3 *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_push_align(ndr, 4));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->domain));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->workstation));
			ndr->flags = _flags_save_string;
		}
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

static enum ndr_err_code ndr_pull_schannel_bind_3(struct ndr_pull *ndr, int ndr_flags, struct schannel_bind_3 *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_pull_align(ndr, 4));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->domain));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->workstation));
			ndr->flags = _flags_save_string;
		}
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ void ndr_print_schannel_bind_3(struct ndr_print *ndr, const char *name, const struct schannel_bind_3 *r)
{
	ndr_print_struct(ndr, name, "schannel_bind_3");
	ndr->depth++;
	ndr_print_string(ndr, "domain", r->domain);
	ndr_print_string(ndr, "workstation", r->workstation);
	ndr->depth--;
}

static enum ndr_err_code ndr_push_schannel_bind_23(struct ndr_push *ndr, int ndr_flags, const struct schannel_bind_23 *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_push_align(ndr, 4));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->domain));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->workstation));
			ndr->flags = _flags_save_string;
		}
		NDR_CHECK(ndr_push_nbt_string(ndr, NDR_SCALARS, r->dnsdomain));
		NDR_CHECK(ndr_push_nbt_string(ndr, NDR_SCALARS, r->dnsworkstation));
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

static enum ndr_err_code ndr_pull_schannel_bind_23(struct ndr_pull *ndr, int ndr_flags, struct schannel_bind_23 *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_pull_align(ndr, 4));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->domain));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->workstation));
			ndr->flags = _flags_save_string;
		}
		NDR_CHECK(ndr_pull_nbt_string(ndr, NDR_SCALARS, &r->dnsdomain));
		NDR_CHECK(ndr_pull_nbt_string(ndr, NDR_SCALARS, &r->dnsworkstation));
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ void ndr_print_schannel_bind_23(struct ndr_print *ndr, const char *name, const struct schannel_bind_23 *r)
{
	ndr_print_struct(ndr, name, "schannel_bind_23");
	ndr->depth++;
	ndr_print_string(ndr, "domain", r->domain);
	ndr_print_string(ndr, "workstation", r->workstation);
	ndr_print_nbt_string(ndr, "dnsdomain", r->dnsdomain);
	ndr_print_nbt_string(ndr, "dnsworkstation", r->dnsworkstation);
	ndr->depth--;
}

static enum ndr_err_code ndr_push_schannel_bind_info(struct ndr_push *ndr, int ndr_flags, const union schannel_bind_info *r)
{
	if (ndr_flags & NDR_SCALARS) {
		int level = ndr_push_get_switch_value(ndr, r);
		switch (level) {
			case 3: {
				NDR_CHECK(ndr_push_schannel_bind_3(ndr, NDR_SCALARS, &r->info3));
			break; }

			case 23: {
				NDR_CHECK(ndr_push_schannel_bind_23(ndr, NDR_SCALARS, &r->info23));
			break; }

			default:
				return ndr_push_error(ndr, NDR_ERR_BAD_SWITCH, "Bad switch value %u at %s", level, __location__);
		}
	}
	if (ndr_flags & NDR_BUFFERS) {
		int level = ndr_push_get_switch_value(ndr, r);
		switch (level) {
			case 3:
			break;

			case 23:
			break;

			default:
				return ndr_push_error(ndr, NDR_ERR_BAD_SWITCH, "Bad switch value %u at %s", level, __location__);
		}
	}
	return NDR_ERR_SUCCESS;
}

static enum ndr_err_code ndr_pull_schannel_bind_info(struct ndr_pull *ndr, int ndr_flags, union schannel_bind_info *r)
{
	int level;
	level = ndr_pull_get_switch_value(ndr, r);
	if (ndr_flags & NDR_SCALARS) {
		switch (level) {
			case 3: {
				NDR_CHECK(ndr_pull_schannel_bind_3(ndr, NDR_SCALARS, &r->info3));
			break; }

			case 23: {
				NDR_CHECK(ndr_pull_schannel_bind_23(ndr, NDR_SCALARS, &r->info23));
			break; }

			default:
				return ndr_pull_error(ndr, NDR_ERR_BAD_SWITCH, "Bad switch value %u at %s", level, __location__);
		}
	}
	if (ndr_flags & NDR_BUFFERS) {
		switch (level) {
			case 3:
			break;

			case 23:
			break;

			default:
				return ndr_pull_error(ndr, NDR_ERR_BAD_SWITCH, "Bad switch value %u at %s", level, __location__);
		}
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ void ndr_print_schannel_bind_info(struct ndr_print *ndr, const char *name, const union schannel_bind_info *r)
{
	int level;
	level = ndr_print_get_switch_value(ndr, r);
	ndr_print_union(ndr, name, level, "schannel_bind_info");
	switch (level) {
		case 3:
			ndr_print_schannel_bind_3(ndr, "info3", &r->info3);
		break;

		case 23:
			ndr_print_schannel_bind_23(ndr, "info23", &r->info23);
		break;

		default:
			ndr_print_bad_level(ndr, name, level);
	}
}

_PUBLIC_ enum ndr_err_code ndr_push_schannel_bind(struct ndr_push *ndr, int ndr_flags, const struct schannel_bind *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_push_align(ndr, 4));
		NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->unknown1));
		NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->bind_type));
		NDR_CHECK(ndr_push_set_switch_value(ndr, &r->u, r->bind_type));
		NDR_CHECK(ndr_push_schannel_bind_info(ndr, NDR_SCALARS, &r->u));
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ enum ndr_err_code ndr_pull_schannel_bind(struct ndr_pull *ndr, int ndr_flags, struct schannel_bind *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_pull_align(ndr, 4));
		NDR_CHECK(ndr_pull_uint32(ndr, NDR_SCALARS, &r->unknown1));
		NDR_CHECK(ndr_pull_uint32(ndr, NDR_SCALARS, &r->bind_type));
		NDR_CHECK(ndr_pull_set_switch_value(ndr, &r->u, r->bind_type));
		NDR_CHECK(ndr_pull_schannel_bind_info(ndr, NDR_SCALARS, &r->u));
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ void ndr_print_schannel_bind(struct ndr_print *ndr, const char *name, const struct schannel_bind *r)
{
	ndr_print_struct(ndr, name, "schannel_bind");
	ndr->depth++;
	ndr_print_uint32(ndr, "unknown1", r->unknown1);
	ndr_print_uint32(ndr, "bind_type", r->bind_type);
	ndr_print_set_switch_value(ndr, &r->u, r->bind_type);
	ndr_print_schannel_bind_info(ndr, "u", &r->u);
	ndr->depth--;
}

_PUBLIC_ enum ndr_err_code ndr_push_schannel_bind_ack(struct ndr_push *ndr, int ndr_flags, const struct schannel_bind_ack *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_push_align(ndr, 4));
		NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->unknown1));
		NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->unknown2));
		NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->unknown3));
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ enum ndr_err_code ndr_pull_schannel_bind_ack(struct ndr_pull *ndr, int ndr_flags, struct schannel_bind_ack *r)
{
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_pull_align(ndr, 4));
		NDR_CHECK(ndr_pull_uint32(ndr, NDR_SCALARS, &r->unknown1));
		NDR_CHECK(ndr_pull_uint32(ndr, NDR_SCALARS, &r->unknown2));
		NDR_CHECK(ndr_pull_uint32(ndr, NDR_SCALARS, &r->unknown3));
	}
	if (ndr_flags & NDR_BUFFERS) {
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ void ndr_print_schannel_bind_ack(struct ndr_print *ndr, const char *name, const struct schannel_bind_ack *r)
{
	ndr_print_struct(ndr, name, "schannel_bind_ack");
	ndr->depth++;
	ndr_print_uint32(ndr, "unknown1", r->unknown1);
	ndr_print_uint32(ndr, "unknown2", r->unknown2);
	ndr_print_uint32(ndr, "unknown3", r->unknown3);
	ndr->depth--;
}
