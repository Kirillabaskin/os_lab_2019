#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // your code here
            // error handling
            break;
          case 1:
            array_size = atoi(optarg);
            // your code here
            // error handling
            break;
          case 2:
            pnum = atoi(optarg);
            // your code here
            // error handling
            break;
          case 3:
            with_files = true;
            break;

          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  //
  int rt;
  int pipeid[4];
  int pipeFlag=pipe(pipeid);
  pipe(pipeid+2);
  //
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        int buf;
        if(i%2==0){
            buf=INT_MIN;
            for(int j=0;j<array_size;j++)
            {
                if(buf<array[j])
                    buf=array[j];
            }
        }
        else{
            buf=INT_MAX;
            for(int j=0;j<array_size;j++)
            {
                if(buf>array[j])
                    buf=array[j];
            }
        }
        // parallel somehow

        if (with_files) {
          // use files here
          if(i%2==0){
              FILE *fp=fopen("MaxFile.txt","w");
              fprintf(fp,"%d",buf);
              fclose(fp);
          }
          else{
              FILE *fp=fopen("MinFile.txt","w");
              fprintf(fp,"%d",buf);
              fclose(fp);
          }
        } else {
          // use pipe here
          if(pipeFlag!=-1){
              if(i%2==0){
                  close(pipeid[0]);
                  write(pipeid[1],&buf,sizeof(buf));
                  //printf("max w %d\n",buf);
                  close(pipeid[1]);
              }
              else{
                  close(pipeid[2]);
                  write(pipeid[3],&buf,sizeof(buf));
                 // printf("min w %d\n",buf);
                  close(pipeid[3]);
              }
          }else{
              printf("Pipe dont work");
              return -1;
          }
          //
        }
        exit(rt);
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  while (active_child_processes > 0) {
    // your code here
    wait(0);
    //
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    if (with_files) {
      //read from files
      if(i%2==0){
              FILE *fp=fopen("MaxFile.txt","r");
             // printf("%d\n",max);
              fscanf(fp,"%d",&max);
              fclose(fp);
          }
          else{
              FILE *fp=fopen("MinFile.txt","r");
             // printf("%d\n",min);
              fscanf(fp,"%d",&min);
              fclose(fp);
          }
      //
    } else {
      // read from pipes
      if(i%2==0){
        close(pipeid[1]);
        read(pipeid[0],&max,sizeof(int));
       // printf("max r %d\n",max);
        close(pipeid[0]);
      }
      else{
        close(pipeid[3]);
        read(pipeid[2],&min,sizeof(int));
        //printf("min r %d\n",min);
        close(pipeid[2]);
      }
      //
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
