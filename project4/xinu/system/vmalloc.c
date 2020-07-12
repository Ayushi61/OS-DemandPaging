
#include <xinu.h>


uint32 available_heap_size;
/*------------------------------------------------------------------------
 *  vmalloc  -  allocate virtual memory on xinu
 *------------------------------------------------------------------------
 */

char* vmalloc(uint32 nbytes)
{

	
	//struct virtual_memblk* prev;
	//struct virtual_memblk* curr;
	//struct virtual_memblk* remaining;
	intmask 	mask;
	mask = disable();
	uint32 num_pgs=nbytes/PAGE_SIZE;
	if(nbytes%PAGE_SIZE!=0)
		num_pgs+=1;
//	if((num_pgs> (available_heap_size + available_swap_size ))|| (num_pgs > (proctab[currpid].heap_size+proctab[currpid].swap_size)))
//	{
		restore(mask);
		sleep(1);
		mask = disable();
//	}
//	if((num_pgs> (available_heap_size + available_swap_size))|| (num_pgs > (proctab[currpid].heap_size+proctab[currpid].swap_size)))
//	{
		
	//	write_cr3(proctab[currpid].pdbr);
	//	kprintf("available_heap_space=%d\n",available_heap_size);
	//	kprintf("Exceeded Heap space\n");
	//	restore(mask);
	//	return SYSERR;
//	}
	
	write_cr3(proctab[0].pdbr);
	
	//prev=proctab[currpid].v_freelist;
	//curr=prev->vnext;
	////kprintf("inside vmalloc\n");
	uint32 ffs_start=FFS_START;
	uint32 nentries=NUM_ENT_PPAGE;
	uint32 swap_end=SWAP_END;
	uint32 pd_end_offset=swap_end/nentries;
	uint32 pd_offset=ffs_start/nentries;
	int i,j;
	pd_t* pdbr=(pd_t *)proctab[currpid].pdbr;
	pd_t* pde;
	uint32 pt_base_fr;
	pt_t *pt_base;
	pt_t *pte;
	uint32 curr=-1,flag=0,cnt=0,temp_i=0,check_bit=0,strt_i=0,strt_j=0;
	//kprintf("pd_offset=%d,pt_offset=%d, num_pgs=%d\n",pd_offset,pd_end_offset,num_pgs);
	for(i=0;i<12;i++)
	{
		
		if(i<4)
			temp_i=i;
		else if(i<8)
		{
		//	kprintf("here\n");
			temp_i=i-4;
		}
		else if (i<12)
		{
		//	kprintf("here\n");
			temp_i=i-8;
		}
			
		pde=(pd_t *)(pdbr+temp_i);
		//////kprintf("%d::%d::%d\n",i,pde,pdbr);
		pt_base_fr=pde->pd_base;
		pt_base=(pt_t*)((pt_base_fr)<<12);
		//////kprintf("pt_base_fr=%d\n",pt_base_fr);
		for(j=0;j<nentries;j++)
		{
			pte=(pt_t *)(pt_base+j);
			if(i<4)
				check_bit=pte->pt_avail;
			else if(i<8)
				check_bit=pte->pt_avail2;
			else if (i<12)
				check_bit=pte->pt_avail3;
			if(flag==0)
			{
				
				if(check_bit)
				{
				//kprintf("in %d\n",pte);
				strt_i=i;
				strt_j=j;				
				curr=(i+5)*NUM_ENT_PPAGE+j;
				flag=1;
				//////kprintf("pte=%d,pt_base=%d\n",pte,curr);
				cnt++;
				}
				
			}
			else 
			{
				if(check_bit)
				{
					//////kprintf("incrementing cnt\n");
					//kprintf("in %d\n",pte);
					cnt++;
				}
				else
				{
					flag=0;	
					cnt=0;
				}
			}
			if(cnt==num_pgs)
			{
				//////kprintf("break here!! \n");
				break;
			}
			
		}
		if(cnt==num_pgs)
		{
			break;
		}
	}
	//kprintf("%d:%d::::%d, strt_i=%d::%d\n",i,j,cnt,strt_i,strt_j);
	int ci=i;
	int cj=j;
	int last_cj,start_cj;
	
	uint32 mod_cnt=cnt%nentries;
	if(curr==-1 || cnt!=num_pgs)
	{
		kprintf("did not find a free block\n");
		write_cr3(proctab[currpid].pdbr);
		restore(mask);
		return SYSERR;
	}
	else{
		//////kprintf("vmalloc - %d\n",curr);
		for(i=strt_i;i<=ci;i++)
		{
			if(i<4)
				temp_i=i;
			else if(i<8)
				temp_i=i-4;
			else if (i<12)
				temp_i=i-8;
			pde=(pd_t *)(pdbr+temp_i);
		//////kprintf("%d::%d::%d\n",i,pde,pdbr);
			pt_base_fr=pde->pd_base;
			pt_base=(pt_t*)((pt_base_fr)<<12);
			//////kprintf("pt_base_fr=%d\n",pt_base_fr);
			if(i==ci)
			{
				last_cj=cj+ 1;
				if(strt_i==ci)
					start_cj=strt_j;
				else
					start_cj=0;
				//kprintf("in\n");
			}
			else if (i==strt_i)
			{
				last_cj=nentries;
				start_cj=strt_j;
			}
			else
			{
				last_cj=nentries;
				start_cj=0;
			}
			for(j=start_cj;j<last_cj;j++)
			{
				pte=(pt_t *)(pt_base+j);
				if(i<4)
				{
					pte->pt_avail=0;
					proctab[currpid].heap_size-=1;
				}
				else if(i<8)
				{
					pte->pt_avail2=0;
					proctab[currpid].swap_size-=1;
				}
				else if(i<12)
				{
					pte->pt_avail3=0;
					proctab[currpid].swap_size-=1;
				}
				//kprintf("vmalloc %d\n",pte);
				//end_curr--;
				//j--;
			}
			//j=NUM_ENT_PPAGE;
		}
	
		//proctab[currpid].heap_size-=num_pgs;
		write_cr3(proctab[currpid].pdbr);
		restore(mask);
		curr=curr<<12;
		return (char *)curr;
	}
	
	
	
}
