#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

char usage[] = "tc_test Help\n"
	       "============\n\n"	
	       "Usage example:\n"
	       "./tc_test -b 2048 -n 10000 -m 100000 -c 5 -p \"/mnt/test/abcd\" -r \n"
	       "-b <Block_size per read/write>\n"
	       "-l <Path of the input files>\n"
	       "-n <Num of files of the form path0, path1, path2, ....>\n"
	       "-m <Total num of reads>\n"
	       "-c <Num of reads/writes per compound>\n"
	       "-r <Read>\n"
	       "-w <Write>\n"
               "-z <dist>\n"
	       "Only one of -r,-w can be specifed\n";

int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	int fd;
	char *temp_path = NULL;
	char *input_path = NULL;
	int input_len = 0;
	double dist = 0.0;
	char *data_buf = NULL;
	unsigned int block_size = 0; /* int because the max block size is 16k */
	unsigned int num_files = 0;
	unsigned int ops_per_comp = 0;
	unsigned int file_size = 0;
	unsigned int rw = 0;
	unsigned int *op_array = NULL;
	unsigned int *temp_array = NULL;
	unsigned int j = 0;
	int opt;
	clock_t t;
	struct timeval tv1, tv2;
	double time_taken;
	srand (time(NULL));

	while ((opt = getopt(argc, argv, "b:n:m:c:l:z:hrw")) != -1) {
		switch (opt) {
		case 'b':
			/* Block size per read/write */

			block_size = atoi((char *)optarg);

			if (block_size <= 0 || block_size > 32 * 1024) {
				printf(
				    "Invalid block size or it exceeds 32k\n");
				exit(-1);
			}

			break;
		case 'n':
			/*
			 * Total number of files, each file of the form
			 * /mnt/test/abcd0, /mnt/test/abcd1, .....
			 * if the path is specified as "/mnt/test/abcd"
			 */

			num_files = atoi((char *)optarg);

			if (num_files <= 0 || num_files > 10000) {
				printf(
				    "Number of files exceeds 10000 or invalid\n");
				exit(-1);
			}

			break;
		case 'm':
			/*
			 * Total number of reads/writes
			 */

			file_size = atoi((char *)optarg);

			if (file_size <= 0) {
				printf("Invalid total number of reads/writes "
				       "or it exceeds 10000\n");
				exit(-1);
			}

			break;
		case 'c':
			/*
			 * Number of operations in a single compound, should not
			 * matter in normal reads
			 */

			ops_per_comp = atoi((char *)optarg);

			if (ops_per_comp <= 0 || ops_per_comp > 10) {
				printf(
				    "Invalid ops per comp or it exceeds 10\n");
				exit(-1);
			}

			break;
		case 'l':
			/*
			 * Path of the files
			 *
			 * If the files are of the form -
			 * /mnt/test/abcd0, /mnt/test/abcd1, ......
			 * Path should be "/mnt/test/abcd"
			 */

			input_path = (char *)optarg;
			input_len = strlen(input_path);
			if (input_len <= 0 || input_len > 50) {
				printf("Name is invalid or is too long, max 50 chars \n");
				exit(-1);
			}

			break;
		case 'z':
			/*
 			 * Distribution parameter
 			 */

			dist = atof((char *)optarg);

			if (dist < 0.0 || dist > 1.0) {
				printf("Invalid distribution parameter "
				       "specified\n");
				exit(-1);
			}
			break;
		case 'r':
			/* Read */

			rw = 0;

			break;
		case 'w':
			/* Write */

			rw = 1;

			break;
		case 'h':
		default:
			printf("%s", usage);
			exit(-1);
		}
	}

	if (argc != 14) {
		printf("Wrong usage, use -h to get help\n");
		exit(-1);
	}

	if (rw == 0) {
		printf("TC_TEST - READ\n");
	} else {
		printf("TC_TEST - WRITE\n");
	}
	printf("Block size - %d\n", block_size);
	printf("Path - %s\n", input_path);
	printf("Num_files - %d\n", num_files);
	printf("Size of the file - %d\n", file_size);
	printf("Num of ops in a comp - %d\n", ops_per_comp);
	printf("Distribution parameter - %f\n", dist);

	temp_path = malloc(input_len + 8);
	data_buf = malloc(block_size);
	j = 0;

	printf("Op_array allocated\n");

	j = 0;

	// t = clock();
	gettimeofday(&tv1, NULL);

	snprintf(temp_path, input_len + 8, "%s%u", input_path, 0);

	if (rw == 0) { /* Read */
		//fp = fopen(temp_path, "r");
		fd = open(temp_path, O_RDONLY);
	} else { /* Write */
		//fp = fopen(temp_path, "w");
		fd = open(temp_path, O_WRONLY);
	}

	//if (fp == NULL) {
	if (fd < 0) {
		printf("Error opening file - %s\n", temp_path);
		goto main_exit;
	}

	j = 0;
	while (j <= (file_size / block_size)) {

		//fseek(fp, file_size - j * block_size, SEEK_SET);
		posix_fadvise(fd, j * block_size, block_size,
			      POSIX_FADV_RANDOM);
		if (rw == 0) { /* Read */
			//fread(data_buf, block_size, 1, (FILE *)fp);
			read(fd, data_buf, block_size);
		} else { /* Write */
			//fwrite(data_buf, 1, block_size, (FILE *)fp);
			write(fd, data_buf, block_size);
		}

		j++;
	}

	//fclose(fp);
	close(fd);

	//t = clock() - t;
	gettimeofday(&tv2, NULL);
	// time_taken = ((double)t) / CLOCKS_PER_SEC;
	time_taken = ((double)(tv2.tv_usec - tv1.tv_usec) / 1000000) +
		     (double)(tv2.tv_sec - tv1.tv_sec);

	printf("Took %f seconds to execute \n", time_taken);

main_exit:
	free(data_buf);
        free(temp_path);

	return 0;
}
