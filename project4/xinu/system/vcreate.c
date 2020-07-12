/* create.c - create, newpid */

#include <xinu.h>

local	int newpid();

/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	vcreate(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr;		/* Stack address		*/
	uint32		heap_size;
	heap_size=MAX_FFS_SIZE;
	mask = disable();
	write_cr3(proctab[0].pdbr);
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) ) {
		restore(mask);
		return SYSERR;
	}
	//kprintf("----in vcreate\n");
	//if(heap_size>available_heap_size)
	//{
	//	//kprintf("demand of heap size above required size");
	//	restore(mask);
//		return SYSERR;
//	}
	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prstklen = ssize;

	prptr->is_user_proc=1;
	prptr->heap_size=heap_size;
	prptr->swap_size=MAX_SWAP_SIZE;
	//prptr->pdbr=
	//prptr->
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;
	prptr->pdbr=proctab[NULLPROC].pdbr;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	/* Initialize stack as if the process was called		*/

	*saddr = STACKMAGIC;
	savsp = (uint32)saddr;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--)	/* Machine dependent; copy args	*/
		*--saddr = *a--;	/* onto created process's stack	*/
	*--saddr = (long)INITRET;	/* Push on return address	*/

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	*--saddr = (long)funcaddr;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = 0;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
	
	/*get page directory */
	uint32 page_dir_fr,num_of_pt;
	page_dir_fr=get_phy_fr_num(pid,PT_ENTRY,PAGE_DIRECTORY,-1);
	//kprintf("stuck here!!!!%d\n",page_dir_fr);
	pd_t* pr_pd=(pd_t*)(page_dir_fr*PAGE_SIZE);
	init_pde(pr_pd);
	uint32 xinu_pages=XINU_PAGES;
	uint32 nentries=NUM_ENT_PPAGE;
	num_of_pt=xinu_pages/nentries;
	//kprintf("num of xinu pages %d\n",num_of_pt);
	for(i=0;i<num_of_pt;i++)
	{
		//kprintf("pde #%d\n",i);
		uint32 pt_base_fr=get_phy_fr_num(pid,PT_ENTRY,PAGE_TABLE,-1);
		uint32 pt_base_addr=pt_base_fr*PAGE_SIZE;
		pt_t* pt_base=(pt_t*)pt_base_addr;
		set_pde(pr_pd,pt_base_fr);
		uint32 j=0;
		
		for(j=0;j<NUM_ENT_PPAGE;j++)
		{
			set_pte(pt_base,i*NUM_ENT_PPAGE+j);
			pt_base->pt_avail=1;
			pt_base->pt_avail2=1;
			pt_base->pt_avail3=1;
			pt_base++;
		}
		pr_pd++;

	}
	//uint32 other_pgs=(MAX_PT_SIZE+MAX_FFS_SIZE+MAX_SWAP_SIZE)/nentries;
	/*for(i=num_of_pt;i<other_pgs+num_of_pt;i++)
	{
		uint32 pt_base_fr=get_phy_fr_num(pid,PT_ENTRY,PAGE_TABLE,-1);
		uint32 pt_base_addr=pt_base_fr*PAGE_SIZE;
		pt_t* pt_base=(pt_t*)pt_base_addr;
		//set_pde(pr_pd,pt_base_fr);
		pr_pd->pd_base=pt_base_fr;
		//pr_pd->pd_write=0;
		uint32 j=0;
		//kprintf("##pr_pd=%d\n",((uint32)(pr_pd)>>12));
		for(j=0;j<NUM_ENT_PPAGE;j++)
		{
			//init_pte(pt_base,i*NUM_ENT_PPAGE+j)
			pt_base->pt_avail=1;
			//pt_base->pt_base=i*NUM_ENT_PPAGE+j;
			pt_base++;
		}
		//kprintf("%d\n",pr_pd->pd_pres);
		pr_pd++;
	}*/
	prptr->pdbr=page_dir_fr<<12;
	//kprintf("pdbr of process %d \n",prptr->pdbr);
	prptr->v_freelist=getmem(sizeof(struct virtual_memblk));
	prptr->v_freelist->vlength=PAGE_SIZE*heap_size;
	uint32 ffs_start=FFS_START;
	uint32 page_size=PAGE_SIZE;
	prptr->v_freelist->vnext=(struct virtual_memblk *)(ffs_start*page_size);
	prptr->v_freelist->vnext->vlength=PAGE_SIZE*heap_size;
	prptr->v_freelist->vnext->vnext=NULL;
	//kprintf("prptr->v_freelist->vlength=%d,prptr->v_freelist->vnext=%d,FFS_START*PAGE_SIZE=%d,prptr->v_freelist=%d\n",prptr->v_freelist->vlength,prptr->v_freelist->vnext,ffs_start*page_size,prptr->v_freelist);
	write_cr3(prptr->pdbr);
	//kprintf("returning from vcreate\n");

	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}
	return (pid32) SYSERR;
}
