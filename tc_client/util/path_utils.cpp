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
 *
 */

#include "path_utils.h"

#include <assert.h>
#include <vector>
#include <iostream>
#include "util/slice.h"

#define TC_PATH_MAX 4096

using util::Slice;

static std::vector<Slice> tc_get_path_components(Slice path)
{
	assert(!path.empty());
	std::vector<Slice> components;
	bool is_absolute = path[0] == '/';
	path.trim('/');
	int beg = 0;
	int end = 0;   // component within [beg, end)
	while (end < path.size()) {
		while (end < path.size() && path[end] != '/') {
			++end;
		}
		Slice comp(path.data() + beg, end - beg);
		if (comp == "..") {
			if (components.empty()) {
				if (!is_absolute)
					components.push_back(comp);
			} else {
				if (components.back() != "..")
					components.pop_back();
				else
					components.push_back(comp);
			}
		} else if (!comp.empty() && comp != "/" && comp != ".") {
			// ignore '/', and "."
			components.push_back(comp);
		}
		beg = ++end;
	}

	return components;
}

int tc_path_tokenize_s(slice_t path, slice_t **components)
{
	std::vector<Slice> comps = tc_get_path_components(path);
	if (components == NULL) {
		return comps.size();
	}
	if (comps.empty()) {
		*components = NULL;
		return 0;
	}
	slice_t *sls = (slice_t *)malloc(sizeof(slice_t) * comps.size());
	if (!sls) {
		return -1;
	}
	for (int i = 0; i < comps.size(); ++i) {
		sls[i].data = comps[i].data();
		sls[i].size = comps[i].size();
	}
	*components = sls;
	return comps.size();
}

int tc_path_tokenize(const char *path, slice_t **components)
{
	size_t n;
	if (path == NULL || (n = strnlen(path, TC_PATH_MAX)) >= TC_PATH_MAX)
		return -1;
	return tc_path_tokenize_s(mkslice(path, n), components);
}

int tc_path_depth_s(slice_t path)
{
	std::vector<Slice> comps = tc_get_path_components(path);
	int depth;
	if (comps.size() == 1 && comps[0] == "/") {
		depth = 0;
	} else {
		depth = comps.size();
	}
	return depth;
}

int tc_path_depth(const char *path)
{
	return tc_path_depth_s(toslice(path));
}

int tc_path_distance_s(slice_t src, slice_t dst)
{
	assert(dst.size > 0);
	if (dst.data[0] != '/') {
		auto comps = tc_get_path_components(dst);
		return comps.size();
	}
	assert(src.size > 0 && src.data[0] == '/');
	std::vector<Slice> src_comps = tc_get_path_components(src);
	std::vector<Slice> dst_comps = tc_get_path_components(dst);
	int src_len = src_comps.size();
	int dst_len = dst_comps.size();
	if (src_len == 1 && src_comps[0] == "/") {
		return (dst_len == 1 && dst_comps[0] == "/") ? 0 : dst_len;
	}
	if (dst_len == 1 && dst_comps[0] == "/") {
		return src_len;
	}
	int l = 0;
	while (l < src_len && l < dst_len && src_comps[l] == dst_comps[l])
		++l;
	return src_len - l + dst_len - l;
}

int tc_path_distance(const char *src, const char *dst)
{
	return tc_path_distance_s(toslice(src), toslice(dst));
}

static int tc_path_append_impl(buf_t *pbuf, slice_t comp)
{
	if (comp.size == 0) {
		return 0;
	}
	if (comp.size > buf_remaining(pbuf)) {
		return -1;
	}
	if (pbuf->size != 0) {
		buf_append_char(pbuf, '/');
	}
	buf_append_slice(pbuf, comp);
}

int tc_path_join_s(slice_t path1, slice_t path2, buf_t *pbuf)
{
	tc_path_append_impl(pbuf, path1);
	tc_path_append_impl(pbuf, path2);
	return tc_path_normalize(asstr(pbuf), pbuf->data, pbuf->capacity);
}

int tc_path_join(const char *path1, const char *path2, char *buf,
		 size_t buf_size)
{
	int len1 = strnlen(path1, TC_PATH_MAX);
	int len2 = strnlen(path2, TC_PATH_MAX);

	if (len1 >= TC_PATH_MAX || len2 >= TC_PATH_MAX)
		return -1;

	buf_t bf = BUF_INITIALIZER(buf, buf_size);
	int n = tc_path_join_s(mkslice(path1, len1), mkslice(path2, len2), &bf);
	buf_append_null(&bf);

	return n;
}

int tc_path_append(buf_t *pbuf, slice_t comp)
{
	tc_path_append_impl(pbuf, comp);
	return tc_path_normalize(asstr(pbuf), pbuf->data, pbuf->capacity);
}

int tc_path_normalize_s(slice_t path, buf_t *pbuf)
{
	std::vector<Slice> components = tc_get_path_components(path);
	if (components.empty()) {
		buf_append_char(pbuf, path.data[0] == '/' ? '/' : '.');
		return 1;
	}

	int old_size = pbuf->size;
	if (path.data[0] == '/') {
		buf_append_char(pbuf, '/');
	}

	for (int i = 0; i < components.size(); ++i) {
		if (i > 0) buf_append_char(pbuf, '/');
		buf_append_slice(pbuf, components[i].toslice());
	}
	return pbuf->size - old_size;
}

int tc_path_normalize(const char *path, char *buf, size_t buf_size)
{
	int plen;
	if (!path || (plen = strnlen(path, TC_PATH_MAX)) >= TC_PATH_MAX ||
	    buf_size <= 1)
		return -1;

	buf_t bf = BUF_INITIALIZER(buf, buf_size);
	int n = tc_path_normalize_s(mkslice(path, plen), &bf);
	buf_append_null(&bf);
	return n;
}

int tc_path_rebase_s(slice_t base, slice_t path, buf_t *pbuf)
{
	std::vector<Slice> base_comps = tc_get_path_components(base);
	std::vector<Slice> path_comps = tc_get_path_components(path);
	int l = 0;
	while (l < base_comps.size() && l < path_comps.size() &&
	       base_comps[l] == path_comps[l])
		++l;
	int dist = base_comps.size() - l + path_comps.size() - l;
	if (dist >= path_comps.size()) {
		// No need to rebase
		buf_append_slice(pbuf, path);
		return path.size;
	}

	std::vector<Slice> relative_comps;
	int result_size = 0;
	for (int i = l; i < base_comps.size(); ++i) {
		relative_comps.push_back("..");
		result_size += 2;
	}
	for (int j = l; j < path_comps.size(); ++j) {
		relative_comps.push_back(path_comps[j]);
		result_size += path_comps[j].size();
	}
	result_size += relative_comps.size() - 1;  // count "/"s
	if (result_size > buf_remaining(pbuf)) {
		return -1;  // buffer too small
	}

	size_t size = 0;
	for (int i = 0; i < relative_comps.size(); ++i) {
		if (i > 0) size += buf_append_char(pbuf, '/');
		size += buf_append_slice(pbuf, relative_comps[i].toslice());
	}
	if (size == 0) {  // empty
		size += buf_append_char(pbuf, '.');
	}
	return size;
}

int tc_path_rebase(const char *base, const char *path, char *buf,
		   size_t buf_size)
{
	buf_t bf = BUF_INITIALIZER(buf, buf_size);

	int n = tc_path_rebase_s(toslice(base), toslice(path), &bf);
	buf_append_null(&bf);

	return n;
}
