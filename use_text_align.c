#include <stdio.h>
#include <stdlib.h>
#include "text_align.h"
#include <string.h>


int main(int argc,char *argv[])
{
    if (argc < 4)
        printf("get input file and output file\n");
    else
        text_align(argv[1], argv[2], atoi(argv[3]));

    return 0;
}
