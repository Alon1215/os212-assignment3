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
    
    

    #ifdef SCFIFO
    char in[3];
    int* pages[32];
    ////-----SCFIFO TEST----------///////////
    printf( "--------------------SCFIFO TEST:----------------------\n");
    printf( "-------------allocating 12 pages-----------------\n");
    if(fork() == 0){
        for(int i = 0; i < 13; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
        
        printf( "-------------now add another page. page[0] should move to the file-----------------\n");
        pages[13] = (int*)sbrk(PGSIZE);
        //printf( "-------------all pte_a except the new page should be turn off-------\n");
        printf( "-------------now access to pages[1],[3],[4]-----------------\n");
        printf("pages[1] contains  %d\n",*pages[1]);
         printf("pages[3] contains  %d\n",*pages[3]);
        printf("pages[4] contains  %d\n",*pages[4]);
        printf( "-------------now add another page. page[2] should move to the file-----------------\n");
        pages[14] = (int*)sbrk(PGSIZE);

       
        printf( "-------------now acess to page[2] should cause pagefault-----------------\n");
        printf("pages[2] contains  %d\n",*pages[2]);
        printf( "-------------now acess to page[5] should cause pagefault -----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        printf("---------passed scifo test!!!!----------\n");
        gets(in,3);
        exit(0);

    }
    wait(0);

    #endif

    #ifdef NFUA
    char in[3];
    int* pages[18];
    ////-----NFU + AGING----------///////////
    printf( "--------------------NFU + AGING:----------------------\n");
    printf( "-------------allocating 12 pages-----------------\n");
    if(fork() == 0){
        for(int i = 0; i < 13; i++){
            pages[i] = (int*)sbrk(PGSIZE);
            *pages[i] = i;
        }
        
        printf( "-------------now access all pages except pages[5]-----------------\n");
        for(int i = 0; i < 13; i++){
            if (i!=5)
                *pages[i] = i;
        }
        printf( "-------------now create a new page, pages[5] should be moved to file-----------------\n");
        pages[13] = (int*)sbrk(PGSIZE);
        
        printf( "-------------now acess to page[5] should cause pagefault-----------------\n");
        printf("pages[5] contains  %d\n",*pages[5]);
        printf("---------passed NFUA test!!!!----------\n");
        gets(in,3);
        exit(0);

    }
    wait(0);
    #endif

    #ifdef LAPA
    char in[3];
    int* pages[18];
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
        pages[12] = (int*)sbrk(PGSIZE);
        
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
        pages[12] = (int*)sbrk(PGSIZE);
        
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

  
    
 
  exit(0);
}