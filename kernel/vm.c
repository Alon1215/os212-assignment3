#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "spinlock.h" // Task 1

#include "proc.h" // Task 1
/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

// Make a direct-map page table for the kernel.
pagetable_t
kvmmake(void)
{
  pagetable_t kpgtbl;

  kpgtbl = (pagetable_t) kalloc();
  memset(kpgtbl, 0, PGSIZE);

  // uart registers
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // PLIC
  kvmmap(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  kvmmap(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

  // map kernel stacks
  proc_mapstacks(kpgtbl);
  
  return kpgtbl;
}

// Initialize the one kernel_pagetable
void
kvminit(void)
{
  kernel_pagetable = kvmmake();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart()
{
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return 0;
  if((*pte & PTE_V) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kpgtbl, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if(*pte & PTE_V)
      panic("remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

//  ---Task 1 ----

// // reset page meta data in mpage struct and proc's meta data fields
// void resetpagemd(struct proc *p, struct mpage *page) {
//   page->allpagesindex = -1;
//   page->state = FREE;
//   page->va = -1; // check

// }

// ---------------

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PTE_V) == 0){ // page is not in RAM, check if page is in swap file
      if ((*pte & PTE_PG) == 0)
        panic("uvmunmap: not mapped");
      
      #ifndef NONE
     
        struct proc *p = myproc();
        struct mpage *page;
        int i;
        for(i=0; i < MAX_TOTAL_PAGES; i++){
          page = &p->allpages[i];
          if (page->va == va){ //found in file
            goto found;
          }
        }
        panic("uvmunmap: not in file (but should be)");


        found:
          resetpagemd(p,page);
          p->fileentries[page->entriesarrayindex] = 0;
      
      #endif
    } 
    ///TODO: if page in ram, any other action?
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) kalloc();
  if(pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void
uvminit(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
  memmove(mem, src, sz);
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{

  printf("in uvmalloc. process has %d physical and %d in file \n",myproc()->physcnumber,myproc()->swapednumber);//TODO delete
  char *mem;
  uint64 a;
  struct proc *p = myproc();
  //struct page *page;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  for(a = oldsz; a < newsz; a += PGSIZE){
    #ifndef NONE
    
      if (p->pid>2){
        // printf("in uvmalloc in ifdef\n");//TODO delete
        //in case there are already 32 pages, return -1
        if ((p->physcnumber) + (p->swapednumber)==32)
        {
          return 0 ;
        }
        
          //chaeck whether we need to make space for the new page in the ram.
        if (p->physcnumber == 16)
        {

          if (p->swapFile == 0)
          {
            createSwapFile(p);
          }
          
          ///TODO: after implement getpagetoreplace, check what can it retrun.
          int ptomoveindex = getpagetoreplace();
          if (physicpagetoswapfile(&p->allpages[ptomoveindex])<0){
              // 0 indicates a failure in this function
              return 0;

          }
        }  
      }    
    
    #endif
    //now we know there is enough place on the ram for allocating a new page
    //printf("in uvmalloc after ifdef\n");//TODO delete
    mem = kalloc();
    if(mem == 0){
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  //update all data structures we added a new page:
  if(p->pid>2){
    
    //find an empty slot in allpages:
    int i;
    for ( i = 0; i < 32; i++)
    {
      if (p->allpages[i].state == FREE)
      {
        break;
      }
      
    }
    p->allpages[i].state = RAM;
    p->allpages[i].va = a;
    p->allpages[i].allpagesindex = i;
    p->allpages[i].entriesarrayindex = -1;
    p->physcnumber++;

  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  printf("in uvmdeaaloc b1 \n");//TODO delete
  if(newsz >= oldsz)
    return oldsz;

  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
  if(sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
}

// <<< Task 1

//TASK2:
// simple bit counter from SO
uint64 countSetBits(unsigned int n)
{
    unsigned int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}


int nfua(){
  struct proc *p = myproc();
  struct mpage *current = p->queueRAM;
  int minvalue;
  struct mpage *page;
  
  while (current != 0){
    // look for pages in RAM & compare to min value
    if( current->entriesarrayindex == -1 && current->access_counter < minvalue){
      minvalue = current->access_counter;
      page = current;
    }
  }
  ///TODO: should return index or page?
  return page->allpagesindex;
}
int lapa(){
  struct proc *p = myproc();
  struct mpage *current = p->queueRAM;
  struct mpage *page;
  int minvalue;
  int minindex;
  
  while (current != 0){
    // look for pages in RAM & compare
    if(current->va != 0 && current->entriesarrayindex == -1){
      if (countSetBits(current->access_counter) < countSetBits(minvalue) 
      || (countSetBits(current->access_counter) == countSetBits(minvalue)) && current->access_counter < minvalue){
        minvalue = current->access_counter;
        page = current;
      }
    }
  }
  
  ///TODO: should return index or page?
  return page->allpagesindex;
}
int scfifo(){
  struct proc *p = myproc();
  struct mpage *current = p->queueRAM;
  struct mpage *page = 0;
  pte_t *pte;

  while (current != 0){
    pte = walk(p->pagetable,page->va, 0); ///TODO: add any checks here?
    if ((*pte & PTE_A)){
      *pte & ~PTE_A; // turn access bit off
    } else { 
      page = current;
      break;
    }
    if (current->next == 0 && page == 0) current = p->queueRAM; // demonstrate a circular queue for scfifo logic
  }
  return page->allpagesindex;
}

int getpagetoreplace(){
  #if SELECTION == NONE
    printf("getpagetoreplace(): error SELECTION=NONE\n");
    return -1;
  #endif
  
  #if SELECTION == NFUA
    return nfua();
  #endif
  #if SELECTION == LAPA
    return lapa();
  #endif
  #if SELECTION == SCFIFO
    return scfifo();
  #endif
  
  panic("getpagetoreplace: bad SELECTION"); // selected not valid, abort.
  return -1;
}

// Adding a given page the proc's swap file
int pagetoswapfile(struct mpage* page){
  struct proc* p = myproc();
  pte_t *pte;
  int offset;
  for (offset = 0; offset <= MAX_PSYC_PAGES; offset++){
    if (!p->fileentries[offset]) goto found;
  }

  printf("pagetoswapfile: No free entry in swao file\n");
  return -1;

  found:

    
    pte  = walk(p->pagetable, page->va, 0);

    // Should we check the (PTE_P & *pte)?
    if (!pte){
      printf("pagetoswapfile: pte = 0\n");
      return -1;
    } 
    
    if(writeToSwapFile(p, (char *)page->va, (offset*PGSIZE), PGSIZE) == -1) panic("pagetoswapfile: writeToSwapFile() failed");
    
    p->fileentries[offset] = 1;
    p->swapednumber++;
    page->state = FILE;
    *pte = (*pte | PTE_PG) &~ PTE_V; // set PTE_PG flag up, and PTE_V down

  return 0;
}

int physicpagetoswapfile(struct mpage* page) {

  if (pagetoswapfile(page) == -1) return -1;

  struct proc *p = myproc();
  p->physcnumber--;
  kfree((char*)page->va);
  // TODO: is there anything else to do to release the pysic page?
  return 0;
}

int filetophysical(struct mpage* page) {
  char * va;
  uint64 pa;
  struct proc *p = myproc();

  // allocate a new page in the physical memory 
  if ((va = kalloc()) == 0){
    printf("retrievingpage: kalloc failed\n");
    return -1;
  } 
  //archive the pa from the va we got in kalloc
  ///TODO: assure this is the right way to get pa.
  if((pa = walkaddr(p->pagetable,(uint64)va))==0) 
    return -1;

  //we need to map the page va to the new physical memory we allocated. 
  ///TODO: decide what permissions we want.
  mappages(p->pagetable,page->va,PGSIZE,pa,PTE_R | PTE_W);


  // copy page to pa
  if(readFromSwapFile(p,(char*)page->va,page->entriesarrayindex*PGSIZE,PGSIZE) < 0){
    return -1;
  }
  pte_t* pte;

  //get pte in the pagetable in order to set the flags
  if((pte = walk(p->pagetable,page->va, 0)) == 0){
    return -1;
  }
  p->fileentries[page->entriesarrayindex] = 0;
  page->state = RAM;
  page->entriesarrayindex = -1;
  *pte = (*pte | PTE_V) &~ PTE_PG;

 
  return 0;
}






// Retrieving pages on demand (file to page)
int retrievingpage (struct mpage* page){
  //struct proc *p = myproc();
  //pte_t *pte;
  //char *va;
  int pagetoreplace; 
  ///TODO: create getpagetoreplace. which choose page to swap.
  if((pagetoreplace = getpagetoreplace()) >0){
    physicpagetoswapfile(&myproc()->allpages[pagetoreplace]);
  
  }

  
  ///TODO: it's okay to return? or panic?!
  if (filetophysical(page) < 0){
    return -1;
  }

  

  return 0 ;
}



int handlepagefault(){
  struct proc *p = myproc();
  //retreive adress caused pagefault
  uint64 va_fault = r_stval();

  pte_t* pte; 
  if ((pte = walk(p->pagetable,va_fault,0)) < 0){
    return -1;
  } 

  //check if the pte valid flag is down and file flagis up
  if (!(*pte & PTE_V) && (*pte & PTE_PG))
  {
    int i;
    //find the page caused pagefault and swap it to the RAM
    for ( i = 0; i < MAX_TOTAL_PAGES; i++)
    {
      if(p->allpages[i].va == va_fault){
        break;
      }
    }
    if (i == MAX_TOTAL_PAGES){
      panic("page caused fault wasnt found");
    }
    retrievingpage(&p->allpages[i]);
  }
  ///TODO: how can we detect segmentation fault? and hoe to handle this case???
  //--seg fault--//



  //

  return 0;
}




int updatepagesage(struct proc* p){
  struct mpage *page;
  pte_t *pte;
  int i;
  for (i=0; i < MAX_TOTAL_PAGES; i++){
    page = &p->allpages[i];
    if (page->entriesarrayindex == -1){
      // page is in RAM
      page->access_counter >> 1;
      pte = walk(p->pagetable,page->va, 0); ///TODO: add any checks here?
      if ((*pte & PTE_A)){
        page->access_counter &= (1L << 31); //TODO: right shift?

        *pte & ~PTE_A; // turn access bit off
      }
    }
  }


}


// >>> Task 1 END
