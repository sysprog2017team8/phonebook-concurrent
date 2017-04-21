#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "phonebook_lockfree.h"
#include "debug.h"
#include "text_align.h"

#define ALIGN_FILE "align.txt"

#ifndef THREAD_NUM
#define THREAD_NUM 4
#endif

static entry *entryHead,*entry_pool,*entrytail,*tmpHead;
static pthread_t threads[THREAD_NUM];
static thread_arg *thread_args[THREAD_NUM];
static char *map;
static int data_number;
static int *space_used; //0 for not used, 1 for used, -1 for deleted
static off_t file_size;

static inline
int is_marked_ref(long i)
{
    return (int)(i & 0x1L);
}

static inline
long get_unmarked_ref(long i)
{
    return i & ~0x1L;
}

static inline
long get_marked_ref(long i)
{
    return i | 0x1L;
}

static entry *search(char *str, entry **left_entry)
{
    int i;
    int len = strlen(str);
    entry *j,*j_next,*h;
    entry *left_next, *right;
    left_next = right = NULL;
    while(1) {
        h = j = tmpHead;
        j_next = j->pNext;
        while( is_marked_ref(j_next) || strncasecmp(str,j_next->lastName,len)!=0 ) {
            if(!is_marked_ref(j_next)) {
                __sync_val_compare_and_swap(&tmpHead,h,j);
                (*left_entry) = j;
                left_next = j_next;
            }

            j = get_unmarked_ref(j_next);
            if(j_next == entrytail)
                break;
            j_next = j_next->pNext;
        }

        right = j;

        if(left_next == right) {
            if(!is_marked_ref(right->pNext))
                return right;
        } else if(__sync_val_compare_and_swap(&((*left_entry)->pNext), left_next, right) == left_next ) {
            if(!is_marked_ref(right->pNext)) {
                return right;
            }
        }

    }
}

static entry *findName(char lastname[], entry *pHead)
{
    size_t len = strlen(lastname);
    pHead = entryHead;
    while (pHead) {
        if (strncasecmp(lastname, pHead->lastName, len) == 0
                && (pHead->lastName[len] == '\n' ||
                    pHead->lastName[len] == '\0')) {
            pHead->lastName[len] = '\0';
            if (!pHead->dtl)
                pHead->dtl = (pdetail) malloc(sizeof(detail));
            return pHead;
        }
        DEBUG_LOG("find string = %s\n", pHead->lastName);
        pHead = pHead->pNext;
    }
    return NULL;
}

static thread_arg *createThread_arg(char *data_begin, char *data_end,
                                    int threadID, int numOfThread,
                                    entry *entryPool)
{
    thread_arg *new_arg = (thread_arg *) malloc(sizeof(thread_arg));

    new_arg->data_begin = data_begin;
    new_arg->data_end = data_end;
    new_arg->threadID = threadID;
    new_arg->numOfThread = numOfThread;
    new_arg->lEntryPool_begin = entryPool;
    new_arg->lEntry_head = new_arg->lEntry_tail = entryPool;
    return new_arg;
}

/**
 * Generate a local linked list in thread.
 */
static void append(void *arg)
{
    struct timespec start, end;
    double cpu_time;

    clock_gettime(CLOCK_REALTIME, &start);

    thread_arg *t_arg = (thread_arg *) arg;

    int count = 0, len;
    entry *left, *right;

    entry *j = t_arg->lEntryPool_begin;

    for (char *i = t_arg->data_begin; i < t_arg->data_end;
            i += MAX_LAST_NAME_SIZE * t_arg->numOfThread,
            j += t_arg->numOfThread, count++) {

        left = right = NULL;
        j->lastName = i;
        j->pNext = NULL;
        len = strlen(i);

        /* Append the new at the end of the local linked list */
        while(1) {
            right = search(i, &left);
            if(right != NULL && strncasecmp(right->lastName,i,len) == 0)
                break;
            j->pNext = right;
            if(__sync_val_compare_and_swap(&(left->pNext), right, j) == right)
                break;
        }

        DEBUG_LOG("thread %d t_argend string = %s\n",
                  t_arg->threadID, new -> lastName);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time = diff_in_second(start, end);

    DEBUG_LOG("thread take %lf sec, count %d\n", cpu_time, count);

    pthread_exit(NULL);
}

static void show_entry(entry *pHead)
{
    while (pHead) {
        printf("%s", pHead->lastName);
        pHead = pHead->pNext;
    }
}

static void phonebook_create()
{
}

static entry *phonebook_appendByFile(char *fileName)
{
    text_align(fileName, ALIGN_FILE, MAX_LAST_NAME_SIZE);
    int fd = open(ALIGN_FILE, O_RDONLY | O_NONBLOCK);
    file_size = fsize(ALIGN_FILE);
    /* Allocate the resource at first */
    map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    assert(map && "mmap error");

    data_number = file_size / MAX_LAST_NAME_SIZE;
    entry_pool = (entry *) malloc(sizeof(entry) * data_number);
    space_used = (int *) malloc(sizeof(int) * data_number);
    assert(entry_pool && "entry_pool error");

    entryHead = malloc(sizeof(entry));
    entrytail = malloc(sizeof(entry));
    entryHead -> lastName = "TOTALYNOTIMPOSSIBLENAME";
    entryHead -> pNext = entrytail;
    entrytail -> lastName = "TOTALYNOTIMPOSSIBLENAME";
    entrytail -> pNext = NULL;
    tmpHead = entryHead;

    /* Prepare for mutli-threading */
    pthread_setconcurrency(THREAD_NUM + 1);
    for (int i = 0; i < THREAD_NUM; i++)
        thread_args[i] = createThread_arg(map + MAX_LAST_NAME_SIZE * i,map + file_size, i,THREAD_NUM, entry_pool + i);

    /* Deliver the jobs to all thread and wait for completing  */

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_create(&threads[i], NULL, (void *)&append, (void *)thread_args[i]);


    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(threads[i], NULL);

    long long cs=0;

    for( entry * j = entryHead; j!=entrytail; j=j->pNext)
        ++cs;
    printf("Entries of link list: %lld\n",cs-1);

    close(fd);
    pthread_setconcurrency(0);
    /* Return head of linked list */
    return entryHead;
}

static entry *phonebook_findName(char *lastName)
{
    return findName(lastName, entryHead);
}

static void phonebook_free()
{
    entry *e = entryHead;
    while (e) {
        free(e->dtl);
        e = e->pNext;
    }

    free(entry_pool);
    for (int i = 0; i < THREAD_NUM; i++)
        free(thread_args[i]);

    munmap(map, file_size);
}

static int phonebook_remove(char *lastName)
{
}

/* API */
struct __PHONEBOOK_API Phonebook = {
    .create = phonebook_create,
    .appendByFile = phonebook_appendByFile,
    .findName = phonebook_findName,
    .remove = phonebook_remove,
    .free = phonebook_free,
};

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}
