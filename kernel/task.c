#include "serial.h"
#include "task.h"
#include "mm.h"
#include "zone.h"

struct task_struct *process_table[NR_TASKS];
int last_pid = 0;

void init_process_subsystem(void)
{
    int iCnt = 0;
    for (iCnt = 0; iCnt < NR_TASKS; iCnt++){
        process_table[iCnt] = NULL;
    }
}

void fork(void)
{
    
}

int find_empty_process(void)
{
    int iCnt = last_pid;
    for (; iCnt < NR_TASKS; iCnt++){
        if (process_table[iCnt] == NULL){
            return iCnt;
        }
    }
    return -1;
}


