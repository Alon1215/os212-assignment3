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
    //        sbrk(4096);
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

    #ifdef SCFIFO
    ////-----SCFIFO TEST----------///////////
    printf( "--------------------SCFIFO TEST:----------------------\n");
    printf( "-------------allocating 12 pages-----------------\n");
    if(fork() == 0){
        for(int i = 0; i < 12; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
        
        printf( "-------------now add another page. page[0] should move to the file-----------------\n");
        pages[13] = (int*)sbrk(PGSIZE);
        //printf( "-------------all pte_a except the new page should be turn off-------\n");
        printf( "-------------now access to pages[1]-----------------\n");
        printf("pages[1] contains  %d\n",*pages[1]);
        printf( "-------------now add another page. page[2] should move to the file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);
        printf( "-------------now acess to page[2] should cause pagefault-----------------\n");
        printf("pages[2] contains  %d\n",*pages[2]);
        printf("---------passed scifo test!!!!----------\n");
        gets(in,3);
        exit(0);

    }
    wait(0);
    #endif

    #ifdef NFUA
    ////-----NFU + AGING----------///////////
    printf( "--------------------NFU + AGING:----------------------\n");
    printf( "-------------allocating 12 pages-----------------\n");
    if(fork() == 0){
        for(int i = 0; i < 12; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
        
        printf( "-------------now access all pages except pages[5]-----------------\n");
        for(int i = 0; i < 12; i++){
            if (i!=5)
                *pages[i] = i;
        }
        printf( "-------------now create a new page, pages[5] should be moved to file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);
        
        printf( "-------------now acess to page[5] should cause pagefault-----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        printf("---------passed NFUA test!!!!----------\n");
        gets(in,3);
        exit(0);

    }
    wait(0);
    #endif

    #ifdef LAPA
    printf( "--------------------LAPA 1:----------------------\n");
    printf( "-------------allocating 12 pages-----------------\n");
    if(fork() == 0){
        for(int i = 0; i < 12; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
        printf( "-------------now access all pages  pages[5] will be acessed first -----------------\n");
        *pages[5] = 5;
        sleep(1);
        for(int i = 0; i < 12; i++){
            if (i!=5)
                *pages[i] = i;
        }
        
       
        printf( "-------------now create a new page, pages[5] should be moved to file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);
        
        printf( "-------------now acess to page[5] should cause pagefault-----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        printf("---------passed LALA 1 test!!!!----------\n");
        gets(in,3);
        exit(0);

    }
    wait(0);

    printf( "--------------------LAPA 2:----------------------\n");
    printf( "-------------allocating 12 pages-----------------\n");
    if(fork() == 0){
        for(int i = 0; i < 12; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
        
        printf( "-------------now access all pages twice except pages[5]-----------------\n");
        for(int i = 0; i < 12; i++){
            if (i!=5)
                *pages[i] = i;
        }
        sleep(1);
        for(int i = 0; i < 12; i++){
            if (i!=5)
                *pages[i] = i;
        }
        printf( "-------------now access pages[5] once-----------------\n");
        *pages[5] = 5;
        printf( "-------------now create a new page, pages[5] should be moved to file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);
        
        printf( "-------------now acess to page[5] should cause pagefault-----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        printf("---------passed LAPA 2 test!!!!----------\n");
        gets(in,3);
        exit(0);

    }
    wait(0);

    printf( "--------------------LAPA 3 : FORK test:----------------------\n");
    printf( "-------------allocating 12 pages for father-----------------\n");
    for(int i = 0; i < 12; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
    printf( "-------------now access all pages twice except pages[5]-----------------\n");
        for(int i = 0; i < 12; i++){
            if (i!=5)
                *pages[i] = i;
        }
        sleep(1);
        for(int i = 0; i < 12; i++){
            if (i!=5)
                *pages[i] = i;
        }
        printf( "-------------now access pages[5] once-----------------\n");
        *pages[5] = 5;
    if(fork() == 0){
        printf( "-------------CHILD: create a new page, pages[5] should be moved to file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);
        
        printf( "-------------CHILD: now acess to page[5] should cause pagefault-----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        exit(0);

    }
    wait(0);
    printf( "-------------FATHER: create a new page, pages[5] should be moved to file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);
        
        printf( "-------------FATHER: now acess to page[5] should cause pagefault-----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        
        
   
    printf("---------passed LAPA 3 test!!!!----------\n");
    gets(in,3);
    #endif

//   printf( "--------------------TEST 2:----------------------\n");
//   printf( "-------------allocating 28 pages-----------------\n");
//   if(fork() == 0){
//     for(int i = 0; i < 29; i++){
//         printf( "doing sbrk number %d\n", i);
//         sbrk(PGSIZE);
//     }
//     printf( "------------child --> allocated_memory_pages: 16 paged_out: 16------------\n");
//     printf( "--------for our output press CTRL^P:--------\n");
//     printf("---------press enter to continue------------\n");
//     gets(in,3);
//     exit(0);
//   }
//   wait(0);
    
//   //TEST 3 - father wait for child and then allocating 18 pages
//   printf("---------press enter to continue------------\n");
//   gets(in,3);
//   printf( "--------------------TEST 3:----------------------\n");
//   for(int i = 0; i < 19; i++){
//     printf( "i: %d\n", i);
//     pages[i] = (int*)sbrk(PGSIZE);
//     printf("pages[i] is %d\n",pages[i]);
//     *pages[i] = i;
//   }
//   printf( "--------father --> allocated_memory_pages: 16 paged_out: 6--------\n");
//   printf( "--------for our output press CTRL^P:--------\n");
//   printf("---------press enter to continue------------\n");
//   gets(in,3);

//   //TEST 4 - fork from father & check if child copy file & RAM data and counters
//   printf( "--------------------TEST 4:----------------------\n");
//   if(fork() == 0){
//     for(int i = 0; i < 18; i++){
//         printf("pages[i] is %d\n",pages[i]);
//         printf( "expected: %d, our output: %d\n",i,*pages[i]);
//     }
//     printf( "--------------expected: allocated_memory_pages: 16 paged_out: 6--------------\n");
//     exit(0);
//   }
//   sleep(5);
//   wait(0);
//   printf( "---------------press enter to continue---------------\n");
//   gets(in,3);

//   //TEST 4 - deleting RAM
//   printf( "-----------deleting physical pages-----------\n");
//   sbrk(-16*PGSIZE);
//   if(fork() == 0){
//     printf( "--------total pages for process is should be 6--------\n");
//     printf( "--------for our output press (CTRL^P):--------\n");
//     exit(0);
//   }
//   wait(0);
//   printf( "--------------press enter to continue--------------\n");
//   gets(in,3);
  
//   // TEST 5 - fail to read pages[17] beacause it deleted from memory
//   if(fork() == 0){
//     printf( "---------------TEST 5 should fail on access to *pages[17]---------------\n");
//     printf( "%d", *pages[17]);
//   }
//   wait(0);
//   printf( "**************************** All tests passed ****************************\n");
    
 
  exit(0);
}