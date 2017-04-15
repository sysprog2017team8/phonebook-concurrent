#ifndef _PHONEBOOK_H
#define _PHONEBOOK_H

#include <pthread.h>
#include <time.h>

#define MAX_LAST_NAME_SIZE 16

#define OPT 1

typedef struct _detail {
    char firstName[16];
    char email[16];
    char phone[10];
    char cell[10];
    char addr1[16];
    char addr2[16];
    char city[16];
    char state[2];
    char zip[5];
} detail;

typedef detail *pdetail;

typedef struct __PHONE_BOOK_ENTRY {
    char *lastName;
    struct __PHONE_BOOK_ENTRY *pNext;
    pdetail dtl;
} entry;

typedef struct _thread_argument {
    char *data_begin;
    char *data_end;
    int threadID;
    int numOfThread;
    entry *lEntryPool_begin;    /* The local entry pool */
    entry *lEntry_head;	/* local entry linked list */
    entry *lEntry_tail;	/* local entry linked list */
} thread_arg;

/* API */
extern struct __PHONEBOOK_API {
    void (*create)();
    entry *(*appendByFile)(char *fileName);
    entry *(*append)(char *lastName);
    entry *(*removeByFile)(char *fileName);
    entry *(*remove)(char *lastName);
    entry *(*findName)(char *lastName);
    void (*free)();
} Phonebook;

static double diff_in_second(struct timespec t1, struct timespec t2);

#endif
