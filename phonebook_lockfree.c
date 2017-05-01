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

#define ALIGN_APPEND_FILE "align_append.txt"
#define ALIGN_REMOVE_FILE "align_remove.txt"

#ifndef THREAD_NUM
#define THREAD_NUM 4
#endif

#define REMOVE_NUM 100

static entry *entryHead,*entry_pool,*entrytail;
static pthread_t threads[THREAD_NUM];
static thread_arg *thread_args[THREAD_NUM],*thread_args_pool;
static char *map;
static int data_number;
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

int xx = 0;

static entry *search(char *str, entry **left_entry, entry *Start)
{
    int i;
    int len = strlen(str);
    entry *j,*j_next,*h;
    entry *left_next, *right;
    left_next = right = NULL;
    while(1) {
        j = Start;
        j_next = j->pNext;
        while(1) {
            while( is_marked_ref(j_next) || strncasecmp(str,j->lastName,len)!=0 ) {
                if(!is_marked_ref(j_next)) {
                    (*left_entry) = j;
                    left_next = j_next;
                }

                j = get_unmarked_ref(j_next);
                if(j == entrytail)
                    break;
                j_next = j->pNext;
            }

            if(j == entrytail) break;

            if(strlen(j->lastName) == len)
                break;

            if(!is_marked_ref(j_next)) {
                (*left_entry) = j;
                left_next = j_next;
            }
            j = get_unmarked_ref(j_next);
            if(j == entrytail) break;
            j_next = j->pNext;
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
    entry *left, *right;
    left = right = NULL;
    right = search(lastname, &left, entryHead);
    return right;
}

static thread_arg *createThread_arg(char *data_begin, char *data_end,
                                    int threadID, int numOfThread,
                                    entry *entryPool,thread_arg *argPool)
{
    argPool->data_begin = data_begin;
    argPool->data_end = data_end;
    argPool->threadID = threadID;
    argPool->numOfThread = numOfThread;
    argPool->lEntryPool_begin = entryPool;
    argPool->lEntry_head = argPool->lEntry_tail = entryPool;
    return argPool;
}

static void append(void *arg)
{
    thread_arg *t_arg = (thread_arg *) arg;

    int count = 0, len;
    entry *left, *right,*tmpHead;

    entry *j = t_arg->lEntryPool_begin;
    tmpHead = entryHead;

    for (char *i = t_arg->data_begin; i < t_arg->data_end;
            i += MAX_LAST_NAME_SIZE * t_arg->numOfThread,
            j += t_arg->numOfThread, count++) {

        left = right = NULL;
        if(i[strlen(i)-1]=='\n') i[strlen(i)-1] = '\0';
        j->lastName = i;

        while(1) {
            right = tmpHead->pNext;
            j->pNext = right;
            if(__sync_val_compare_and_swap(&(tmpHead->pNext), right, j) == right)
                break;
        }
        tmpHead = j;

        DEBUG_LOG("thread %d t_argend string = %s\n",
                  t_arg->threadID, new -> lastName);
    }

    pthread_exit(NULL);
}

static void append_on_head(void *arg)
{
    thread_arg *t_arg = (thread_arg *) arg;

    int count = 0, len;

    entry *j = t_arg->lEntryPool_begin;
    entry *Hnext;
    for (char *i = t_arg->data_begin; i < t_arg->data_end;
            i += MAX_LAST_NAME_SIZE * t_arg->numOfThread,
            j += t_arg->numOfThread, count++) {

        if(i[strlen(i)-1]=='\n') i[strlen(i)-1] = '\0';
        j->lastName = i;
        j->pNext = NULL;
        len = strlen(i);

        while(1) {
            Hnext = entryHead -> pNext;
            j->pNext = Hnext;
            if(__sync_val_compare_and_swap(&(entryHead->pNext), Hnext, j) == Hnext)
                break;
        }

        DEBUG_LOG("thread %d t_argend string = %s\n",
                  t_arg->threadID, new -> lastName);
    }

    pthread_exit(NULL);

}

static void show_entry(entry *pHead)
{
    while (pHead) {
        printf("%s", pHead->lastName);
        pHead = pHead->pNext;
    }
}

static void phonebook_size()
{
    long long cs = 0;
    entry *tmpHead = entryHead -> pNext;
    while(tmpHead != entrytail) {
        ++cs;
        if((long long)(tmpHead->pNext) %2) {
            --cs;
        }
        tmpHead = (entry *)get_unmarked_ref(tmpHead->pNext);
    }
    printf("Entries of link list: %lld\n",cs);
}

static void phonebook_create()
{
}

static entry *phonebook_appendByFile(char *fileName)
{
//	text_align("./dictionary/words.txt","./dictionary/append.txt",MAX_LAST_NAME_SIZE);

    int fd = open(fileName, O_RDONLY | O_NONBLOCK);
    file_size = fsize(fileName);

    /* Allocate the resource at first */
    map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    assert(map && "mmap error");

    data_number = file_size / MAX_LAST_NAME_SIZE;
    entry_pool = (entry *) malloc(sizeof(entry) * data_number);
    thread_args_pool = (thread_arg *) malloc(sizeof(thread_arg) * THREAD_NUM);
    assert(entry_pool && "entry_pool error");

    entryHead = malloc(sizeof(entry));
    entrytail = malloc(sizeof(entry));
    entryHead -> lastName = "TOTALYNOTIMPOSSIBLENAME_HEAD";
    entryHead -> pNext = entrytail;
    entrytail -> lastName = "TOTALYNOTIMPOSSIBLENAME_TAIL";
    entrytail -> pNext = NULL;

    /* Prepare for mutli-threading */
    pthread_setconcurrency(THREAD_NUM + 1);
    for (int i = 0; i < THREAD_NUM; i++)
        thread_args[i] = createThread_arg(map + MAX_LAST_NAME_SIZE * i,map + file_size, i,THREAD_NUM, entry_pool + i, thread_args_pool + i);

    /* Deliver the jobs to all thread and wait for completing  */

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_create(&threads[i], NULL, (void *)&append, (void *)thread_args[i]);

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(threads[i], NULL);

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
        e = get_unmarked_ref(e->pNext);
    }

    free(entry_pool);
    free(thread_args_pool);

    munmap(map, file_size);
}

static int phonebook_removeEach(char *lastName)
{
    entry *left, *right, *right_succ;
    left = right = NULL;
    int len = strlen(lastName);
    while(1) {
        right = search(lastName, &left, entryHead);
        if(right == NULL || strncasecmp(right->lastName,lastName,len) != 0) {
            return 0;
        }
        right_succ = right->pNext;
        if(!is_marked_ref(right_succ)) {
            if(__sync_val_compare_and_swap(&(right->pNext), right_succ, get_marked_ref(right_succ)) == right_succ) {
                return 1;
            }
        }
    }
}

static void phonebook_remove(void *arg)
{
    thread_arg *t_arg = (thread_arg *) arg;

    int count = 0, len;

    for (char *i = t_arg->data_begin; i < t_arg->data_end; i+= MAX_LAST_NAME_SIZE * t_arg->numOfThread, ++count) {
        if(i[strlen(i)-1]=='\n') i[strlen(i)-1] = '\0';
        phonebook_removeEach(i);
    }

    pthread_exit(NULL);
}

static int phonebook_removeByFile(char *fileName)
{
//	text_align("./dictionary/words_random.txt","./dictionary/remove.txt",MAX_LAST_NAME_SIZE);

    int fd = open(fileName, O_RDONLY | O_NONBLOCK);
    file_size = fsize(fileName);

    map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    assert(map && "mmap error");

    data_number = file_size / MAX_LAST_NAME_SIZE;
    data_number = REMOVE_NUM;

    pthread_setconcurrency(THREAD_NUM + 10);
    for(int i = 0; i < THREAD_NUM ; ++i)
        thread_args[i] = createThread_arg(map + MAX_LAST_NAME_SIZE * i, map + REMOVE_NUM * MAX_LAST_NAME_SIZE, i, THREAD_NUM, NULL, thread_args_pool + i);

    for(int i = 0; i< THREAD_NUM; ++i)
        pthread_create(&threads[i], NULL, (void *)&phonebook_remove, (void *)thread_args[i]);

    for(int i = 0; i< THREAD_NUM; ++i)
        pthread_join(threads[i], NULL);

    close(fd);
    pthread_setconcurrency(0);

    return 1;
}

/* API */
struct __PHONEBOOK_API Phonebook = {
    .create = phonebook_create,
    .appendByFile = phonebook_appendByFile,
    .removeByFile = phonebook_removeByFile,
    .findName = phonebook_findName,
    .remove = phonebook_removeEach,
    .free = phonebook_free,
    .size = phonebook_size,
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
