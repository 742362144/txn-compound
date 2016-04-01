/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright CEA/DAM/DIF  (2008)
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * ---------------------------------------
 */

/**
 * @file nfs_main.c
 * @brief The file that contain the 'main' routine for the nfsd.
 *
 */
#include "config.h"
#include "nfs_init.h"
#include "fsal.h"
#include "log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>		/* for sigaction */
#include <errno.h>
#include "fsal_pnfs.h"
#include "../nfs4/nfs4_util.h"

config_file_t config_struct;

static char exe_path[PATH_MAX];
static char tc_config_path[PATH_MAX];

int main(int argc, char *argv[])
{
	struct fsal_module* module = NULL;
	int rc = 0;

	readlink("/proc/self/exe", exe_path, PATH_MAX);
	snprintf(tc_config_path, PATH_MAX,
		 "%s/../../secnfs/config/vfs.proxy.conf", dirname(exe_path));
	fprintf(stderr, "using config file: %s\n", tc_config_path);

	module = (struct fsal_module *)nfs4_init(tc_config_path,
						 "/tmp/tc_daemon.log", 77);

	if (module == NULL) {
		LogFatal(COMPONENT_INIT, "Error while initializing tc_client");
	}

	/* Everything seems to be OK! We can now start service threads */
	tc_singlefile("/vfs0/test_cdist/abcd", 65536, 1, 200000, 3, 0.0, 0);

	nfs4_deinit(module);

	return 0;

}
