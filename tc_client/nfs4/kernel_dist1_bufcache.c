#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
	       "Only one of -r,-w can be specifed\n";

double randn(double mu, double sigma)
{
	double U1, U2, W, mult;
	static double X1, X2;
	static int call = 0;

	if (call == 1) {
		call = !call;
		return (mu + sigma * (double)X2);
	}

	do {
		U1 = -1 + ((double)rand() / RAND_MAX) * 2;
		U2 = -1 + ((double)rand() / RAND_MAX) * 2;
		W = pow(U1, 2) + pow(U2, 2);
	} while (W >= 1 || W == 0);

	mult = sqrt((-2 * log(W)) / W);
	X1 = U1 * mult;
	X2 = U2 * mult;

	call = !call;

	return (mu + sigma * (double)X1);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	char *temp_path = NULL;
	char *input_path = NULL;
	int input_len = 0;
	char *data_buf = NULL;
	unsigned int block_size = 0; /* int because the max block size is 16k */
	unsigned int num_files = 0;
	unsigned int ops_per_comp = 0;
	unsigned int num_ops = 0;
	unsigned int rw = 0;
	unsigned int *op_array = NULL;
	unsigned int *temp_array = NULL;
	unsigned int j = 0;
	int opt;
	clock_t t;
	struct timeval tv1, tv2;
	double time_taken;
	srand (time(NULL));

	while ((opt = getopt(argc, argv, "b:n:m:c:l:hrw")) != -1) {
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

			num_ops = atoi((char *)optarg);

			if (num_ops <= 0 || num_ops > 10000) {
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

	if (argc != 12) {
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
	printf("Total num of reads/writes - %d\n", num_ops);
	printf("Num of ops in a comp - %d\n", ops_per_comp);

	temp_path = malloc(input_len + 8);
	data_buf = malloc(block_size);
	j = 0;

	op_array = malloc(num_files * sizeof(double));

	if (op_array == NULL) {
		printf("Cannot allocate memory for op_array\n");
		goto main_exit;
	}

	while (j < num_ops) {
		temp_array = op_array + j;
		*temp_array = (unsigned int)randn(num_ops / 2, num_ops / 8);
		j++;
	}

	printf("Op_array allocated\n");

	j = 0;

	// t = clock();
	gettimeofday(&tv1, NULL);
	while (j < num_ops) {

		temp_array = op_array + j;

		snprintf(temp_path, input_len + 8, "%s%u", input_path, *temp_array);

		if (rw == 0) { /* Read */
			fp = fopen(temp_path, "r");
		} else { /* Write */
			fp = fopen(temp_path, "w");
		}

		if (fp == NULL) {
			printf("Error opening file - %s\n", temp_path);
			goto exit;
		}

		if (rw == 0) { /* Read */
			fread(data_buf, block_size, 1, (FILE *)fp);
		} else { /* Write */
			fwrite(data_buf, 1, block_size, (FILE *)fp);
		}

		fclose(fp);
                j++;
        }

	//t = clock() - t;
	gettimeofday(&tv2, NULL);
	// time_taken = ((double)t) / CLOCKS_PER_SEC;
	time_taken = ((double)(tv2.tv_usec - tv1.tv_usec) / 1000000) +
		     (double)(tv2.tv_sec - tv1.tv_sec);

	printf("Took %f seconds to execute \n", time_taken);

exit:
	free(op_array);
main_exit:
	free(data_buf);
        free(temp_path);

	return 0;
}
