/* 
   Unix SMB/CIFS mplementation.

   The module that handles the Schema checkings and dynamic attributes
   
   Copyright (C) Stefan Metzmacher <metze@samba.org> 2007
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2009
    
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
#include "ldb_module.h"
#include "dsdb/samdb/samdb.h"
#include "librpc/gen_ndr/ndr_misc.h"
#include "librpc/gen_ndr/ndr_drsuapi.h"
#include "librpc/gen_ndr/ndr_drsblobs.h"
#include "param/param.h"
#include "dsdb/samdb/ldb_modules/util.h"

static int generate_objectClasses(struct ldb_context *ldb, struct ldb_message *msg,
				  const struct dsdb_schema *schema);
static int generate_attributeTypes(struct ldb_context *ldb, struct ldb_message *msg,
				   const struct dsdb_schema *schema);
static int generate_dITContentRules(struct ldb_context *ldb, struct ldb_message *msg,
				    const struct dsdb_schema *schema);
static int generate_extendedAttributeInfo(struct ldb_context *ldb, struct ldb_message *msg,
					  const struct dsdb_schema *schema);
static int generate_extendedClassInfo(struct ldb_context *ldb, struct ldb_message *msg,
				      const struct dsdb_schema *schema);
static int generate_possibleInferiors(struct ldb_context *ldb, struct ldb_message *msg,
				      const struct dsdb_schema *schema);

static const struct {
	const char *attr;
	int (*fn)(struct ldb_context *, struct ldb_message *, const struct dsdb_schema *);
	bool aggregate;
} generated_attrs[] = {
	{
		.attr = "objectClasses",
		.fn = generate_objectClasses,
		.aggregate = true,
	},
	{
		.attr = "attributeTypes",
		.fn = generate_attributeTypes,
		.aggregate = true,
	},
	{
		.attr = "dITContentRules",
		.fn = generate_dITContentRules,
		.aggregate = true,
	},
	{
		.attr = "extendedAttributeInfo",
		.fn = generate_extendedAttributeInfo,
		.aggregate = true,
	},
	{
		.attr = "extendedClassInfo",
		.fn = generate_extendedClassInfo,
		.aggregate = true,
	},
	{
		.attr = "possibleInferiors",
		.fn = generate_possibleInferiors,
		.aggregate = false,
	}
};

struct schema_data_private_data {
	struct ldb_dn *aggregate_dn;
	struct ldb_dn *schema_dn;
};

struct schema_data_search_data {
	struct ldb_module *module;
	struct ldb_request *req;

	const struct dsdb_schema *schema;
};

/* context to be used during async operations */
struct schema_data_context {
	struct ldb_module *module;
	struct ldb_request *req;

	const struct dsdb_schema *schema;
};

/* Create new context using
 * ldb_request as memory context */
static int _schema_data_context_new(struct ldb_module *module,
				    struct ldb_request *req,
				    struct schema_data_context **pac)
{
	struct schema_data_context *ac;
	struct ldb_context *ldb;

	ldb = ldb_module_get_ctx(module);

	*pac = ac = talloc_zero(req, struct schema_data_context);
	if (ac == NULL) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}
	ac->module = module;
	ac->req = req;
	ac->schema = dsdb_get_schema(ldb);

	return LDB_SUCCESS;
}

static int schema_data_init(struct ldb_module *module)
{
	struct ldb_context *ldb;
	struct ldb_dn *schema_dn;
	int ret;
	struct schema_data_private_data *data;

	ret = ldb_next_init(module);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	ldb = ldb_module_get_ctx(module);
	schema_dn = samdb_schema_dn(ldb);
	if (!schema_dn) {
		ldb_reset_err_string(ldb);
		ldb_debug(ldb, LDB_DEBUG_WARNING,
			  "schema_data_init: no schema dn present: (skip schema loading)\n");
		return LDB_SUCCESS;
	}

	data = talloc(module, struct schema_data_private_data);
	if (data == NULL) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	data->schema_dn = schema_dn;

	/* Used to check to see if this is a result on the CN=Aggregate schema */
	data->aggregate_dn = samdb_aggregate_schema_dn(ldb, data);
	if (!data->aggregate_dn) {
		ldb_set_errstring(ldb, "Could not build aggregate schema DN");
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ldb_module_set_private(module, data);
	return LDB_SUCCESS;
}


/* Generate new value for msDs-IntId
 * Value should be in 0x80000000..0xBFFFFFFF range
 * Generated value is added ldb_msg */
static int _schema_data_gen_msds_intid(struct schema_data_context *ac,
				       struct ldb_message *ldb_msg)
{
	uint32_t id;

	/* generate random num in 0x80000000..0xBFFFFFFF */
	id = generate_random() % 0X3FFFFFFF;
	id += 0x80000000;

	/* make sure id is unique and adjust if not */
	while (dsdb_attribute_by_attributeID_id(ac->schema, id)) {
		id++;
		if (id > 0xBFFFFFFF) {
			id = 0x80000001;
		}
	}

	/* add generated msDS-IntId value to ldb_msg */
	return ldb_msg_add_fmt(ldb_msg, "msDS-IntId", "%d", id);
}

static int _schema_data_add_callback(struct ldb_request *req,
				     struct ldb_reply *ares)
{
	struct schema_data_context *ac;

	ac = talloc_get_type(req->context, struct schema_data_context);

	if (!ares) {
		return ldb_module_done(ac->req, NULL, NULL,
					LDB_ERR_OPERATIONS_ERROR);
	}
	if (ares->error != LDB_SUCCESS) {
		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}

	if (ares->type != LDB_REPLY_DONE) {
		talloc_free(ares);
		return ldb_module_done(ac->req, NULL, NULL,
					LDB_ERR_OPERATIONS_ERROR);
	}

	return ldb_module_done(ac->req, ares->controls,
				ares->response, ares->error);
}

static int schema_data_add(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb;
	struct dsdb_schema *schema;
	const struct ldb_val *attributeID = NULL;
	const struct ldb_val *governsID = NULL;
	const char *oid_attr = NULL;
	const char *oid = NULL;
	WERROR status;

	ldb = ldb_module_get_ctx(module);

	/* special objects should always go through */
	if (ldb_dn_is_special(req->op.add.message->dn)) {
		return ldb_next_request(module, req);
	}

	/* replicated update should always go through */
	if (ldb_request_get_control(req, DSDB_CONTROL_REPLICATED_UPDATE_OID)) {
		return ldb_next_request(module, req);
	}

	schema = dsdb_get_schema(ldb);
	if (!schema) {
		return ldb_next_request(module, req);
	}

	if (!schema->fsmo.we_are_master) {
		ldb_debug_set(ldb, LDB_DEBUG_ERROR,
			  "schema_data_add: we are not master: reject request\n");
		return LDB_ERR_UNWILLING_TO_PERFORM;
	}

	attributeID = ldb_msg_find_ldb_val(req->op.add.message, "attributeID");
	governsID = ldb_msg_find_ldb_val(req->op.add.message, "governsID");

	if (attributeID) {
		/* Sanity check for not allowed attributes */
		if (ldb_msg_find_ldb_val(req->op.add.message, "msDS-IntId")) {
			return LDB_ERR_UNWILLING_TO_PERFORM;
		}

		oid_attr = "attributeID";
		oid = talloc_strndup(req, (const char *)attributeID->data, attributeID->length);
	} else if (governsID) {
		oid_attr = "governsID";
		oid = talloc_strndup(req, (const char *)governsID->data, governsID->length);
	} else {
		return ldb_next_request(module, req);
	}

	if (!oid) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	status = dsdb_schema_pfm_find_oid(schema->prefixmap, oid, NULL);
	if (!W_ERROR_IS_OK(status)) {
		/* check for internal errors */
		if (!W_ERROR_EQUAL(WERR_DS_NO_MSDS_INTID, status)) {
			ldb_debug_set(ldb, LDB_DEBUG_ERROR,
			              "schema_data_add: failed to map %s[%s]: %s\n",
			              oid_attr, oid, win_errstr(status));
			return LDB_ERR_UNWILLING_TO_PERFORM;
		}

		/* Update prefixMap and save it */
		status = dsdb_create_prefix_mapping(ldb, schema, oid);
		if (!W_ERROR_IS_OK(status)) {
			ldb_debug_set(ldb, LDB_DEBUG_ERROR,
				  "schema_data_add: failed to create prefix mapping for %s[%s]: %s\n",
				  oid_attr, oid, win_errstr(status));
			return LDB_ERR_UNWILLING_TO_PERFORM;
		}
	}

	/* bypass further processing if CONTROL_RELAX is set */
	if (ldb_request_get_control(req, LDB_CONTROL_RELAX_OID)) {
		return ldb_next_request(module, req);
	}

	/* generate and add msDS-IntId attr value */
	if (attributeID
	    && (dsdb_functional_level(ldb) >= DS_DOMAIN_FUNCTION_2003)
	    && !(ldb_msg_find_attr_as_uint(req->op.add.message, "systemFlags", 0) & SYSTEM_FLAG_SCHEMA_BASE_OBJECT)) {
		struct ldb_message *msg;
		struct schema_data_context *ac;
		struct ldb_request *add_req;

		if (_schema_data_context_new(module, req, &ac) != LDB_SUCCESS) {
			return LDB_ERR_OPERATIONS_ERROR;
		}

		/* we have to copy the message as the caller might have it as a const */
		msg = ldb_msg_copy_shallow(ac, req->op.add.message);
		if (msg == NULL) {
			ldb_oom(ldb);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		/* generate unique value for msDS-IntId attr value */
		if (_schema_data_gen_msds_intid(ac, msg) != LDB_SUCCESS) {
			ldb_debug_set(ldb, LDB_DEBUG_ERROR,
			              "_schema_data_gen_msds_intid() failed to generate msDS-IntId value\n");
			return LDB_ERR_OPERATIONS_ERROR;
		}

		ldb_build_add_req(&add_req, ldb, ac,
				  msg,
				  req->controls,
				  ac, _schema_data_add_callback,
				  req);

		return ldb_next_request(module, add_req);
	}

	return ldb_next_request(module, req);
}

static int schema_data_modify(struct ldb_module *module, struct ldb_request *req)
{
	/* special objects should always go through */
	if (ldb_dn_is_special(req->op.mod.message->dn)) {
		return ldb_next_request(module, req);
	}

	/* replicated update should always go through */
	if (ldb_request_get_control(req, DSDB_CONTROL_REPLICATED_UPDATE_OID)) {
		return ldb_next_request(module, req);
	}

	/* msDS-IntId is not allowed to be modified */
	if (ldb_msg_find_ldb_val(req->op.mod.message, "msDS-IntId")) {
		return LDB_ERR_CONSTRAINT_VIOLATION;
	}

	/* go on with the call chain */
	return ldb_next_request(module, req);
}

static int generate_objectClasses(struct ldb_context *ldb, struct ldb_message *msg,
				  const struct dsdb_schema *schema) 
{
	const struct dsdb_class *sclass;
	int ret;

	for (sclass = schema->classes; sclass; sclass = sclass->next) {
		ret = ldb_msg_add_string(msg, "objectClasses", schema_class_to_description(msg, sclass));
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}
	return LDB_SUCCESS;
}
static int generate_attributeTypes(struct ldb_context *ldb, struct ldb_message *msg,
				  const struct dsdb_schema *schema) 
{
	const struct dsdb_attribute *attribute;
	int ret;
	
	for (attribute = schema->attributes; attribute; attribute = attribute->next) {
		ret = ldb_msg_add_string(msg, "attributeTypes", schema_attribute_to_description(msg, attribute));
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}
	return LDB_SUCCESS;
}

static int generate_dITContentRules(struct ldb_context *ldb, struct ldb_message *msg,
				    const struct dsdb_schema *schema) 
{
	const struct dsdb_class *sclass;
	int ret;

	for (sclass = schema->classes; sclass; sclass = sclass->next) {
		if (sclass->auxiliaryClass || sclass->systemAuxiliaryClass) {
			char *ditcontentrule = schema_class_to_dITContentRule(msg, sclass, schema);
			if (!ditcontentrule) {
				ldb_oom(ldb);
				return LDB_ERR_OPERATIONS_ERROR;
			}
			ret = ldb_msg_add_steal_string(msg, "dITContentRules", ditcontentrule);
			if (ret != LDB_SUCCESS) {
				return ret;
			}
		}
	}
	return LDB_SUCCESS;
}

static int generate_extendedAttributeInfo(struct ldb_context *ldb,
					  struct ldb_message *msg,
					  const struct dsdb_schema *schema)
{
	const struct dsdb_attribute *attribute;
	int ret;

	for (attribute = schema->attributes; attribute; attribute = attribute->next) {
		char *val = schema_attribute_to_extendedInfo(msg, attribute);
		if (!val) {
			ldb_oom(ldb);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		ret = ldb_msg_add_string(msg, "extendedAttributeInfo", val);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	return LDB_SUCCESS;
}

static int generate_extendedClassInfo(struct ldb_context *ldb,
				      struct ldb_message *msg,
				      const struct dsdb_schema *schema)
{
	const struct dsdb_class *sclass;
	int ret;

	for (sclass = schema->classes; sclass; sclass = sclass->next) {
		char *val = schema_class_to_extendedInfo(msg, sclass);
		if (!val) {
			ldb_oom(ldb);
			return LDB_ERR_OPERATIONS_ERROR;
		}

		ret = ldb_msg_add_string(msg, "extendedClassInfo", val);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	return LDB_SUCCESS;
}


static int generate_possibleInferiors(struct ldb_context *ldb, struct ldb_message *msg,
				      const struct dsdb_schema *schema) 
{
	struct ldb_dn *dn = msg->dn;
	int ret, i;
	const char *first_component_name = ldb_dn_get_component_name(dn, 0);
	const struct ldb_val *first_component_val;
	const struct dsdb_class *schema_class;
	const char **possibleInferiors;

	if (strcasecmp(first_component_name, "cn") != 0) {
		return LDB_SUCCESS;
	}

	first_component_val = ldb_dn_get_component_val(dn, 0);

	schema_class = dsdb_class_by_cn_ldb_val(schema, first_component_val);
	if (schema_class == NULL) {
		return LDB_SUCCESS;
	}
	
	possibleInferiors = schema_class->possibleInferiors;
	if (possibleInferiors == NULL) {
		return LDB_SUCCESS;
	}

	for (i=0;possibleInferiors[i];i++) {
		ret = ldb_msg_add_string(msg, "possibleInferiors", possibleInferiors[i]);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
	}

	return LDB_SUCCESS;
}


/* Add objectClasses, attributeTypes and dITContentRules from the
   schema object (they are not stored in the database)
 */
static int schema_data_search_callback(struct ldb_request *req, struct ldb_reply *ares)
{
	struct ldb_context *ldb;
	struct schema_data_search_data *ac;
	struct schema_data_private_data *mc;
	int i, ret;

	ac = talloc_get_type(req->context, struct schema_data_search_data);
	mc = talloc_get_type(ldb_module_get_private(ac->module), struct schema_data_private_data);
	ldb = ldb_module_get_ctx(ac->module);

	if (!ares) {
		return ldb_module_done(ac->req, NULL, NULL,
					LDB_ERR_OPERATIONS_ERROR);
	}
	if (ares->error != LDB_SUCCESS) {
		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}
	/* Only entries are interesting, and we handle the case of the parent seperatly */

	switch (ares->type) {
	case LDB_REPLY_ENTRY:

		if (ldb_dn_compare(ares->message->dn, mc->aggregate_dn) == 0) {
			for (i=0; i < ARRAY_SIZE(generated_attrs); i++) {
				if (generated_attrs[i].aggregate &&
				    ldb_attr_in_list(ac->req->op.search.attrs, generated_attrs[i].attr)) {
					ret = generated_attrs[i].fn(ldb, ares->message, ac->schema);
					if (ret != LDB_SUCCESS) {
						return ret;
					}
				}
			}
		} else if ((ldb_dn_compare_base(mc->schema_dn, ares->message->dn) == 0)
			   && (ldb_dn_compare(mc->schema_dn, ares->message->dn) != 0)) {
			for (i=0; i < ARRAY_SIZE(generated_attrs); i++) {
				if (!generated_attrs[i].aggregate &&
				    ldb_attr_in_list(ac->req->op.search.attrs, generated_attrs[i].attr)) {
					ret = generated_attrs[i].fn(ldb, ares->message, ac->schema);
					if (ret != LDB_SUCCESS) {
						return ret;
					}
				}
			}
		}


		return ldb_module_send_entry(ac->req, ares->message, ares->controls);

	case LDB_REPLY_REFERRAL:

		return ldb_module_send_referral(ac->req, ares->referral);

	case LDB_REPLY_DONE:

		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}

	return LDB_SUCCESS;
}

/* search */
static int schema_data_search(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	int i, ret;
	struct schema_data_search_data *search_context;
	struct ldb_request *down_req;
	struct dsdb_schema *schema = dsdb_get_schema(ldb);

	if (!schema || !ldb_module_get_private(module)) {
		/* If there is no schema, there is little we can do */
		return ldb_next_request(module, req);
	}
	for (i=0; i < ARRAY_SIZE(generated_attrs); i++) {
		if (ldb_attr_in_list(req->op.search.attrs, generated_attrs[i].attr)) {
			break;
		}
	}
	if (i == ARRAY_SIZE(generated_attrs)) {
		/* No request for a generated attr found, nothing to
		 * see here, move along... */
		return ldb_next_request(module, req);
	}

	search_context = talloc(req, struct schema_data_search_data);
	if (!search_context) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	search_context->module = module;
	search_context->req = req;
	search_context->schema = schema;

	ret = ldb_build_search_req_ex(&down_req, ldb, search_context,
					req->op.search.base,
					req->op.search.scope,
					req->op.search.tree,
					req->op.search.attrs,
					req->controls,
					search_context, schema_data_search_callback,
					req);
	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	return ldb_next_request(module, down_req);
}


_PUBLIC_ const struct ldb_module_ops ldb_schema_data_module_ops = {
	.name		= "schema_data",
	.init_context	= schema_data_init,
	.add		= schema_data_add,
	.modify		= schema_data_modify,
	.search         = schema_data_search
};
