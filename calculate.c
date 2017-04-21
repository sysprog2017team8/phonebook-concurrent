#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EVENT_NUM 10

int main(int argc,char *argv[])
{
    if (argc < 4) {
        printf("need input [format] [intput file]* [output file]\n");
        exit(0);
    }
    int event_count = 0;
    char event[EVENT_NUM][50];

    char *tmp = argv[1];

    for (int i=0; i<=strlen(tmp); i++) {
        if (tmp[i] == '\0' && i!=0) {
            strcpy(event[event_count++],tmp);
            break;
        }
        if (tmp[i] == '\n' || tmp[i]==' ') {
            tmp[i] = '\0';
            strcpy(event[event_count++],tmp);
            tmp = tmp + i + 1;
            i = -1;
        }
    }

    int sample_count = argc - 3;

    FILE *output = fopen(argv[argc-1],"w");
    if (!output) {
        printf("ERROR opening output file %s\n",argv[argc-1]);
        exit(0);
    }


    double sample,average[EVENT_NUM][sample_count];
    FILE *fp;
    char ch;

    for (int i=0; i<sample_count; i++) {
        fp = fopen(argv[2+i],"r");
        if (!fp) {
            printf("ERROR opening input file %s\n",argv[2+i]);
            exit(0);
        }
        for (int j=0; j<event_count; j++)
            average[j][i] = 0.0;


        for (int k=0; k < 100; k++) {
            if (feof(fp)) {
                printf("ERROR: You need 100 datum instead of %d\n", i);
                printf("run 'make run' longer to get enough information\n\n");
                exit(0);
            }
            for (int j=0; j<event_count; j++) {
                fscanf(fp, "%lf",&sample);
                average[j][i] += sample;
            }
            while (fscanf(fp,"%c",&ch)) {
                if (ch=='\n')
                    break;
            }
        }


        for (int j=0; j<event_count; j++) {
            average[j][i] /= 100;
        }
        fclose(fp);
    }

    /* write to output file */

    for (int i=0; i<event_count; i++) {
        fprintf(output,"%s",event[i]);
        for (int j=0; j<sample_count; j++) {
            fprintf(output," %lf",average[i][j]);
        }
        fprintf(output,"\n");
    }
    fclose(output);

    return 0;
}
