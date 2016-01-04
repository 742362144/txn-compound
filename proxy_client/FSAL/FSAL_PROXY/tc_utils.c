#include "config.h"

#include "fsal.h"
#include <assert.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include "ganesha_list.h"
#include "abstract_atomic.h"
#include "fsal_types.h"
#include "FSAL/fsal_commonlib.h"
#include "pxy_fsal_methods.h"
#include "fsal_nfsv4_macros.h"
#include "nfs_proto_functions.h"
#include "nfs_proto_tools.h"
#include "export_mgr.h"
#include "tc_utils.h"

/*
 *  Caller has to make sure he sets op_ctx->export
 *  arg - Array of reads
 *
 *
 *
 *
 *
 *
 */

tc_res tcread_v(struct tc_iovec *arg, int read_count, bool is_transaction)
{
	struct kernel_tcread_args *kern_arg = NULL;
	struct kernel_tcread_args *cur_arg = NULL;
	fsal_status_t fsal_status = { 0, 0 };
	int i = 0;
	struct gsh_export *export = op_ctx->export;
	tc_res result = { .okay = false, .index = 0, .err_no = (int)ENOENT };

	if (export == NULL) {
		return result;
	}

	if (export->fsal_export->obj_ops->tc_read == NULL) {
		result.err_no = (int)ENOTSUP;
		return result;
	}

	LogDebug(COMPONENT_FSAL, "tcread_v() called \n");

	kern_arg = malloc(read_count * (sizeof(struct kernel_tcread_args)));

	while (i < read_count && i < MAX_READ_COUNT) {
		cur_arg = kern_arg + i;
		cur_arg->user_arg = arg + i;
		cur_arg->opok_handle = NULL;
		cur_arg->path = NULL;
		if (cur_arg->user_arg->path != NULL) {
			cur_arg->path =
			    malloc(strlen(cur_arg->user_arg->path) + 1);
			strcpy(cur_arg->path, cur_arg->user_arg->path);
		}
		// cur_arg->read_ok = NULL;
		i++;
	}

	fsal_status =
	    export->fsal_export->obj_ops->tc_read(kern_arg, read_count);

	i = 0;
	while (i < read_count && i < MAX_READ_COUNT) {
		cur_arg = kern_arg + i;
		if (cur_arg->path != NULL) {
			free(cur_arg->path);
		}
		i++;
	}

	free(kern_arg);

	if (FSAL_IS_ERROR(fsal_status)) {
		result.index = 0; // get this to the right index
		result.err_no = (int)fsal_status.major;
		return result;
	}

	result.okay = true;
	return result;
}

/*
 *  Caller has to make sure he sets op_ctx->export
 *  arg - Array of writes
 *
 *
 *
 *
 *
 *
 */

tc_res tcwrite_v(struct tc_iovec *arg, int write_count, bool is_transaction)
{
	struct kernel_tcwrite_args *kern_arg = NULL;
	struct kernel_tcwrite_args *cur_arg = NULL;
	fsal_status_t fsal_status = { 0, 0 };
	int i = 0;
	struct gsh_export *export = op_ctx->export;
	tc_res result = { .okay = false, .index = 0, .err_no = (int)ENOENT };

	if (export == NULL) {
		return result;
	}

	if (export->fsal_export->obj_ops->tc_write == NULL) {
		result.err_no = (int)ENOTSUP;
		return result;
	}

	LogDebug(COMPONENT_FSAL, "tcwrite_v() called \n");

	kern_arg = malloc(write_count * (sizeof(struct kernel_tcwrite_args)));

	while (i < write_count && i < MAX_WRITE_COUNT) {
		cur_arg = kern_arg + i;
		cur_arg->user_arg = arg + i;
		cur_arg->opok_handle = NULL;
		cur_arg->path = NULL;
		if (cur_arg->user_arg->path != NULL) {
			cur_arg->path =
			    malloc(strlen(cur_arg->user_arg->path) + 1);
			strcpy(cur_arg->path, cur_arg->user_arg->path);
		}
		// cur_arg->write_ok = NULL;
		i++;
	}

	fsal_status =
	    export->fsal_export->obj_ops->tc_write(kern_arg, write_count);

	i = 0;
	while (i < write_count && i < MAX_WRITE_COUNT) {
		cur_arg = kern_arg + i;
		if (cur_arg->path != NULL) {
			free(cur_arg->path);
		}
		i++;
	}

	free(kern_arg);

	if (FSAL_IS_ERROR(fsal_status)) {
		result.index = 0; // get this to the right index
		result.err_no = (int)fsal_status.major;
		return result;
	}

	result.okay = true;
	return result;
}
