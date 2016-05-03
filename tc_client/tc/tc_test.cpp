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

/**
 * XXX: To add a new test, don't forget to register the test in
 * REGISTER_TYPED_TEST_CASE_P().
 *
 * This file uses an advanced GTEST feature called Type-Parameterized Test,
 * which is documented at
 * https://github.com/google/googletest/blob/master/googletest/docs/V1_7_AdvancedGuide.md
 */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <gtest/gtest.h>

#include "tc_api.h"
#include "tc_helper.h"
#include "util/fileutil.h"
#include "log.h"

#define APPEND -1
#define CURRENT -2

#define TCTEST_ERR(fmt, args...) LogCrit(COMPONENT_TC_TEST, fmt, ##args)
#define TCTEST_WARN(fmt, args...) LogWarn(COMPONENT_TC_TEST, fmt, ##args)
#define TCTEST_INFO(fmt, args...) LogInfo(COMPONENT_TC_TEST, fmt, ##args)
#define TCTEST_DEBUG(fmt, args...) LogDebug(COMPONENT_TC_TEST, fmt, ##args)

/**
 * TODO(mchen): move to fileutil.h
 * Ensure the file does not exist
 * before test.
 */
static void RemoveFile(const char *path)
{
	int r = unlink(path);
	EXPECT_TRUE(r == 0 || errno == ENOENT);
}

/**
 * Ensure files does not exist
 * before test.
 */
static void RemoveFiles(const char **path, int count)
{
	int i = 0, r = 0;

	while (i < count) {
		r = unlink(path[i]);
		EXPECT_TRUE(r == 0 || errno == ENOENT);
		i++;
	}
}

/**
 * Ensure Directory does not exist
 * before test.
 */
static void RemoveDir(const char **path, int count)
{
	int i = 0, r = 0;

	while (i < count) {
		r = rmdir(path[i]);
		EXPECT_TRUE(r == 0 || r == ENOENT)
		    << "error removing dirs: " << strerror(r);
		i++;
	}
}

/**
 * Set the tc_iovec
 */
static tc_iovec *set_iovec_file_paths(const char **PATH, int count,
				      int is_write, int offset)
{
	int i = 0;
	tc_iovec *user_arg = NULL;
	const int N = 4096;

	user_arg = (tc_iovec *)calloc(count, sizeof(tc_iovec));

	while (i < count) {
		if (PATH[i] == NULL) {
			TCTEST_WARN(
			    "set_iovec_FilePath() failed for file : %s\n",
			    PATH[i]);

			int indx = 0;
			while (indx < i) {
				free((user_arg + indx)->data);
				indx++;
			}
			free(user_arg);

			return NULL;
		}

		(user_arg + i)->file = tc_file_from_path(PATH[i]);
		(user_arg + i)->offset = offset;

		(user_arg + i)->length = N;
		(user_arg + i)->data = (void *)malloc(N);

		if (is_write)
			(user_arg + i)->is_creation = 1;

		i++;
	}

	return user_arg;
}

class TcPosixImpl {
public:
	static void *tcdata;
	static constexpr const char* POSIX_TEST_DIR = "/tmp/tc_posix_test";
	static void SetUpTestCase() {
		tcdata = tc_init(NULL, "/tmp/tc-posix.log", 0);
		TCTEST_WARN("Global SetUp of Posix Impl\n");
		util::CreateOrUseDir(POSIX_TEST_DIR);
		chdir(POSIX_TEST_DIR);
	}
	static void TearDownTestCase() {
		TCTEST_WARN("Global TearDown of Posix Impl\n");
		tc_deinit(tcdata);
	}
	static void SetUp() {
		TCTEST_WARN("SetUp Posix Impl Test\n");
	}
	static void TearDown() {
		TCTEST_WARN("TearDown Posix Impl Test\n");
	}
};
void *TcPosixImpl::tcdata = NULL;

class TcNFS4Impl {
public:
	static void *tcdata;
	static void SetUpTestCase() {
		tcdata = tc_init("../../../config/tc.ganesha.conf",
				 "/tmp/tc-nfs4.log", 77);
		TCTEST_WARN("Global SetUp of NFS4 Impl\n");
		tc_res res = tc_ensure_dir("/vfs0/tc_nfs4_test", 0755, NULL);
		EXPECT_TRUE(res.okay);
		tc_chdir("/vfs0/tc_nfs4_test");  /* change to mnt point */
	}
	static void TearDownTestCase() {
		TCTEST_WARN("Global TearDown of NFS4 Impl\n");
		tc_deinit(tcdata);
	}
	static void SetUp() {
		TCTEST_WARN("SetUp NFS4 Impl Test\n");
	}
	static void TearDown() {
		TCTEST_WARN("TearDown NFS4 Impl Test\n");
	}
};
void *TcNFS4Impl::tcdata = NULL;

template <typename T>
class TcTest : public ::testing::Test {
public:
	static void SetUpTestCase() {
		T::SetUpTestCase();
	}
	static void TearDownTestCase() {
		T::TearDownTestCase();
	}
	void SetUp() override {
		T::SetUp();
	}
	void TearDown() override {
		T::TearDown();
	}
};

TYPED_TEST_CASE_P(TcTest);

/**
 * TC-Read and Write test using
 * File path
 */
TYPED_TEST_P(TcTest, WritevCanCreateFiles)
{
	const char *PATH[] = { "WritevCanCreateFiles1.txt",
			       "WritevCanCreateFiles2.txt",
			       "WritevCanCreateFiles3.txt",
			       "WritevCanCreateFiles4.txt" };
	char data[] = "abcd123";
	tc_res res;
	int count = 4;

	RemoveFiles(PATH, count);

	struct tc_iovec *writev = NULL;
	writev = set_iovec_file_paths(PATH, count, 1, 0);
	EXPECT_FALSE(writev == NULL);

	res = tc_writev(writev, count, false);
	EXPECT_TRUE(res.okay);

	struct tc_iovec *readv = NULL;
	readv = set_iovec_file_paths(PATH, count, 0, 0);
	EXPECT_FALSE(readv == NULL);

	res = tc_readv(readv, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare_content(writev, readv, count));

	free_iovec(writev, count);
	free_iovec(readv, count);
}

/**
 * Set the TC I/O vector
 */
static tc_iovec *set_iovec_fd(int *fd, int count, int offset)
{
	int i = 0, N = 4096;
	tc_iovec *user_arg = NULL;

	user_arg = (tc_iovec *)calloc(count, sizeof(tc_iovec));

	while (i < count) {
		if (fd[i] < 0) {
			TCTEST_WARN(
			    "set_iovec_fd() failed for fd at index : %d\n",
			    fd[i]);

			int indx = 0;
			while (indx < i) {
				free((user_arg + indx)->data);
				indx++;
			}
			free(user_arg);

			return NULL;
		}

		(user_arg + i)->file.type = TC_FILE_DESCRIPTOR;
		(user_arg + i)->file.fd = fd[i];
		(user_arg + i)->offset = offset;
		(user_arg + i)->length = N;
		(user_arg + i)->data = (void *)malloc(N);

		i++;
	}

	return user_arg;
}

/**
 * TC-Read and Write test using
 * File Descriptor
 */
TYPED_TEST_P(TcTest, TestFileDesc)
{
	const char *PATH[] = { "WritevCanCreateFiles1.txt",
			       "WritevCanCreateFiles2.txt",
			       "WritevCanCreateFiles3.txt",
			       "WritevCanCreateFiles4.txt" };
	const int N = 7;
	char data[] = "abcd123";
	tc_res res;
	int i = 0, count = 4;
	int fd[count];
	int open_flags = O_RDWR | O_CREAT;

	RemoveFiles(PATH, 4);

	while (i < count) {
		fd[i] = open(PATH[i], open_flags);
		if (fd[i] < 0)
			TCTEST_WARN("open failed for file %s\n", PATH[i]);
		i++;
	}

	struct tc_iovec *writev = NULL;
	writev = set_iovec_fd(fd, count, 0);
	EXPECT_FALSE(writev == NULL);

	res = tc_writev(writev, count, false);
	EXPECT_TRUE(res.okay);

	struct tc_iovec *readv = NULL;
	readv = set_iovec_fd(fd, count, 0);
	EXPECT_FALSE(readv == NULL);

	res = tc_readv(readv, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare_content(writev, readv, count));

	free_iovec(writev, count);
	free_iovec(readv, count);

	i = 0;
	while (i < count) {
		close(fd[i]);
		i++;
	}
}

/**
 * Compare the attributes once set, to check if set properly
 */

bool compare(tc_attrs *usr, tc_attrs *check, int count)
{
	int i = 0;
	tc_attrs *written = NULL;
	tc_attrs *read = NULL;

	while (i < count) {

		written = usr + i;
		read = check + i;

		if (written->masks.has_mode) {
			if (!written->mode & read->mode) {
				TCTEST_WARN("Mode does not match\n");
				TCTEST_WARN(" %d %d\n", written->mode,
					   read->mode);

				return false;
			}
		}

		if (written->masks.has_rdev) {
			if (memcmp((void *)&(written->rdev),
				   (void *)&(read->rdev), sizeof(read->rdev))) {
				TCTEST_WARN("rdev does not match\n");
				TCTEST_WARN(" %d %d\n", written->rdev,
					   read->rdev);

				return false;
			}
		}

		if (written->masks.has_nlink) {
			if (written->nlink != read->nlink) {
				TCTEST_WARN("nlink does not match\n");
				TCTEST_WARN(" %d %d\n", written->nlink,
					   read->nlink);

				return false;
			}
		}

		if (written->masks.has_uid) {
			if (written->uid != read->uid) {
				TCTEST_WARN("uid does not match\n");
				TCTEST_WARN(" %d %d\n", written->uid, read->uid);

				return false;
			}
		}

		if (written->masks.has_gid) {
			if (written->gid != read->gid) {
				TCTEST_WARN("gid does not match\n");
				TCTEST_WARN(" %d %d\n", written->gid, read->gid);

				return false;
			}
		}

		if (written->masks.has_atime) {
			if (memcmp((void *)&(written->atime),
				   (void *)&(read->atime),
				   sizeof(read->atime))) {
				TCTEST_WARN("atime does not match\n");
				TCTEST_WARN(" %d %d\n", written->atime,
					   read->atime);

				return false;
			}
		}

		if (written->masks.has_mtime) {
			if (memcmp((void *)&(written->mtime),
				   (void *)&(read->mtime),
				   sizeof(read->mtime))) {
				TCTEST_WARN("mtime does not match\n");
				TCTEST_WARN(" %d %d\n", written->mtime,
					   read->mtime);

				return false;
			}
		}

		i++;
	}

	return true;
}

/**
 * Set the TC Attributes
 */
static tc_attrs *set_tc_attrs(const char **PATH, int count, bool isPath)
{
	if (count > 3) {
		TCTEST_WARN("count should be less than 4\n");
		return NULL;
	}

	tc_attrs *change_attr = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	tc_attrs_masks masks[3] = { 0 };
	int i = 0;

	uid_t uid[] = { 2711, 456, 789 };
	gid_t gid[] = { 87, 4566, 2311 };
	mode_t mode[] = { S_IRUSR | S_IRGRP | S_IROTH,
			  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH, S_IRWXU };
	size_t size[] = { 256, 56, 125 };
	time_t atime[] = { time(NULL), 1234, 567 };

	while (i < count) {

		if (PATH[i] == NULL) {
			free(change_attr);
			return NULL;
		}

		if (isPath) {
			(change_attr + i)->file.type = TC_FILE_PATH;
			(change_attr + i)->file.path = PATH[i];

		} else {
			(change_attr + i)->file.type = TC_FILE_DESCRIPTOR;
			(change_attr + i)->file.fd =
			    open(PATH[i], O_RDWR | O_CREAT);

			if ((change_attr + i)->file.fd < 0) {
				free(change_attr);
				return NULL;
			}
		}

		(change_attr + i)->mode = mode[i];
		(change_attr + i)->size = size[i];
		(change_attr + i)->uid = uid[i];
		(change_attr + i)->gid = gid[i];
		(change_attr + i)->atime.tv_sec = atime[i];
		(change_attr + i)->mtime.tv_sec = time(NULL);

		masks[i].has_mode = 1;
		masks[i].has_size = 1;
		masks[i].has_atime = 1;
		masks[i].has_mtime = 1;
		masks[i].has_uid = 1;
		masks[i].has_gid = 1;
		masks[i].has_rdev = 0;
		masks[i].has_nlink = 0;
		masks[i].has_ctime = 0;

		change_attr[i].masks = masks[i];

		i++;
	}

	return change_attr;
}

/* Set the TC attributes masks */
static void set_attr_masks(tc_attrs *write, tc_attrs *read, int count)
{
	int i = 0;
	tc_attrs *write_attr = NULL;
	tc_attrs *read_attr = NULL;

	while (i < count) {
		write_attr = write + i;
		read_attr = read + i;

		/* set tc_file */
		read_attr->file = write_attr->file;

		/* set masks */
		read_attr->masks.has_mode = write_attr->masks.has_mode;
		read_attr->masks.has_size = write_attr->masks.has_size;
		read_attr->masks.has_atime = write_attr->masks.has_atime;
		read_attr->masks.has_mtime = write_attr->masks.has_mtime;
		read_attr->masks.has_uid = write_attr->masks.has_uid;
		read_attr->masks.has_gid = write_attr->masks.has_gid;
		read_attr->masks.has_rdev = write_attr->masks.has_rdev;
		read_attr->masks.has_nlink = write_attr->masks.has_nlink;
		read_attr->masks.has_ctime = write_attr->masks.has_ctime;

		i++;
	}
}

/**
 * TC-Set/Get Attributes test
 * using File Path
 */
TYPED_TEST_P(TcTest, AttrsTestPath)
{
	const char *PATH[] = { "WritevCanCreateFiles1.txt",
			       "WritevCanCreateFiles2.txt",
			       "WritevCanCreateFiles3.txt" };
	tc_res res = { 0 };
	int count = 3;

	tc_attrs *write_attrs = NULL;
	write_attrs = set_tc_attrs(PATH, count, true);
	EXPECT_FALSE(write_attrs == NULL);

	res = tc_setattrsv(write_attrs, count, false);
	EXPECT_TRUE(res.okay);

	tc_attrs *read_attrs = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	set_attr_masks(write_attrs, read_attrs, count);

	res = tc_getattrsv(read_attrs, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare(write_attrs, read_attrs, count));

	free(write_attrs);
	free(read_attrs);
}

/*
 * TC-Set/Get Attributes test
 * using File Descriptor
 */
TYPED_TEST_P(TcTest, AttrsTestFileDesc)
{
	const char *PATH[] = { "WritevCanCreateFiles4.txt",
			       "WritevCanCreateFiles5.txt",
			       "WritevCanCreateFiles6.txt" };
	tc_res res = { 0 };
	int i = 0, count = 3;

	RemoveFiles(PATH, count);

	tc_attrs *write_attrs = NULL;
	write_attrs = set_tc_attrs(PATH, count, false);
	EXPECT_FALSE(write_attrs == NULL);

	res = tc_setattrsv(write_attrs, count, false);
	EXPECT_TRUE(res.okay);

	tc_attrs *read_attrs = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	set_attr_masks(write_attrs, read_attrs, count);

	res = tc_getattrsv(read_attrs, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare(write_attrs, read_attrs, count));

	while (i < count) {
		close((read_attrs + i)->file.fd);
		i++;
	}

	free(write_attrs);
	free(read_attrs);
}

/**
 * List Directory Contents Test
 */
TYPED_TEST_P(TcTest, ListDirContents)
{
	tc_attrs *contents = (tc_attrs *)calloc(5, sizeof(tc_attrs));
	tc_attrs_masks masks = { 0 };
	int count = 0;

	masks.has_mode = 1;
	masks.has_size = 1;
	masks.has_atime = 1;
	masks.has_mtime = 1;
	masks.has_uid = 1;
	masks.has_gid = 1;
	masks.has_rdev = 1;
	masks.has_nlink = 1;
	masks.has_ctime = 1;

	contents->masks = masks;

	tc_res res = tc_listdir(".", masks, 5, &contents, &count);
	EXPECT_TRUE(res.okay);

	tc_attrs *read_attrs = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	set_attr_masks(contents, read_attrs, count);

	res = tc_getattrsv(read_attrs, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare(contents, read_attrs, count));

	free(contents);
	free(read_attrs);
}

/**
 * Rename File Test
 */
TYPED_TEST_P(TcTest, RenameFile)
{
	int i = 0;
	const char *src_path[] = { "WritevCanCreateFiles1.txt",
				   "WritevCanCreateFiles2.txt",
				   "WritevCanCreateFiles3.txt",
				   "WritevCanCreateFiles4.txt" };

	const char *dest_path[] = { "rename1.txt", "rename2.txt",
				    "rename3.txt", "rename4.txt" };

	tc_file_pair *file = (tc_file_pair *)calloc(4, sizeof(tc_file_pair));

	while (i < 4) {
		file[i].src_file = tc_file_from_path(src_path[i]);
		file[i].dst_file = tc_file_from_path(dest_path[i]);

		i++;
	}

	tc_res res = tc_renamev(file, 4, false);

	EXPECT_TRUE(res.okay);

	free(file);
}

/**
 * Remove File Test
 */
TYPED_TEST_P(TcTest, RemoveFileTest)
{
	int i = 0;
	const char *path[] = { "rename1.txt", "rename2.txt",
			       "rename3.txt", "rename4.txt" };

	tc_file *file = (tc_file *)calloc(4, sizeof(tc_file));

	while (i < 4) {
		file[i] = tc_file_from_path(path[i]);

		i++;
	}

	tc_res res = tc_removev(file, 4, false);

	EXPECT_TRUE(res.okay);

	free(file);
}

/**
 * Make Directory Test
 */
TYPED_TEST_P(TcTest, MakeDirectory)
{
	int i = 0;
	mode_t mode[] = { S_IRWXU, S_IRUSR | S_IRGRP | S_IROTH,
			  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH };
	const char *path[] = { "a", "b", "c" };
	struct tc_attrs dirs[3];

	RemoveDir(path, 3);

	while (i < 3) {
		tc_set_up_creation(&dirs[i], path[i], 0755);
		i++;
	}

	tc_res res = tc_mkdirv(dirs, 3, false);

	EXPECT_TRUE(res.okay);
}

/**
 * Append test case
 */
TYPED_TEST_P(TcTest, Append)
{
	const char *PATH[] = { "WritevCanCreateFiles6.txt" };
	int i = 0, count = 4, N = 4096;
	struct stat st;
	void *data = calloc(1, N);
	tc_res res;

	struct tc_iovec *writev = NULL;
	writev = set_iovec_file_paths(PATH, 1, 1, APPEND);
	EXPECT_FALSE(writev == NULL);

	int fd = open(PATH[0], O_RDONLY);
	EXPECT_FALSE(fd < 0);

	while (i < 4) {
		fstat(fd, &st);
		free(writev->data);
		writev->data = (void *)malloc(N);
		res = tc_writev(writev, 1, false);
		EXPECT_TRUE(res.okay);

		/* read the data from the file from the same offset */
		int error = pread(fd, data, writev->length, st.st_size);
		EXPECT_FALSE(error < 0);

		/* compare data written with just read data from the file */
		error = memcmp(data, writev->data, writev->length);
		EXPECT_TRUE(error == 0);

		i++;
	}

	free(data);
	free_iovec(writev, 1);
}

/**
 * Successive reads
 */
TYPED_TEST_P(TcTest, SuccesiveReads)
{
	const char *path = "WritevCanCreateFiles6.txt";
	int fd[2], i = 0, N = 4096;
	fd[0] = open(path, O_RDONLY);
	fd[1] = open(path, O_RDONLY);
	tc_res res;
	off_t offset = 0;

	void *data = calloc(1, N);

	struct tc_iovec *readv = NULL;
	readv = set_iovec_fd(fd, 1, CURRENT);
	EXPECT_FALSE(readv == NULL);

	/* move th current pointer by 10 bytes */
	lseek(fd[0], 10, SEEK_CUR);

	while (i < 4) {
		/* get the current offset of the file */
		offset = lseek(fd[0], 0, SEEK_CUR);

		res = tc_readv(readv, 1, false);
		EXPECT_TRUE(res.okay);

		TCTEST_WARN("Test reading from offset : %d\n", offset);

		/* read from the file to compare the data */
		int error = pread(fd[1], data, readv->length, offset);
		EXPECT_FALSE(error < 0);

		/* compare the content read */
		error = memcmp(data, readv->data, readv->length);
		EXPECT_TRUE(error == 0);

		i++;
	}

	free(data);
	free_iovec(readv, 1);

	RemoveFile(path);
}

/**
 * Successive writes
 */
TYPED_TEST_P(TcTest, SuccesiveWrites)
{
	const char *path = "WritevCanCreateFiles10.txt";
	int fd[2], i = 0, N = 4096;
	off_t offset = 0;
	void *data = calloc(1, N);
	tc_res res;

	/*
	 * open file one for actual writing
	 * other descriptor to verify
	 */
	fd[0] = open(path, O_WRONLY | O_CREAT);
	fd[1] = open(path, O_RDONLY);
	EXPECT_FALSE(fd[0] < 0);
	EXPECT_FALSE(fd[1] < 0);

	struct tc_iovec *writev = NULL;
	writev = set_iovec_fd(fd, 1, CURRENT);
	EXPECT_FALSE(writev == NULL);

	while (i < 4) {
		/* get the current offset of the file */
		offset = lseek(fd[0], 0, SEEK_CUR);

		free(writev->data);
		writev->data = (void *)malloc(N);

		res = tc_writev(writev, 1, false);
		EXPECT_TRUE(res.okay);

		TCTEST_WARN("Test read from offset : %d\n", offset);

		/* read the data from the file from the same offset */
		int error = pread(fd[1], data, writev->length, offset);
		EXPECT_FALSE(error < 0);

		/* compare data written with just read data from the file */
		error = memcmp(data, writev->data, writev->length);
		EXPECT_TRUE(error == 0);

		i++;
	}

	free(data);
	free_iovec(writev, 1);

	RemoveFile(path);
}

static char *getRandomBytes(int N)
{
	int fd;
	char *buf;
	ssize_t ret;
	ssize_t n;

	buf = (char *)malloc(N);
	if (!buf) {
		return NULL;
	}

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		free(buf);
		return NULL;
	}

	n = 0;
	while (n < N) {
		ret = read(fd, buf + n, MIN(16384, N - n));
		if (ret < 0) {
			free(buf);
			close(fd);
			return NULL;
		}
		n += ret;
	}

	close(fd);
	return buf;
}

TYPED_TEST_P(TcTest, CopyFiles)
{
	const int N = 1024 * 1024;
	struct tc_extent_pair pairs[2];
	struct tc_iovec iov[2];
	struct tc_iovec read_iov[2];
	tc_res tcres;

	pairs[0].src_path = "SourceFile1.txt";
	pairs[0].src_offset = 0;
	pairs[0].dst_path = "DestinationFile1.txt";
	pairs[0].dst_offset = 0;
	pairs[0].length = N;

	pairs[1].src_path = "SourceFile2.txt";
	pairs[1].src_offset = 0;
	pairs[1].dst_path = "DestinationFile2.txt";
	pairs[1].dst_offset = 0;
	pairs[1].length = N;

	// create source files
	iov[0].file = tc_file_from_path(pairs[0].src_path);
	iov[0].is_creation = true;
	iov[0].offset = 0;
	iov[0].length = N;
	iov[0].data = getRandomBytes(N);
	EXPECT_TRUE(iov[0].data);
	iov[1].file = tc_file_from_path(pairs[1].src_path);
	iov[1].is_creation = true;
	iov[1].offset = 0;
	iov[1].length = N;
	iov[1].data = getRandomBytes(N);
	EXPECT_TRUE(iov[1].data);
	tcres = tc_writev(iov, 2, false);
	EXPECT_TRUE(tcres.okay);

	// create empty dest files
	iov[0].file = tc_file_from_path(pairs[0].dst_path);
	iov[0].is_creation = true;
	iov[0].offset = 0;
	iov[0].length = 0;
	iov[0].data = NULL;
	iov[1].file = tc_file_from_path(pairs[1].dst_path);
	iov[1].is_creation = true;
	iov[1].offset = 0;
	iov[1].length = 0;
	iov[1].data = NULL;
	tcres = tc_writev(iov, 2, false);
	EXPECT_TRUE(tcres.okay);

	// copy files
	tcres = tc_copyv(pairs, 2, false);
	EXPECT_TRUE(tcres.okay);

	read_iov[0].file = tc_file_from_path(pairs[0].dst_path);
	read_iov[0].is_creation = false;
	read_iov[0].offset = 0;
	read_iov[0].length = N;
	read_iov[0].data = malloc(N);
	EXPECT_TRUE(read_iov[0].data);
	read_iov[1].file = tc_file_from_path(pairs[1].dst_path);
	read_iov[1].is_creation = false;
	read_iov[1].offset = 0;
	read_iov[1].length = N;
	read_iov[1].data = malloc(N);
	EXPECT_TRUE(read_iov[1].data);

	tcres = tc_readv(read_iov, 2, false);
	EXPECT_TRUE(tcres.okay);

	compare_content(iov, read_iov, 2);

	free(iov[0].data);
	free(iov[1].data);
	free(read_iov[0].data);
	free(read_iov[1].data);
}

REGISTER_TYPED_TEST_CASE_P(TcTest,
			   WritevCanCreateFiles,
			   TestFileDesc,
			   AttrsTestPath,
			   AttrsTestFileDesc,
			   ListDirContents,
			   RenameFile,
			   RemoveFileTest,
			   MakeDirectory,
			   Append,
			   SuccesiveReads,
			   SuccesiveWrites,
			   CopyFiles);

//typedef ::testing::Types<TcPosixImpl, TcNFS4Impl> TcImpls;
typedef ::testing::Types<TcPosixImpl> TcImpls;
INSTANTIATE_TYPED_TEST_CASE_P(TC, TcTest, TcImpls);
