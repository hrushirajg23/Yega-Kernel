
#define NR_TASKS 10
typedef int (*fn_ptr)();

struct i387_struct {
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
};

struct tss_struct {
    long back_link; //16bits long 
    long esp0;  //kernel stack pointer
    long ss0;  //16bits , kernel stack segment
    long esp1; 
    long ss1; //16bits
    long esp2;
    long ss2; //16bits
    long cr3; //its va_space i.e pg_dir
    long eip;
    long eflags;

/* general purpose registers */
    long eax;
    long ecx;
    long edx;
    long ebx;
    long esp;
    long ebp;
    long esi;
    long edi;
/* segment registers */
    long es; /* 16 high bits are zero */
    long cs; /* 16 high bits are zero */
    long ss; /* 16 high bits are zero */
    long ds; /* 16 high bits are zero */
    long fs; /* 16 high bits are zero */
    long gs; /* 16 high bits are zero */
    long ldtr /* 16 high bits are zero */

    long iopb; // trace bitmap 0-15: reserved, 16-31: iopb
    struct i387_struct i387;
};

struct region {
    // struct m_inode *ex_inode; //pointer to inode of executable file
    char reg_type; 
    unsigned long reg_size;
    unsigned long reg_phy_addr; //physical address of region
    unsigned char status; /*
                             locked,
                             in demand,
                             being loaded in memory,
                             valid(loaded in memory)
                             */

    unsigned int ref_cnt; //no of proc using this region
};
/* per process region table */
/*
 * pprt contains the starting virtual addresses of the regions
 * pprt allows giving permissions for a process to access
 * its regions r/w, r, w, etc much similar like file table */
/* bach ch6 page 166 */
struct pprt {
    unsigned long va_text;
    unsigned char p_text; //permission for text
    struct region *ptr_text;
    unsigned long va_data;
    unsigned char p_data;
    struct region *ptr_data;
    unsigned long va_stack;
    unsigned char p_stack;
    struct region *ptr_stack;
 };

struct task_struct {
    long state; 
    long priority;
    long signal; //signals sent to the process but not yet handled
    fn_ptr sig_restorer;
    fn_ptr sig_fn[32];
    
    int exit_code;
    struct pprt pprt;
    long pid,father,pgrp,session,leader;
	unsigned short uid,euid,suid;
	unsigned short gid,egid,sgid;
	long alarm;
	long utime,stime,cutime,cstime,start_time;
	unsigned short used_math;

    struct pprt pprt;

    /* commenting now since file system is not ready */
	// int tty;		/* -1 if no tty, so it must be signed */
	// unsigned short umask;
	// struct m_inode * pwd;
	// struct m_inode * root;
	// unsigned long close_on_exec;
	// struct file * filp[NR_OPEN];
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */

	// struct desc_struct ldt[3];
/* tss for this task */
	struct tss_struct tss;
};
