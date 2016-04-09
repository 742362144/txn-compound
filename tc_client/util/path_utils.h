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

#ifndef __TC_PATH_UTILS_H__
#define __TC_PATH_UTILS_H__

#include "util/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tokenize "path" into "components"; return the number of components inside
 * path on success, or -1 on failure.
 *
 * Caller owns "components" and is responsible for freeing it.
 */
int tc_path_tokenize(const char *path, slice_t **components);

/**
 * Normalize a path and save the result into "buf".  Normalization include
 * removing ".", "..", consecutive "//", and trailing "/".
 * For example:
 *
 * "/" -> "/"
 * "//" -> "/"
 * "/foo/bar/" -> "/foo/bar"
 * "/foo/../bar/" -> "/bar"
 * "/foo/../../../" -> "/"
 *
 * "path" and "buf" may be the same.
 */
int tc_path_normalize(const char *path, char *buf, size_t buf_size);

/**
 * Return the depth of the path in the FS tree. "/" is zero; "/foo" is one;
 * "/foo/bar" is two.
 */
int tc_path_depth(const char *path);

/**
 * Return the distance of two nodes in the FS tree. For example, the distance
 * is 1 between between "/" and "/foo", 2 between "/foo" and "/bar".
 *
 * When "dst" is a relative path, "src" is not used and can be NULL.
 * When "dst" is an absolute path, "src" must be an ansolute path.
 */
int tc_path_distance(const char *src, const char *dst);

/**
 * Convert "path" to relative path based on "base" if the distance between
 * "base" and "path" is smaller than the distance between "/" and "path".
 * The result relative path is stored in "buf".
 *
 * NOTE: "buf" must not overlap with "path"
 *
 * Return the size of the resultant "path".
 */
int tc_path_rebase(const char *base, const char *path, char *buf, int buf_size);

/**
 * Join "path1" and "path2" to "buf".
 *
 * "buf" and "path1" may be the same.
 */
int tc_path_join(const char *path1, const char *path2, char *buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif // __TC_PATH_UTILS_H__
