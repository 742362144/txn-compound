/**
 * Copyright (C) Stony Brook University 2016
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#include "tc_bench_util.h"

#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tc_api.h"
#include "tc_helper.h"

#include <string>
#include <vector>

using std::vector;

const size_t BUFSIZE = 4096;

void ResetTestDirectory(const char *dir)
{
	tc_rm_recursive(dir);
	tc_ensure_dir(dir, 0755, NULL);
}

vector<const char *> NewPaths(const char *format, int n, int start)
{
	vector<const char *> paths(n);
	for (int i = 0; i < n; ++i) {
		char *p = (char *)malloc(PATH_MAX);
		assert(p);
		snprintf(p, PATH_MAX, format, start + i);
		paths[i] = p;
	}
	return paths;
}

void FreePaths(vector<const char *> *paths)
{
	for (auto p : *paths)
		free((char *)p);
}

vector<tc_iovec> NewIovecs(tc_file *files, int n, size_t offset)
{
	vector<tc_iovec> iovs(n);
	for (int i = 0; i < n; ++i) {
		iovs[i].file = files[i];
		iovs[i].offset = offset;
		iovs[i].length = BUFSIZE;
		iovs[i].data = (char *)malloc(PATH_MAX);
		iovs[i].is_write_stable = true;
	}
	return iovs;
}

void FreeIovecs(vector<tc_iovec> *iovs)
{
	for (auto iov : *iovs)
		free((char *)iov.data);
}

vector<tc_attrs> NewTcAttrs(size_t nfiles, tc_attrs *values, int start)
{
	vector<const char *> paths = NewPaths("Bench-Files/file-%d", nfiles, start);
	vector<tc_attrs> attrs(nfiles);

	for (size_t i = 0; i < nfiles; ++i) {
		if (values) {
			attrs[i] = *values;
		} else {
			attrs[i].masks = TC_ATTRS_MASK_ALL;
		}
		attrs[i].file = tc_file_from_path(paths[i]);
	}

	return attrs;
}

void FreeTcAttrs(vector<tc_attrs> *attrs)
{
	for (const auto& at : *attrs) {
		free((char *)at.file.path);
	}
}

tc_attrs GetAttrValuesToSet(int nattrs)
{
	tc_attrs attrs;

	attrs.masks = TC_ATTRS_MASK_NONE;
	if (nattrs >= 1) {
		tc_attrs_set_mode(&attrs, S_IRUSR | S_IRGRP | S_IROTH);
	}
	if (nattrs >= 2) {
		tc_attrs_set_uid(&attrs, 0);
		tc_attrs_set_gid(&attrs, 0);
	}
	if (nattrs >= 3) {
		tc_attrs_set_atime(&attrs, totimespec(time(NULL), 0));
	}
	if (nattrs >= 4) {
		tc_attrs_set_size(&attrs, 8192);
	}
	return attrs;
}

void CreateFiles(vector<const char *>& paths)
{
	const size_t nfiles = paths.size();
	tc_file *files =
	    tc_openv_simple(paths.data(), nfiles, O_WRONLY | O_CREAT, 0644);
	assert(files);
	vector<tc_iovec> iovs = NewIovecs(files, nfiles);
	tc_res tcres = tc_writev(iovs.data(), nfiles, false);
	assert(tc_okay(tcres));
	tc_closev(files, nfiles);
	FreeIovecs(&iovs);
}

vector<tc_extent_pair> NewFilePairsToCopy(size_t nfiles)
{
	vector<const char *> srcs = NewPaths("file-%d", nfiles);
	CreateFiles(srcs);
	vector<const char *> dsts = NewPaths("dst-%d", nfiles);
	vector<tc_extent_pair> pairs(nfiles);
	for (size_t i = 0; i < nfiles; ++i) {
		pairs[i].src_path = srcs[i];
		pairs[i].dst_path = dsts[i];
		pairs[i].src_offset = 0;
		pairs[i].dst_offset = 0;
		pairs[i].length = BUFSIZE;
	}
	return pairs;
}

void FreeFilePairsToCopy(vector<tc_extent_pair> *pairs)
{
	for (auto& p : *pairs) {
		free((char *)p.src_path);
		free((char *)p.dst_path);
	}
}

bool DummyListDirCb(const struct tc_attrs *entry, const char *dir, void *cbarg)
{
	return true;
}

// There average directory width is 17:
//
// #find linux-4.6.3/ -type d | \
//  while read dname; do ls -l $dname | wc -l; done  | \
//  awk '{s += $1} END {print s/NR;}'
// 16.8402
void CreateDirsWithContents(vector<const char *>& dirs)
{
	const int kFilesPerDir = 17;
	vector<tc_attrs> attrs(dirs.size());
	for (size_t i = 0; i < dirs.size(); ++i) {
		tc_set_up_creation(&attrs[i], dirs[i], 0755);
	}
	tc_res tcres = tc_mkdirv(attrs.data(), dirs.size(), false);
	assert(tc_okay(tcres));

	for (size_t i = 0; i < dirs.size(); ++i) {
		char p[PATH_MAX];
		snprintf(p, PATH_MAX, "%s/%%d", dirs[i]);
		auto files = NewPaths(p, 17);
		CreateFiles(files);
		FreePaths(&files);
	}
}

void* SetUp(bool istc)
{
	void *context;
	if (istc) {
		char buf[PATH_MAX];
		context = tc_init(get_tc_config_file(buf, PATH_MAX),
				  "/tmp/tc-bench-tc.log", 77);
		fprintf(stderr, "Using config file at %s\n", buf);
	} else {
		context = tc_init(NULL, "/tmp/tc-bench-posix.log", 0);
	}
	return context;
}

void TearDown(void *context)
{
	tc_deinit(context);
}
