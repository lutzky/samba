/*
   Unix SMB/CIFS implementation.
   async queryuser
   Copyright (C) Volker Lendecke 2009

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"
#include "winbindd.h"
#include "librpc/gen_ndr/cli_wbint.h"

struct wb_queryuser_state {
	struct dom_sid sid;
	struct wbint_userinfo info;
};

static void wb_queryuser_done(struct tevent_req *subreq);

struct tevent_req *wb_queryuser_send(TALLOC_CTX *mem_ctx,
				     struct tevent_context *ev,
				     const struct dom_sid *user_sid)
{
	struct tevent_req *req, *subreq;
	struct wb_queryuser_state *state;
	struct winbindd_domain *domain;
	struct winbind_userinfo info;
	NTSTATUS status;

	req = tevent_req_create(mem_ctx, &state, struct wb_queryuser_state);
	if (req == NULL) {
		return NULL;
	}
	sid_copy(&state->sid, user_sid);

	domain = find_domain_from_sid_noinit(user_sid);
	if (domain == NULL) {
		tevent_req_nterror(req, NT_STATUS_NO_SUCH_USER);
		return tevent_req_post(req, ev);
	}

	status = wcache_query_user(domain, state, &state->sid, &info);
	if (NT_STATUS_IS_OK(status)) {
		state->info.acct_name = info.acct_name;
		state->info.full_name = info.full_name;
		state->info.homedir = info.homedir;
		state->info.shell = info.shell;
		state->info.primary_gid = info.primary_gid;
		sid_copy(&state->info.user_sid, &info.user_sid);
		sid_copy(&state->info.group_sid, &info.group_sid);
		tevent_req_done(req);
		return tevent_req_post(req, ev);
	}

	subreq = rpccli_wbint_QueryUser_send(state, ev, domain->child.rpccli,
					     &state->sid, &state->info);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, wb_queryuser_done, req);
	return req;
}

static void wb_queryuser_done(struct tevent_req *subreq)
{
	struct tevent_req *req = tevent_req_callback_data(
		subreq, struct tevent_req);
	struct wb_queryuser_state *state = tevent_req_data(
		req, struct wb_queryuser_state);
	NTSTATUS status, result;

	status = rpccli_wbint_QueryUser_recv(subreq, state, &result);
	TALLOC_FREE(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}
	if (!NT_STATUS_IS_OK(result)) {
		tevent_req_nterror(req, result);
		return;
	}
	tevent_req_done(req);
}

NTSTATUS wb_queryuser_recv(struct tevent_req *req, TALLOC_CTX *mem_ctx,
			   struct winbind_userinfo **pinfo)
{
	struct wb_queryuser_state *state = tevent_req_data(
		req, struct wb_queryuser_state);
	struct winbind_userinfo *info;
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		return status;
	}

	info = talloc(mem_ctx, struct winbind_userinfo);
	if (info == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
	info->acct_name = talloc_move(info, &state->info.acct_name);
	info->full_name = talloc_move(info, &state->info.full_name);
	info->homedir = talloc_move(info, &state->info.homedir);
	info->shell = talloc_move(info, &state->info.shell);
	info->primary_gid = state->info.primary_gid;
	sid_copy(&info->user_sid, &state->info.user_sid);
	sid_copy(&info->group_sid, &state->info.group_sid);
	*pinfo = info;
	return NT_STATUS_OK;
}