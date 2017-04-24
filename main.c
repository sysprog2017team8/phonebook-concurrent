#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include IMPL

#ifndef OPT
#define OUTPUT_FILE "orig.txt"

#else
#define OUTPUT_FILE "opt.txt"

#endif

#define DICT_FILE "./dictionary/words.txt"

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

struct timespec start, end;

void Stopwatch_start()
{
    clock_gettime(CLOCK_REALTIME, &start);
}

double Stopwatch_end(char *name)
{
    clock_gettime(CLOCK_REALTIME, &end);
    double tmp = diff_in_second(start, end);
    printf("execution time of %s : %lf sec\n", name,tmp);
    return tmp;
}

struct __Stopwatch_API {
    void (*start)();
    double (*end)(char *);
} Stopwatch = {
    .start = Stopwatch_start,
    .end = Stopwatch_end,
};


int main(int argc, char *argv[])
{
    /* Write the execution time to file. */
    FILE *output;
    output = fopen(OUTPUT_FILE, "a");

    entry *pHead;
    printf("size of entry : %lu bytes\n", sizeof(entry));

    /* Start timing */
    Stopwatch.start();

    Phonebook.create();

    /* Stop timing */
    fprintf(output,"%lf ",Stopwatch.end("Phonebook.create()"));

    /* Start timing */
    Stopwatch.start();

    /*append lastname with DICT_FILE*/
    pHead = Phonebook.appendByFile(DICT_FILE);

    /* Stop timing */
    fprintf(output,"%lf ",Stopwatch.end("Phonebook.appendByFile()"));

    /* Find the given entry */
    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";

    assert(Phonebook.findName(input) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(Phonebook.findName(input)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif

    /* Start timing */
    Stopwatch.start();

    Phonebook.findName(input);

    /* Stop timing */
    fprintf(output,"%lf ",Stopwatch.end("Phonebook.findName()"));

    /*Start timing */
    Stopwatch.start();

    Phonebook.removeByFile(DICT_FILE);
    /*	Phonebook.remove("aaaa");
    printf("cayon1\n");

    	Phonebook.remove("zzzzzz");
    printf("cayon2\n");
    	Phonebook.remove("tota");
    printf("cayon3\n");
    	Phonebook.remove("totaal");
    */

    /*Stop timing */
    fprintf(output,"%lf ",Stopwatch.end("Phonebook.removeByFile()"));

    /* Start timing */
    Stopwatch.start();

    /* Release memory */
    Phonebook.free();

    /* Stop timing */
    fprintf(output,"%lf\n",Stopwatch.end("Phonebook.free()"));

    fclose(output);

    return 0;
}
