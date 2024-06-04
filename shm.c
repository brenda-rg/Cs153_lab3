#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
  int i;
  //initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
      if(shm_table.shm_pages[i].id == id) {
      mappages(myproc()->pgdir, (void*) PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
      shm_table.shm_pages[i].refcnt++;
      *pointer = (char *) PGROUNDUP(myproc()->sz);
      myproc()->sz += PGSIZE;
      release(&(shm_table.lock));
      return 0;
      }
    }
  for (i = 0; i< 64; i++) {
      if(shm_table.shm_pages[i].id == 0) {
      shm_table.shm_pages[i].id = id;
      shm_table.shm_pages[i].frame = kalloc();
      memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
      mappages(myproc()->pgdir, (void *) PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
      shm_table.shm_pages[i].refcnt = 1;
      *pointer = (char*) PGROUNDUP(myproc()->sz);
      myproc()->sz = PGSIZE;
      release(&(shm_table.lock));
      return 0;
      }
    }
//you write this
release(&(shm_table.lock));
return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
//you write this too!
  int i;
  pte_t *pte;
  int found = 0;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    found = 1;
    if(shm_table.shm_pages[i].id == id) {
      shm_table.shm_pages[i].refcnt--;
      if(shm_table.shm_pages[i].refcnt > 0 ) {
        release(&(shm_table.lock));
        return 0;
      }
      shm_table.shm_pages[i].frame = 0;
      shm_table.shm_pages[i].id = 0;
      //shm_table.shm_pages[i].refcnt = 0;
      pte = walkpgdir(myproc()->pgdir, (const void *) PGROUNDUP(myproc()->sz), 1);
      *pte = 0;
      release(&(shm_table.lock));
      return 0;
    }
  }
  if(found == 0) {
    release(&(shm_table.lock));
    return 1; //no matched id found;
  }

  release(&(shm_table.lock));
  return 0; //added to remove compiler warning -- you should decide what to return
}
