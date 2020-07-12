/* initialize.c - nulluser, sysinit */

/* Handle system initialization and become the null process */

#include <xinu.h>
#include <string.h>

extern	void	start(void);	/* Start of Xinu code			*/
extern	void	*_end;		/* End of Xinu code			*/

/* Function prototypes */

extern	void main(void);	/* Main is the first process created	*/
static	void sysinit(); 	/* Internal system initialization	*/
extern	void meminit(void);	/* Initializes the free memory list	*/
local	process startup(void);	/* Process to finish startup tasks	*/
extern void paging_init(); 	/* page table and page directory initializations*/
/* Declarations of major kernel variables */

struct	procent	proctab[NPROC];	/* Process table			*/
struct	sentry	semtab[NSEM];	/* Semaphore table			*/
struct	memblk	memlist;	/* List of free memory blocks		*/
uint32 available_heap_size;
uint32 available_swap_size;
uint32 latency;
/*paging declarations*/
//struct pd_t pdtab[XINU_PAGES/PAGE_SIZE]; /* page directory table*/
//struct pt_t pttable[MAX_PT_SIZE-NPROC];  /* page table table*/

/* Active system status */

int	prcount;		/* Total number of live processes	*/
pid32	currpid;		/* ID of currently executing process	*/

/* Control sequence to reset the console colors and cusor positiion	*/

#define	CONSOLE_RESET	" \033[0m\033[2J\033[;H"

/*------------------------------------------------------------------------
 * nulluser - initialize the system and become the null process
 *
 * Note: execution begins here after the C run-time environment has been
 * established.  Interrupts are initially DISABLED, and must eventually
 * be enabled explicitly.  The code turns itself into the null process
 * after initialization.  Because it must always remain ready to execute,
 * the null process cannot execute code that might cause it to be
 * suspended, wait for a semaphore, put to sleep, or exit.  In
 * particular, the code must not perform I/O except for polled versions
 * such as kprintf.
 *------------------------------------------------------------------------
 */

void	nulluser()
{	
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	uint32	free_mem;		/* Total amount of free memory	*/
	
	/* Initialize the system */
	//kprintf("%d NPROC\n",NPROC);
	sysinit();
//paging!!!
	//paging_init();
	
	
//paging!!

	/* Output Xinu memory layout */
	free_mem = 0;
	for (memptr = memlist.mnext; memptr != NULL;
						memptr = memptr->mnext) {
		free_mem += memptr->mlength;
	}
	kprintf("%10d bytes of free memory.  Free list:\n", free_mem);
	for (memptr=memlist.mnext; memptr!=NULL;memptr = memptr->mnext) {
	    kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)memptr, ((uint32)memptr) + memptr->mlength - 1);
	}

	kprintf("%10d bytes of Xinu code.\n",
		(uint32)&etext - (uint32)&text);
	kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)&text, (uint32)&etext - 1);
	kprintf("%10d bytes of data.\n",
		(uint32)&ebss - (uint32)&data);
	kprintf("           [0x%08X to 0x%08X]\n\n",
		(uint32)&data, (uint32)&ebss - 1);

	/* Enable interrupts */

	enable();

	/* Initialize the network stack and start processes */

	net_init();

	/* Create a process to finish startup and start main */

	resume(create((void *)startup, INITSTK, INITPRIO,
					"Startup process", 0, NULL));

	/* Become the Null process (i.e., guarantee that the CPU has	*/
	/*  something to run when no other process is ready to execute)	*/

	while (TRUE) {
		;		/* Do nothing */
	}

}


/*------------------------------------------------------------------------
 *
 * startup  -  Finish startup takss that cannot be run from the Null
 *		  process and then create and resume the main process
 *
 *------------------------------------------------------------------------
 */
local process	startup(void)
{
	uint32	ipaddr;			/* Computer's IP address	*/
	char	str[128];		/* String used to format output	*/


	/* Use DHCP to obtain an IP address and format it */

	ipaddr = getlocalip();
	if ((int32)ipaddr == SYSERR) {
		kprintf("Cannot obtain an IP address\n");
	} else {
		/* Print the IP in dotted decimal and hex */
		ipaddr = NetData.ipucast;
		sprintf(str, "%d.%d.%d.%d",
			(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
			(ipaddr>>8)&0xff,        ipaddr&0xff);
	
		kprintf("Obtained IP address  %s   (0x%08x)\n", str,
								ipaddr);
	}

	/* Create a process to execute function main() */

	resume(create((void *)main, INITSTK, INITPRIO,
					"Main process", 0, NULL));

	/* Startup process exits at this point */

	return OK;
}


/*------------------------------------------------------------------------
 *
 * sysinit  -  Initialize all Xinu data structures and devices
 *
 *------------------------------------------------------------------------
 */
static	void	sysinit()
{
	int32	i;
	struct	procent	*prptr;		/* Ptr to process table entry	*/
	struct	sentry	*semptr;	/* Ptr to semaphore table entry	*/
	struct pd_t *pd_dir;
	struct pt_t *pt_tab;
	/* Reset the console */

	kprintf(CONSOLE_RESET);
	kprintf("\n%s\n\n", VERSION);

	/* Initialize the interrupt vectors */

	initevec();
	
	/* Initialize free memory list */
	
	meminit();

	/* Initialize system variables */

	/* Count the Null process as the first process in the system */

	prcount = 1;

	/* Scheduling is not currently blocked */

	Defer.ndefers = 0;

	/* Initialize process table entries free */
	/* initialize page directory and page table */
	for (i = 0; i < NPROC; i++) {
		pd_dir=&ptab[i];
		prptr = &proctab[i];
		prptr->prstate = PR_FREE;
		prptr->prname[0] = NULLCH;
		prptr->prstkbase = NULL;
		prptr->prprio = 0;
		
		
	}

	/* Initialize the Null process entry */	

	prptr = &proctab[NULLPROC];
	prptr->prstate = PR_CURR;
	prptr->prprio = 0;
	strncpy(prptr->prname, "prnull", 7);
	prptr->prstkbase = getstk(NULLSTK);
	prptr->prstklen = NULLSTK;
	prptr->prstkptr = 0;
	prptr->is_user_proc=0;
	currpid = NULLPROC;
	
	/* Initialize semaphores */

	for (i = 0; i < NSEM; i++) {
		semptr = &semtab[i];
		semptr->sstate = S_FREE;
		semptr->scount = 0;
		semptr->squeue = newqueue();
	}

	/* Initialize buffer pools */

	bufinit();

	/* Create a ready list for processes */

	readylist = newqueue();


	/* initialize the PCI bus */

	pci_init();

	/* Initialize the real time clock */

	clkinit();

	for (i = 0; i < NDEVS; i++) {
		init(i);
	}
	available_heap_size=MAX_FFS_SIZE;
	available_swap_size=MAX_SWAP_SIZE;
	latency=0;
	init_frames();
	//kprintf("init frame done\n");
	uint32 pd_base_fr=get_phy_fr_num(currpid,PT_ENTRY,PAGE_DIRECTORY,-1);
	uint32 pd_base_addr=pd_base_fr*PAGE_SIZE;
	uint32 total_num_pages=XINU_PAGES+MAX_PT_SIZE+MAX_SWAP_SIZE+MAX_FFS_SIZE;
	uint32 ent_ppage = NUM_ENT_PPAGE;
	//kprintf("total_pages%d\n",total_num_pages);
	//kprintf("entry per page%d\n",NUM_ENT_PPAGE);
	uint32 total_pde=total_num_pages/ent_ppage;
	//kprintf("total number of entries in page dir=%d\n",total_pde);
	pd_t* pd_base=(pd_t*) pd_base_addr;
	init_pde(pd_base);

	//kprintf("init_pde\n");
	//syscall init_pte(pt_base_addr);
	uint32 offset=0;
	//kprintf("total_pde%d\n",total_pde);
	i=0;
	uint32 xinu_pages=XINU_PAGES+MAX_PT_SIZE;
	uint32 total_pde_xinu=xinu_pages/ent_ppage;
	//kprintf("total_xinu_pde=%d\n",total_pde_xinu);
	for(i=0;i<total_pde;i++)
	{
		//kprintf("pde #%d\n",i);
		uint32 pt_base_fr=get_phy_fr_num(currpid,PT_ENTRY,PAGE_TABLE,-1);
		uint32 pt_base_addr=pt_base_fr*PAGE_SIZE;
		pt_t* pt_base=(pt_t*)pt_base_addr;
		set_pde(pd_base,pt_base_fr);
		uint32 j=0;
		
		for(j=0;j<NUM_ENT_PPAGE;j++)
		{
			//if(i<total_pde_xinu)
			//{
				set_pte(pt_base,i*NUM_ENT_PPAGE+j);
			//}
			//else
			//{
			//	set_pte(pt_base,i*NUM_ENT_PPAGE+j);
			//}
			pt_base++;	
		}
		

		pd_base++;
	}
	//kprintf("set pde done\n");
	//struct procent* prptr=&proctab[currpid];
	unsigned long temp=(((pd_base_fr*PAGE_SIZE)>>12)<<12);
	write_cr3((unsigned long)temp);
	proctab[currpid].pdbr=temp;
	
	//kprintf("pdbr before shift%d\n",pd_base_addr);
	//kprintf("pdbr after shift%d\n",temp);
	set_evec(14,(unsigned long)pagefault_handler_disp);
	enable_paging();
	//kprintf("set_evec\n");

	return;
}

int32	stop(char *s)
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* Empty */;
}

int32	delay(int n)
{
	DELAY(n);
	return OK;
}


