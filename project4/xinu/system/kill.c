/* kill.c - kill */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  kill  -  Kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
syscall	kill(
	  pid32		pid		/* ID of process to kill	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	int32	i;			/* Index into descriptors	*/

	mask = disable();
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}

	if (--prcount <= 1) {		/* Last user process completes	*/
		xdone();
	}

	send(prptr->prparent, pid);
	for (i=0; i<3; i++) {
		close(prptr->prdesc[i]);
	}
	freestk(prptr->prstkbase, prptr->prstklen);
	//kprintf("entered kill\n");
	if(proctab[pid].is_user_proc == 1)
	{

		// Remove entries on PT space
		for(i=0; i<MAX_PT_SIZE; i++)
		{	
			////////kprintf("ptab[i].p_fm_pid %d == pid %d\n",ptab[i].p_fm_pid,pid);
			if(ptab[i].p_fm_pid == pid)
			{
				ptab[i].p_fm_pid = 0;
				ptab[i].p_fm_type=0;
				ptab[i].p_fm_vpn = 0;
				ptab[i].p_fm_isfree = 1;
				ptab[i].p_fm_dirty = 0;
				////////kprintf("set %d to free\n",i);
			}

		}

		// Remove entries on FFS space table
		for(i=0; i<MAX_FFS_SIZE; i++)
		{
			if(ftab[i].p_fm_pid == pid)
			{
				ftab[i].p_fm_pid = 0;
				ftab[i].p_fm_type=0;
				ftab[i].p_fm_vpn = 0;
				ftab[i].p_fm_isfree = 1;
				ftab[i].p_fm_dirty = 0;
				available_heap_size+=1;
			}

		}

		// Remove entries on swap space table
		for(i=0; i<MAX_SWAP_SIZE; i++)
		{
			if(stab[i].p_fm_pid == pid)
			{
				stab[i].p_fm_pid = 0;
				stab[i].p_fm_type=0;
				stab[i].p_fm_vpn = 0;
				stab[i].p_fm_isfree = 1;
				stab[i].p_fm_dirty = 0;
				
			}

		}
	}
	
	switch (prptr->prstate) {
	case PR_CURR:
		prptr->prstate = PR_FREE;	/* Suicide */
		resched();

	case PR_SLEEP:
	case PR_RECTIM:
		unsleep(pid);
		prptr->prstate = PR_FREE;
		break;

	case PR_WAIT:
		semtab[prptr->prsem].scount++;
		/* Fall through */

	case PR_READY:
		getitem(pid);		/* Remove from queue */
		/* Fall through */

	default:
		prptr->prstate = PR_FREE;
	}

	restore(mask);
	return OK;
}
