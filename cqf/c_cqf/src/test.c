/*
 * ============================================================================
 *
 *        Authors:  Prashant Pandey <ppandey@cs.stonybrook.edu>
 *                  Rob Johnson <robj@vmware.com>   
 *
 * ============================================================================
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/rand.h>

#include "include/gqf.h"
#include "include/gqf_int.h"
#include "include/gqf_file.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Please specify the log of the number of slots in the CQF.\n");
		exit(1);
	}
	QF qf;
	uint64_t qbits = atoi(argv[1]);
	uint64_t nhashbits = qbits + 8;
	uint64_t nslots = (1ULL << qbits);
	uint64_t nvals = 750*nslots/1000;
	uint64_t key_count = 1000;
	uint64_t *vals;

	/* Initialise the CQF */
	if (!qf_malloc(&qf, nslots, nhashbits, 0, QF_HASH_INVERTIBLE, 0)) {
		fprintf(stderr, "Can't allocate CQF.\n");
		abort();
	}

	qf_set_auto_resize(&qf, true);

	/* Generate random values */
	vals = (uint64_t*)malloc(nvals*sizeof(vals[0]));
	RAND_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
	for (uint64_t i = 0; i < nvals; i++) {
		vals[i] = (1 * vals[i]) % qf.metadata->range;
	}

	/* Insert keys in the CQF */
	for (uint64_t i = 0; i < nvals; i++) {
		int ret = qf_insert(&qf, vals[i], 0, key_count, QF_NO_LOCK);
		if (ret < 0) {
			fprintf(stderr, "failed insertion for key: %lx %d.\n", vals[i], 50);
			if (ret == QF_NO_SPACE)
				fprintf(stderr, "CQF is full.\n");
			else if (ret == QF_COULDNT_LOCK)
				fprintf(stderr, "TRY_ONCE_LOCK failed.\n");
			else
				fprintf(stderr, "Does not recognise return value.\n");
			abort();
		}
	}

	/* Lookup inserted keys and counts. */
	for (uint64_t i = 0; i < nvals; i++) {
		uint64_t count = qf_count_key_value(&qf, vals[i], 0, 0);
		if (count < key_count) {
			fprintf(stderr, "failed lookup after insertion for %lx %ld.\n", vals[i],
							count);
			abort();
		}
	}

	/* Write the CQF to disk and read it back. */
	char filename[] = "mycqf.cqf";
	fprintf(stdout, "Serializing the CQF to disk.\n");
	uint64_t total_size = qf_serialize(&qf, filename);
	if (total_size < sizeof(qfmetadata) + qf.metadata->total_size_in_bytes) {
		fprintf(stderr, "CQF serialization failed.\n");
		abort();
	}

	QF file_qf;
	fprintf(stdout, "Reading the CQF from disk.\n");
	if (!qf_usefile(&file_qf, filename)) {
		fprintf(stderr, "Can't initialize the CQF from file: %s.\n", filename);
		abort();
	}
	for (uint64_t i = 0; i < nvals; i++) {
		uint64_t count = qf_count_key_value(&file_qf, vals[i], 0, 0);
		if (count < key_count) {
			fprintf(stderr, "failed lookup in file based CQF for %lx %ld.\n",
							vals[i], count);
			abort();
		}
	}

	fprintf(stdout, "Testing iterator and unique indexes.\n");
	/* Initialize an iterator and validate counts. */
	QFi qfi;
	qf_iterator_from_position(&qf, &qfi, 0);
	QF unique_idx;
	if (!qf_malloc(&unique_idx, nslots, nhashbits, 0, QF_HASH_INVERTIBLE, 0)) {
		fprintf(stderr, "Can't allocate set.\n");
		abort();
	}
	do {
		uint64_t key, value, count;
		qfi_get_key(&qfi, &key, &value, &count);
		qfi_next(&qfi);
		if (count < key_count) {
			fprintf(stderr, "Failed lookup during iteration for: %lx. Returned count: %ld\n",
							key, count);
			abort();
		}
		int64_t idx = qf_get_unique_index(&qf, key, value, 0);
		if (idx == QF_DOESNT_EXIST) {
			fprintf(stderr, "Failed lookup for unique index for: %lx. index: %ld\n",
							key, idx);
			abort();
		}
		if (qf_count_key_value(&unique_idx, idx, 0, 0) > 0) {
			fprintf(stderr, "Failed unique index for: %lx. index: %ld\n",
							key, idx);
			abort();
		} else {
			qf_insert(&unique_idx, idx, 0, 1, QF_NO_LOCK);
		}
	} while(!qfi_end(&qfi));

	/* remove some counts  (or keys) and validate. */
	fprintf(stdout, "Testing remove/delete_key.\n");
	srand(time(NULL));
	for (uint64_t i = 0; i < 100; i++) {
		uint64_t idx = rand()%nvals;
		int ret = qf_delete_key_value(&file_qf, vals[idx], 0, QF_NO_LOCK);
		uint64_t count = qf_count_key_value(&file_qf, vals[idx], 0, 0);
		if (count > 0) {
			if (ret < 0) {
				fprintf(stderr, "failed deletion for %lx %ld ret code: %d.\n",
								vals[idx], count, ret);
				abort();
			}
			uint64_t new_count = qf_count_key_value(&file_qf, vals[idx], 0, 0);
			if (new_count > 0) {
				fprintf(stderr, "delete key failed for %lx %ld new count: %ld.\n",
								vals[idx], count, new_count);
				abort();
			}
		}
	}

	fprintf(stdout, "Validated the CQF.\n");
}

