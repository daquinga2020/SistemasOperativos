#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include <err.h>

Stack *
newStack(int n_elemn)
{
	Stack *s = malloc(sizeof(Stack));

	s->content = (void *)malloc(sizeof(void *) * n_elemn);
	s->size = n_elemn;
	s->n = 0;

	if (s == NULL) {
		err(EXIT_FAILURE, "error");
	}
	if (s->content == NULL) {
		err(EXIT_FAILURE, "error");
	}

	if (pthread_mutex_init(&s->mutex, NULL) != 0) {
		warnx("can't init mutex");
		return NULL;
	}

	return s;
}

int
nelem(Stack * s)
{
	int n;

	pthread_mutex_lock(&s->mutex);
	n = s->n;
	pthread_mutex_unlock(&s->mutex);

	return n;
}

int
isempty(Stack * s)
{
	int empty;

	pthread_mutex_lock(&s->mutex);
	empty = s->n == 0;
	pthread_mutex_unlock(&s->mutex);

	return empty;
}

static int
isempty_local(Stack * s)
{
	return s->n == 0;
}

void
push(Stack * s, void *elem)
{
	pthread_mutex_lock(&s->mutex);

	if (s->size == s->n) {
		s->size = s->size * 2;
		s->content =
		    (void *)realloc((void *)s->content,
				    sizeof(void *) * s->size);
		if (s->content == NULL) {
			pthread_mutex_unlock(&s->mutex);
			errx(EXIT_FAILURE, "error: realloc failed");
		}
	}
	s->content[s->n] = elem;
	s->n++;
	pthread_mutex_unlock(&s->mutex);
}

void *
pop(Stack * s)
{
	void *elemn;

	pthread_mutex_lock(&s->mutex);

	if (isempty_local(s)) {
		pthread_mutex_unlock(&s->mutex);
		return NULL;
	}

	s->n--;
	elemn = (void *)s->content[s->n];

	pthread_mutex_unlock(&s->mutex);

	return elemn;
}

void
freeStack(Stack * s)
{
	pthread_mutex_destroy(&s->mutex);
	free(s->content);
	free(s);
}
