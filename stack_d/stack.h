struct Stack
{
    pthread_mutex_t mutex;
    void **content;
    int size;
    int n;
};
typedef struct Stack Stack;

Stack *newStack(int n_elemn);

int nelem(Stack *s);

int isempty(Stack *s);

void push(Stack *s, void *elem);

void *pop(Stack *s);

void freeStack(Stack *s);