#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include "stack.h"

enum {
	NTHREADS = 100,
	NVALUES = 100,
	NEXTRACT = 40
};

struct Valor {
	int v;
	int id;
};
typedef struct Valor Valor;

static Stack *s;

void *
do_work(void *id)
{
	int i;
	int count = 0;
	int *p_id = id;

	Valor *sv_value;
	Valor *value;

	for (i = 0; i < NVALUES; i++) {
		value = malloc(sizeof(Valor));
		if (value == NULL) {
			err(EXIT_FAILURE, "malloc failed");
		}

		value->id = (*p_id);
		value->v = i;
		push(s, value);
	}

	for (i = 0; i < NEXTRACT; i++) {
		sv_value = (Valor *) pop(s);
		if (sv_value->id != (*p_id)) {
			count++;
		}
		free(sv_value);
	}
	printf("Stack ID: %d\tCount: %d\n", (*p_id), count);
	return NULL;
}

int
main(int argc, char *argv[])
{
	pthread_t threads[NTHREADS];
	int i;
	int bad_order = 0;
	int n_expected = (NVALUES - NEXTRACT) * NTHREADS;
	int values[NTHREADS];
	int ids[NTHREADS];
	Valor *sv_value;

	s = newStack(NVALUES);

	for (i = 0; i < NTHREADS; i++) {
		values[i] = -1;
		ids[i] = i;
		if (pthread_create(&threads[i], NULL, do_work, &ids[i]) != 0) {
			warnx("error creating thread");
			return 1;
		}
	}

	for (i = 0; i < NTHREADS; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			warnx("error joining thread");
			return 1;
		}
	}

	printf
	    ("FINAL ELEMENTS OF STACK: %d\tEXPECTED ELEMENTS: %d COMPROBATION: %d\n",
	     nelem(s), n_expected, s->n == n_expected);

	for (i = 0; !isempty(s); i++) {
		sv_value = (Valor *) pop(s);

		if (values[sv_value->id] == -1) {
			values[sv_value->id] = sv_value->v;
		}

		if (values[sv_value->id] < sv_value->v) {
			fprintf(stderr,
				"VALUES WITH ID: %d\t VALUE 1: %d AND VALUE 2: %d DISORGANIZED\n",
				sv_value->id, values[sv_value->id],
				sv_value->v);
			bad_order = 1;
		}
		values[sv_value->id] = sv_value->v;

		free(sv_value);
	}

	if (bad_order) {
		fprintf(stderr, "BAD ORDER IN THE STACK\n");
	} else {
		printf("STACK IS CORRECT\n");
	}

	freeStack(s);
	return 0;
}
