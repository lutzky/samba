/*
   Unix SMB/CIFS implementation.
   Core SMB2 server

   Copyright (C) Stefan Metzmacher 2009

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
#include "smbd/globals.h"
#include "../source4/libcli/smb2/smb2_constants.h"

static struct tevent_req *smbd_smb2_setinfo_send(TALLOC_CTX *mem_ctx,
						 struct tevent_context *ev,
						 struct smbd_smb2_request *smb2req,
						 uint8_t in_info_type,
						 uint8_t in_file_info_class,
						 DATA_BLOB in_input_buffer,
						 uint32_t in_additional_information,
						 uint64_t in_file_id_volatile);
static NTSTATUS smbd_smb2_setinfo_recv(struct tevent_req *req);

static void smbd_smb2_request_setinfo_done(struct tevent_req *subreq);
NTSTATUS smbd_smb2_request_process_setinfo(struct smbd_smb2_request *req)
{
	const uint8_t *inhdr;
	const uint8_t *inbody;
	int i = req->current_idx;
	size_t expected_body_size = 0x21;
	size_t body_size;
	uint8_t in_info_type;
	uint8_t in_file_info_class;
	uint16_t in_input_buffer_offset;
	uint32_t in_input_buffer_length;
	DATA_BLOB in_input_buffer;
	uint32_t in_additional_information;
	uint64_t in_file_id_persistent;
	uint64_t in_file_id_volatile;
	struct tevent_req *subreq;

	inhdr = (const uint8_t *)req->in.vector[i+0].iov_base;
	if (req->in.vector[i+1].iov_len != (expected_body_size & 0xFFFFFFFE)) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	inbody = (const uint8_t *)req->in.vector[i+1].iov_base;

	body_size = SVAL(inbody, 0x00);
	if (body_size != expected_body_size) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	in_info_type			= CVAL(inbody, 0x02);
	in_file_info_class		= CVAL(inbody, 0x03);
	in_input_buffer_length		= IVAL(inbody, 0x04);
	in_input_buffer_offset		= SVAL(inbody, 0x08);
	/* 0x0A 2 bytes reserved */
	in_additional_information	= IVAL(inbody, 0x0C);
	in_file_id_persistent		= BVAL(inbody, 0x10);
	in_file_id_volatile		= BVAL(inbody, 0x18);

	if (in_input_buffer_offset == 0 && in_input_buffer_length == 0) {
		/* This is ok */
	} else if (in_input_buffer_offset !=
		   (SMB2_HDR_BODY + (body_size & 0xFFFFFFFE))) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	if (in_input_buffer_length > req->in.vector[i+2].iov_len) {
		return smbd_smb2_request_error(req, NT_STATUS_INVALID_PARAMETER);
	}

	in_input_buffer.data = (uint8_t *)req->in.vector[i+2].iov_base;
	in_input_buffer.length = in_input_buffer_length;

	if (req->compat_chain_fsp) {
		/* skip check */
	} else if (in_file_id_persistent != 0) {
		return smbd_smb2_request_error(req, NT_STATUS_FILE_CLOSED);
	}

	subreq = smbd_smb2_setinfo_send(req,
					req->conn->smb2.event_ctx,
					req,
					in_info_type,
					in_file_info_class,
					in_input_buffer,
					in_additional_information,
					in_file_id_volatile);
	if (subreq == NULL) {
		return smbd_smb2_request_error(req, NT_STATUS_NO_MEMORY);
	}
	tevent_req_set_callback(subreq, smbd_smb2_request_setinfo_done, req);

	if (tevent_req_is_in_progress(subreq)) {
		return smbd_smb2_request_pending_queue(req);
	}

	return NT_STATUS_OK;
}

static void smbd_smb2_request_setinfo_done(struct tevent_req *subreq)
{
	struct smbd_smb2_request *req = tevent_req_callback_data(subreq,
					struct smbd_smb2_request);
	int i = req->current_idx;
	DATA_BLOB outbody;
	NTSTATUS status;
	NTSTATUS error; /* transport error */

	status = smbd_smb2_setinfo_recv(subreq);
	TALLOC_FREE(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		error = smbd_smb2_request_error(req, status);
		if (!NT_STATUS_IS_OK(error)) {
			smbd_server_connection_terminate(req->conn,
							 nt_errstr(error));
			return;
		}
		return;
	}

	outbody = data_blob_talloc(req->out.vector, NULL, 0x02);
	if (outbody.data == NULL) {
		error = smbd_smb2_request_error(req, NT_STATUS_NO_MEMORY);
		if (!NT_STATUS_IS_OK(error)) {
			smbd_server_connection_terminate(req->conn,
							 nt_errstr(error));
			return;
		}
		return;
	}

	SSVAL(outbody.data, 0x00, 0x02);	/* struct size */

	error = smbd_smb2_request_done(req, outbody, NULL);
	if (!NT_STATUS_IS_OK(error)) {
		smbd_server_connection_terminate(req->conn,
						 nt_errstr(error));
		return;
	}
}

struct smbd_smb2_setinfo_state {
	struct smbd_smb2_request *smb2req;
};

static struct tevent_req *smbd_smb2_setinfo_send(TALLOC_CTX *mem_ctx,
						 struct tevent_context *ev,
						 struct smbd_smb2_request *smb2req,
						 uint8_t in_info_type,
						 uint8_t in_file_info_class,
						 DATA_BLOB in_input_buffer,
						 uint32_t in_additional_information,
						 uint64_t in_file_id_volatile)
{
	struct tevent_req *req;
	struct smbd_smb2_setinfo_state *state;
	struct smb_request *smbreq;
	connection_struct *conn = smb2req->tcon->compat_conn;
	files_struct *fsp;

	req = tevent_req_create(mem_ctx, &state,
				struct smbd_smb2_setinfo_state);
	if (req == NULL) {
		return NULL;
	}
	state->smb2req = smb2req;

	DEBUG(10,("smbd_smb2_setinfo_send: file_id[0x%016llX]\n",
		  (unsigned long long)in_file_id_volatile));

	smbreq = smbd_smb2_fake_smb_request(smb2req);
	if (tevent_req_nomem(smbreq, req)) {
		return tevent_req_post(req, ev);
	}

	fsp = file_fsp(smbreq, (uint16_t)in_file_id_volatile);
	if (fsp == NULL) {
		tevent_req_nterror(req, NT_STATUS_FILE_CLOSED);
		return tevent_req_post(req, ev);
	}
	if (conn != fsp->conn) {
		tevent_req_nterror(req, NT_STATUS_FILE_CLOSED);
		return tevent_req_post(req, ev);
	}
	if (smb2req->session->vuid != fsp->vuid) {
		tevent_req_nterror(req, NT_STATUS_FILE_CLOSED);
		return tevent_req_post(req, ev);
	}

	tevent_req_nterror(req, NT_STATUS_NOT_IMPLEMENTED);
	return tevent_req_post(req, ev);
}

static NTSTATUS smbd_smb2_setinfo_recv(struct tevent_req *req)
{
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	tevent_req_received(req);
	return NT_STATUS_OK;
}