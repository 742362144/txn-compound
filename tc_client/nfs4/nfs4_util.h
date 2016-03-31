/* Header file for implementing tc features */

#ifndef __TC_NFS4_UTIL_H__
#define __TC_NFS4_UTIL_H__

#include "export_mgr.h"
#include "tc_impl_nfs4.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structure to be passed to ktcread
 * user_arg - Contains file-path, user buffer, read length, offset, etc
 * which are passed by the user
 */
struct tcread_kargs
{
	struct tc_iovec *user_arg;
	char *path;
	union
	{
		READ4resok *v4_rok;
	} read_ok;
	OPEN4resok *opok_handle;
	struct attrlist attrib;
};

/*
 * Structure to be passed to ktcwrite
 * user_arg - Contains file-path, user buffer, write length, offset, etc
 * which are passed by the user
 */
struct tcwrite_kargs
{
	struct tc_iovec *user_arg;
	char *path;
	union
	{
		WRITE4resok *v4_wok;
	} write_ok;
	OPEN4resok *opok_handle;
	struct attrlist attrib;
};

#define MAX_READ_COUNT      10
#define MAX_WRITE_COUNT     10
#define MAX_DIR_DEPTH       10
#define MAX_FILENAME_LENGTH 256

int tc_seq();
bool readdir_reply(const char *name, void *dir_state, fsal_cookie_t cookie);

#ifdef __cplusplus
}
#endif


#endif // __TC_NFS4_UTIL_H__
