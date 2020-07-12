#include <xinu.h>


/*------------------------------------------------------------------------
 *  vfree  -  allocate virtual memory on xinu
 *------------------------------------------------------------------------
 */

syscall vfree(char* ptr,uint32 nbytes)
{	
	intmask mask;
	mask=disable();
	struct virtual_memblk *prev;
	struct virtual_memblk *next;
	uint32 last_free_blk;
	virt_addr_t* virt_addr;
	write_cr3(proctab[0].pdbr);
	//////kprintf("in free \n");
	uint32 ffs_start=FFS_START;
	uint32 swap_end=SWAP_END;
	uint32 pg_size=PAGE_SIZE;

	//kprintf("ptr=%d\n",ptr);
	//kprintf("nbytes==%d||((uint32)ptr=%d <ffs_start %d *pg_size %d)||((uint32)ptr>ffs_end %d *pg_size)\n",nbytes,ptr,ffs_start,pg_size,ffs_end);
	if(nbytes==0|| ((uint32)ptr < ffs_start*PAGE_SIZE) || ((uint32)ptr > swap_end*PAGE_SIZE))
	{
		kprintf("invalid memory accress\n");
		write_cr3(proctab[currpid].pdbr);
		restore(mask);
		return SYSERR;
	}
	
	uint32 num_of_pages=nbytes/PAGE_SIZE;
	
	uint32 pg_offset,pd_offset,pt_offset;
	uint32 pdbr=proctab[currpid].pdbr;
	pg_offset=(((uint32)ptr)<<22)>>22;
	pt_offset=(((uint32)ptr)<<10)>>22;
	pd_offset=((uint32)ptr)>>22;
	//kprintf("pd_offset = %d::%d::%d\n",pd_offset,pt_offset,pg_offset);
	pd_t * pde=(pd_t*) (pdbr+((pd_offset-1)%4));
	uint32 pt_base_fr=pde->pd_base;
	pt_t* pte=(pt_t *)((pt_base_fr) << 12)+pt_offset;
	int i;
	int check_bit=0;
	//kprintf("num_pgs=%d\n",num_of_pages);
	for(i=0;i<num_of_pages;i++)
	{
		if(i<4096)
			check_bit=pte->pt_avail;
		else if(i< 80192)
		{
			//kprintf("swap\n");
			check_bit=pte->pt_avail2;
		}
		else if (i<12288)
		{
			//kprintf("swap\n");
			check_bit=pte->pt_avail3;
		}
	/* since other processes can free the bits*/
	//	if(check_bit)
	//	{
			//kprintf("pte break=%d\n",pte);
	//		break;
	//	}
	//	else{
			if(i<4096)
			{
				pte->pt_avail=1;
				//kprintf("%d\n",pte);
			}
			else if(i<80192)
				pte->pt_avail2=1;
			else if (i<12288)
				pte->pt_avail3=1;
			//kprintf("%d\n",pte);
	//	}
		if(i==4096 || i==8192)
		{
		//	kprintf("swap \n");
			pte-=4096;
		}
		else
			pte++;
	}
	if(i<num_of_pages)
	{
		kprintf("trying to free more than needed block\n");
		write_cr3(proctab[currpid].pdbr);
		restore(mask);
		return SYSERR;
	}
	//kprintf("freeing\n");
	
	
	
	
	
	//proctab[currpid].v_freelist->vlength+=nbytes;
	//////kprintf("in free , freed %d\n",nbytes);
	//proctab[]
	int j;
	
	if(nbytes%PAGE_SIZE!=0)
		num_of_pages+=1;
	pg_offset=(((uint32)ptr)<<22)>>22;
	pt_offset=(((uint32)ptr)<<10)>>22;
	pd_offset=((uint32)ptr)>>22;
	//pt_t* pte;
	//pd_t* pde;
	uint32 vpn=((uint32)ptr)>>12;
	//proctab[currpid].heap_size+=num
	uint32 flag=num_of_pages;
	//kprintf("starting for loop %d\n",num_of_pages);
	for(i=0;i<num_of_pages;i++)
	{
		for(j=0;j<MAX_SWAP_SIZE;j++)
		{
			////////kprintf("%d\n",j);
			if(stab[j].p_fm_vpn==vpn && stab[j].p_fm_pid==currpid)
			{

				stab[j].p_fm_isfree=1;
				stab[j].p_fm_pid=-1;
				stab[j].p_fm_vpn=0;
				stab[j].p_fm_type=0;
				stab[j].p_fm_dirty=0;
				pde=(pd_t *)(proctab[currpid].pdbr+pd_offset);
				pte=(pt_t *)(pde->pd_base+pt_offset);
				pte->pt_pres=0;
				available_swap_size+=1;
				proctab[currpid].swap_size+=1;
				flag--;
				
			}
		}
		for(j=0;j<MAX_FFS_SIZE;j++)
		{
			if(stab[j].p_fm_vpn==vpn && stab[j].p_fm_pid==currpid)
			{

				ftab[j].p_fm_isfree=1;
				ftab[j].p_fm_pid=-1;
				ftab[j].p_fm_vpn=0;
				ftab[j].p_fm_type=0;
				ftab[j].p_fm_dirty=0;
				pde=(pd_t *)(proctab[currpid].pdbr+pd_offset);
				pte=(pt_t *)(pde->pd_base+pt_offset);
				pte->pt_pres=0;
				available_heap_size+=1;
				proctab[currpid].heap_size+=1;
				flag--;
				
			}
		}
		
		ptr+=PAGE_SIZE;
		////////kprintf("%d\n",i);

	}
	//////kprintf("for loop done\n");
	proctab[currpid].heap_size+=flag;
	//kprintf("exiting free\n");
	//////kprintf("free list ########################################\n");
	print_free_list(currpid);
	write_cr3(proctab[currpid].pdbr);
	restore(mask);
	return OK;
}
