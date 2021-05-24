#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
     printf("tests starting!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // for (int i = 0; i < 20; i++)
    // {
    //     //printf("before one\n");
    sbrk(4096);
    //     //printf("finished one\n");
    // }
    // int pid;
    // if ((pid = fork())==0)
    // {

    //     printf("child waits\n");
    //      sbrk(4096);
    //      sbrk(-4096*12);
    //      sbrk(4096*2);
    //     sleep(10);
    //     exit(0);
    // }
    // wait(0);
    // printf("finished tests\n");
    


    //TEST 2 - fork and child allocating 28 pages
    char in[3];
int* pages[18];
  printf( "--------------------TEST 2:----------------------\n");
  printf( "-------------allocating 28 pages-----------------\n");
  if(fork() == 0){
    for(int i = 0; i < 28; i++){
        printf( "doing sbrk number %d\n", i);
        sbrk(PGSIZE);
    }
    printf( "------------child --> allocated_memory_pages: 16 paged_out: 16------------\n");
    printf( "--------for our output press CTRL^P:--------\n");
    printf("---------press enter to continue------------\n");
    gets(in,3);
    exit(0);
  }
  wait(0);
    
  //TEST 3 - father wait for child and then allocating 18 pages
  printf("---------press enter to continue------------\n");
  gets(in,3);
  printf( "--------------------TEST 3:----------------------\n");
  for(int i = 0; i < 18; i++){
    printf( "i: %d\n", i);
    pages[i] = (int*)sbrk(PGSIZE);
    *pages[i] = i;
  }
  printf( "--------father --> allocated_memory_pages: 16 paged_out: 6--------\n");
  printf( "--------for our output press CTRL^P:--------\n");
  printf("---------press enter to continue------------\n");
  gets(in,3);

  //TEST 4 - fork from father & check if child copy file & RAM data and counters
  printf( "--------------------TEST 4:----------------------\n");
  if(fork() == 0){
    for(int i = 0; i < 18; i++){
        printf( "expected: %d, our output: %d\n",i,*pages[i]);
    }
    printf( "--------------expected: allocated_memory_pages: 16 paged_out: 6--------------\n");
    exit(0);
  }
  sleep(5);
  wait(0);
  printf( "---------------press enter to continue---------------\n");
  gets(in,3);

  //TEST 4 - deleting RAM
  printf( "-----------deleting physical pages-----------\n");
  sbrk(-16*PGSIZE);
  if(fork() == 0){
    printf( "--------total pages for process is should be 6--------\n");
    printf( "--------for our output press (CTRL^P):--------\n");
    exit(0);
  }
  wait(0);
  printf( "--------------press enter to continue--------------\n");
  gets(in,3);
  
  // TEST 5 - fail to read pages[17] beacause it deleted from memory
  if(fork() == 0){
    printf( "---------------TEST 5 should fail on access to *pages[17]---------------\n");
    printf( "%d", *pages[17]);
  }
  wait(0);
  printf( "**************************** All tests passed ****************************\n");
    
 
  exit(0);
}