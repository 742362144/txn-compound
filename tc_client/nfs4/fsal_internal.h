/**
 * @file    FSAL_PROXY/fsal_internal.h
 * @brief   Extern definitions for variables that are
 *          defined in fsal_internal.c.
 */

#include  "fsal.h"
#include "FSAL/common_functions.h"
#include "nfs4.h"

#ifndef FSAL_INTERNAL_H
#define FSAL_INTERNAL_H
/* the following variables must not be defined in fsal_internal.c */
#ifndef FSAL_INTERNAL_C

#define FSAL_PROXY_OWNER_LEN 256

/* static filesystem info.
 * read access only.
 */
extern fsal_staticfsinfo_t global_fs_info;

#endif

/**
 * fsal_do_log:
 * Indicates if an FSAL error has to be traced
 * into its log file in the NIV_EVENT level.
 * (in the other cases, return codes are only logged
 * in the NIV_FULL_DEBUG logging lovel).
 */
bool fsal_do_log(fsal_status_t status);

void fsal_interval_kern_fsalattr2bitmap4(fsal_attrib_list_t *pfsalattr,
					  bitmap4 *pbitmap);

/*
 * A few functions dedicated in kern related information
 * management and conversion
 */
fsal_status_t fsal_internal_set_auth_gss(kernfsal_op_context_t *
					 p_thr_context);
fsal_status_t fsal_internal_kern_error_convert(nfsstat4 nfsstatus,
						int indexfunc);
int fsal_internal_kern_create_fh(nfs_fh4 *pnfs4_handle, fsal_nodetype_t type,
				  fsal_u64_t fileid,
				  fsal_handle_t *pfsal_handle);
int fsal_internal_kern_extract_fh(nfs_fh4 *pnfs4_handle,
				   fsal_handle_t *pfsal_handle);
int fsal_internal_kern_fsal_name_2_utf8(fsal_name_t *pname,
					 utf8string *utf8str);
int fsal_internal_kern_fsal_path_2_utf8(fsal_path_t *ppath,
					 utf8string *utf8str);
int fsal_internal_kern_fsal_utf8_2_name(fsal_name_t *pname,
					 utf8string *utf8str);
int fsal_internal_kern_fsal_utf8_2_path(fsal_path_t *ppath,
					 utf8string *utf8str);
int kern_Fattr_To_FSAL_attr(fsal_attrib_list_t *pFSAL_attr,
			     kernfsal_handle_t *phandle, fattr4 *Fattr);

fsal_status_t FSAL_kern_setclientid(kernfsal_op_context_t *p_context);
fsal_status_t FSAL_kern_setclientid_renego(kernfsal_op_context_t *p_context);

fsal_status_t fsal_kern_create_rpc_clnt(kernfsal_op_context_t *);
int fsal_internal_ClientReconnect(kernfsal_op_context_t *p_thr_context);
fsal_status_t FSAL_kern_open_confirm(kernfsal_file_t *pfd);
void *FSAL_kern_change_user(kernfsal_op_context_t *p_thr_context);

#endif
