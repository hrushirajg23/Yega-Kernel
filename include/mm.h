#ifndef _MM_H
#define _MM_H


#define PAGE_SIZE 4096

#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES (PAGING_MEMORY/PAGE_SIZE)

void memset(void *addr, char val, unsigned int size)
{
    char *ptr = (char*)addr;
    register int iCnt = 0;
    while (iCnt < size){
        ptr[iCnt++]=val;
    }
}
#endif
