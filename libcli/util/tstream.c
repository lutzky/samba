/*
 *  Unix SMB/CIFS implementation.
 *
 *  Copyright (C) Stefan Metzmacher 2009
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <tevent.h>
#include "system/filesys.h"
#include "system/network.h"
#include "../lib/tsocket/tsocket.h"
#include "../libcli/util/tstream.h"
#include "../lib/util/tevent_ntstatus.h"

struct tstream_read_pdu_blob_state {
	/* this structs are owned by the caller */
	struct {
		struct tevent_context *ev;
		struct tstream_context *stream;
		NTSTATUS (*full_fn)(void *private_data,
				    DATA_BLOB blob,
				    size_t *packet_size);
		void *full_private;
	} caller;

	DATA_BLOB pdu_blob;
	struct iovec tmp_vector;
};

static void tstream_read_pdu_blob_done(struct tevent_req *subreq);

struct tevent_req *tstream_read_pdu_blob_send(TALLOC_CTX *mem_ctx,
				struct tevent_context *ev,
				struct tstream_context *stream,
				size_t initial_read_size,
				NTSTATUS (*full_fn)(void *private_data,
						    DATA_BLOB blob,
						    size_t *packet_size),
				void *full_private)
{
	struct tevent_req *req;
	struct tstream_read_pdu_blob_state *state;
	struct tevent_req *subreq;
	uint8_t *buf;

	req = tevent_req_create(mem_ctx, &state,
				struct tstream_read_pdu_blob_state);
	if (!req) {
		return NULL;
	}

	state->caller.ev		= ev;
	state->caller.stream		= stream;
	state->caller.full_fn		= full_fn;
	state->caller.full_private	= full_private;

	if (initial_read_size == 0) {
		tevent_req_error(req, EINVAL);
		return tevent_req_post(req, ev);
	}

	buf = talloc_array(state, uint8_t, initial_read_size);
	if (tevent_req_nomem(buf, req)) {
		return tevent_req_post(req, ev);
	}
	state->pdu_blob.data = buf;
	state->pdu_blob.length = initial_read_size;

	state->tmp_vector.iov_base = buf;
	state->tmp_vector.iov_len = initial_read_size;

	subreq = tstream_readv_send(state, ev, stream, &state->tmp_vector, 1);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, tstream_read_pdu_blob_done, req);

	return req;
}

static void tstream_read_pdu_blob_done(struct tevent_req *subreq)
{
	struct tevent_req *req =
		tevent_req_callback_data(subreq,
		struct tevent_req);
	struct tstream_read_pdu_blob_state *state =
		tevent_req_data(req,
		struct tstream_read_pdu_blob_state);
	ssize_t ret;
	int sys_errno;
	size_t pdu_size;
	NTSTATUS status;
	uint8_t *buf;

	ret = tstream_readv_recv(subreq, &sys_errno);
	TALLOC_FREE(subreq);
	if (ret == -1) {
		status = map_nt_error_from_unix(sys_errno);
		tevent_req_nterror(req, status);
		return;
	}

	status = state->caller.full_fn(state->caller.full_private,
				       state->pdu_blob, &pdu_size);
	if (NT_STATUS_IS_OK(status)) {
		tevent_req_done(req);
		return;
	} else if (NT_STATUS_EQUAL(status, STATUS_MORE_ENTRIES)) {
		/* more to get */
	} else if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}

	buf = talloc_realloc(state, state->pdu_blob.data, uint8_t, pdu_size);
	if (tevent_req_nomem(buf, req)) {
		return;
	}
	state->pdu_blob.data = buf;
	state->pdu_blob.length = pdu_size;

	state->tmp_vector.iov_base = buf + state->tmp_vector.iov_len;
	state->tmp_vector.iov_len = pdu_size - state->tmp_vector.iov_len;

	subreq = tstream_readv_send(state,
				    state->caller.ev,
				    state->caller.stream,
				    &state->tmp_vector,
				    1);
	if (tevent_req_nomem(subreq, req)) {
		return;
	}
	tevent_req_set_callback(subreq, tstream_read_pdu_blob_done, req);
}

NTSTATUS tstream_read_pdu_blob_recv(struct tevent_req *req,
				    TALLOC_CTX *mem_ctx,
				    DATA_BLOB *pdu_blob)
{
	struct tstream_read_pdu_blob_state *state = tevent_req_data(req,
					struct tstream_read_pdu_blob_state);
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	*pdu_blob = state->pdu_blob;
	talloc_steal(mem_ctx, pdu_blob->data);

	tevent_req_received(req);
	return NT_STATUS_OK;
}

