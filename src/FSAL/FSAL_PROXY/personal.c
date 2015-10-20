#include "config.h"
#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fsal_types.h"
#include "fsal.h"
#include "fsal_api.h"
#include "FSAL/fsal_init.h"
#include "pxy_fsal_methods.h"
#include "personal.h"

bool
readdir_reply(const char *name, void *dir_state,
                fsal_cookie_t cookie)
{
	LogDebug(COMPONENT_FSAL, "readdir_reply() called\n");
	return true;
}

int personal_init() {
	struct pxy_fsal_module *new_module = NULL;
	struct gsh_export *export = NULL;
	struct fsal_obj_handle *vfs0_handle = NULL;
	struct fsal_obj_handle *abcd_handle = NULL;
	fsal_status_t fsal_status = { 0, 0 };
	cache_entry_t *entry = NULL;
	char *data_buf = NULL;
	size_t read_amount = 0;
	bool eof = false;
	struct req_op_context req_ctx;

	LogDebug(COMPONENT_FSAL, "personal_init() called\n");
	new_module = lookup_fsal("PROXY");
	if (new_module == NULL) {
		LogDebug(COMPONENT_FSAL, "Proxy Module Not found\n");
		return -1;
	}
	LogDebug(COMPONENT_FSAL, "Proxy Module Found\n");
	export = get_gsh_export(77);
	if(export == NULL){
		LogDebug(COMPONENT_FSAL, "Export Not found\n");
		return -1;
	}
	LogDebug(COMPONENT_FSAL, "Export Found\n");
	LogDebug(COMPONENT_FSAL,
                 "Export %d at pseudo (%s) with path (%s) and tag (%s) \n",
                 export->export_id, export->pseudopath,
                 export->fullpath, export->FS_tag);
	
	if(nfs_export_get_root_entry(export, &entry)
                    != CACHE_INODE_SUCCESS){
		LogDebug(COMPONENT_FSAL, "get root() failed\n");
	}
	LogDebug(COMPONENT_FSAL, "get root() success\n");

	memset(&req_ctx, 0, sizeof(struct req_op_context));
	op_ctx = &req_ctx;
	op_ctx->creds = NULL;
	op_ctx->fsal_export = export->fsal_export;

	//fsal_status = pxy_lookup(NULL, "vfs0", &vfs0_handle);
	fsal_status = export->fsal_export->obj_ops->lookup(NULL, "vfs0", &vfs0_handle);
	if (FSAL_IS_ERROR(fsal_status)) {
		LogDebug(COMPONENT_FSAL, "lookup() for vfs0 failed\n");
		return -1;
	}

	LogDebug(COMPONENT_FSAL, "lookup() for vfs0 succeeded\n");
	if (vfs0_handle == NULL) {
		LogDebug(COMPONENT_FSAL, "vfs0_handle is NULL\n");
		return -1;
	}

	fsal_status = export->fsal_export->obj_ops->lookup(vfs0_handle, "abcd", &abcd_handle);
	if (FSAL_IS_ERROR(fsal_status)) {
                LogDebug(COMPONENT_FSAL, "lookup() for abcd failed\n");
		return -1;
        }

	LogDebug(COMPONENT_FSAL, "lookup() for abcd succeeded\n");
	if (abcd_handle == NULL) {
		LogDebug(COMPONENT_FSAL, "abcd_handle is NULL\n");
		return -1;
	}

	fsal_status = export->fsal_export->obj_ops->read(abcd_handle, 0, 1024, data_buf, &read_amount, &eof);
	if (FSAL_IS_ERROR(fsal_status)) {
                LogDebug(COMPONENT_FSAL, "read() for abcd failed\n");
                return -1;
        }

	LogDebug(COMPONENT_FSAL, "read() for abcd succeeded\n");
	LogDebug(COMPONENT_FSAL, "amount read: %u\n", read_amount);

	fsal_status = export->fsal_export->obj_ops->getattrs(abcd_handle);
	if (FSAL_IS_ERROR(fsal_status)) {
                LogDebug(COMPONENT_FSAL, "getattr() for abcd failed\n");
                return -1;
        }

	LogDebug(COMPONENT_FSAL, "getattr() for abcd succeeded\n");

	fsal_status = export->fsal_export->obj_ops->readdir(abcd_handle, NULL, NULL, readdir_reply, &eof);
	if (FSAL_IS_ERROR(fsal_status)) {
                LogDebug(COMPONENT_FSAL, "read_dir() for abcd failed\n");
        }

	fsal_status = export->fsal_export->obj_ops->readdir(vfs0_handle, NULL, NULL, readdir_reply, &eof);
        if (FSAL_IS_ERROR(fsal_status)) {
                LogDebug(COMPONENT_FSAL, "read_dir() for vfs0 failed\n");
                return -1;
        }

        LogDebug(COMPONENT_FSAL, "readdir() for vfs0 succeeded\n");

	read_amount = 0;
	fsal_status = export->fsal_export->obj_ops->write(abcd_handle, 0, 20, "check write", &read_amount, &eof);
        if (FSAL_IS_ERROR(fsal_status)) {
                LogDebug(COMPONENT_FSAL, "write() for abcd failed\n");
                return -1;
        }

	LogDebug(COMPONENT_FSAL, "write() for abcd succeeded, %u written\n", read_amount);

	return 0;
}
