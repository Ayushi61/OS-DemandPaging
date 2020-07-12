#include <xinu.h>
//#define DEBUG_SWAPPING
phy_fm_t ptab[MAX_PT_SIZE];
phy_fm_t stab[MAX_SWAP_SIZE];
phy_fm_t ftab[MAX_FFS_SIZE];
uint32 latency;
syscall init_frames()
{
	int i=0;
	for(i=0;i<MAX_PT_SIZE;i++)
	{
		//phy_fm_t* ptptr=&ptab[i];
		ptab[i].p_fm_pid=0;
		ptab[i].p_fm_type=0;
		ptab[i].p_fm_vpn=0;
		ptab[i].p_fm_isfree=1;
		ptab[i].p_fm_dirty=0;
		ptab[i].p_fm_latency=0;
		
	}
	for(i=0;i<MAX_FFS_SIZE;i++)
	{
		//phy_fm_t* ffptr=&ftab[i];
		ftab[i].p_fm_pid=0;
		ftab[i].p_fm_type=0;
		ftab[i].p_fm_vpn=0;
		ftab[i].p_fm_isfree=1;
		ftab[i].p_fm_dirty=0;
		ftab[i].p_fm_latency=0;
		
	}
	for(i=0;i<MAX_SWAP_SIZE;i++)
	{
		//phy_fm_t* swptr=&stab[i];
		stab[i].p_fm_pid=0;
		stab[i].p_fm_type=0;
		stab[i].p_fm_vpn=0;
		stab[i].p_fm_isfree=1;
		stab[i].p_fm_dirty=0;
		stab[i].p_fm_latency=0;
	}
	

	return OK;
}

uint32 get_phy_fr_num(pid32 pid,frame_type type,tab_type table,uint32 vpn)
{

	uint32 offset;
	uint32 addr;
	if(type==PT_ENTRY)
	{
		
		for(offset=0;offset<MAX_PT_SIZE;offset++)
		{
			//phy_fm_t* ptptr=&ptab[offset];
			if(ptab[offset].p_fm_isfree)
			{
				////kprintf("here!!!!!!!!!!!!!!!!!!!!!!!!!!!!1%d\n",offset);
				//addr = ptptr;
				ptab[offset].p_fm_type = table;
				ptab[offset].p_fm_pid = pid;
				ptab[offset].p_fm_isfree = 0;
				addr=offset+PT_START;
				//addr=addr*PAGE_SIZE;
				break;
			}
			
		}
		
	}

	else if(type==FFS)
	{
		////kprintf("does not enter \n");
		latency++;
		for(offset=0;offset<MAX_FFS_SIZE;offset++)
		{
			//phy_fm_t* ffsptr=&ftab[offset];
			if(ftab[offset].p_fm_isfree)
			{
				//addr = ffsptr;
				ftab[offset].p_fm_type = PAGE;
				ftab[offset].p_fm_pid = pid;
				ftab[offset].p_fm_isfree = 0;
				ftab[offset].p_fm_vpn=vpn;
				ftab[offset].p_fm_latency=latency;
				//kprintf("%d\n",latency);
				addr=offset+FFS_START;
				//addr=addr*PAGE_SIZE;
				break;
			}

		}
		// there are free frames present in FFS
		if(offset<MAX_FFS_SIZE)
		{
			//available_heap_size -= 1;
			return FFS_START+offset;
		}
		else
		{
			// there are no free frames present in FFS
			// swap a FFS frame to SWAP space
			// use approximate LRU
			//kprintf("using approx lru\n");
			uint32 lru_fr_index, tmp_fr_index;
     			uint32 pd_bits, tmp_pd_bits;
     			uint32 pt_bits, tmp_pt_bits;
     			uint32 lru_fr_vpn, tmp_fr_vpn;
     			uint32 lru_fr_pid, tmp_fr_pid;
     			uint32 swap_fr;
			uint32 fr_num;
			pd_t *tmp_pd;
			pt_t *tmp_pt;
			pd_t *PDE;
			pt_t *PTE;
			uint32 latest_acc=latency;
			//kprintf("latency=%d\n",latency);
			// Approximate LRU to find the frame to be evicted
			for(offset=0; offset<MAX_FFS_SIZE; offset++)
			{
				fr_num = FFS_START + offset;

				tmp_fr_vpn = ftab[offset].p_fm_vpn;
				tmp_fr_pid = ftab[offset].p_fm_pid;
				tmp_pd_bits = tmp_fr_vpn >> 10;
				tmp_pt_bits = (tmp_fr_vpn << 22) >> 22;
				
				tmp_pd = (pd_t *)(proctab[tmp_fr_pid].pdbr);
				tmp_pt = (pt_t *)((tmp_pd + tmp_pd_bits)->pd_base * PAGE_SIZE);
				tmp_pt = tmp_pt + tmp_pt_bits;
				//if(tmp_pt->pt_acc == 1)
				if(ftab[offset].p_fm_latency<latest_acc)
				{
					latest_acc = ftab[offset].p_fm_latency;
					//kprintf("ltatest_acc=%d\n",latest_acc);
					lru_fr_index = offset;
					lru_fr_vpn = tmp_fr_vpn;
					lru_fr_pid = tmp_fr_pid;
				}
				//else
				//{
				//	lru_fr_index = offset;
				//	lru_fr_vpn = tmp_fr_vpn;
				//	lru_fr_pid = tmp_fr_pid;
				//	tmp_pt->pt_acc == 1;
				//	break;
				//}
			}

			//if(offset >= MAX_FFS_SIZE)
			//{
				//kprintf("no frame found for replacement as none were most recently used\n");
			//	return SYSERR;
			//}
			
			pd_bits = lru_fr_vpn >> 10;
			pt_bits = (lru_fr_vpn << 22) >> 22;
			uint32 ffs_fr_to_swap;
			//kprintf("lru frame = %d,latency=%d\n",lru_fr_index,latency);
			PDE = (pd_t *)(proctab[lru_fr_pid].pdbr);
			PTE = (pt_t *)((PDE + pd_bits)->pd_base * PAGE_SIZE);
			PTE = PTE + pt_bits;
			//kprintf(" ffs frame num=%d\n",PTE->pt_base);
			if(PTE->pt_dirty == 0)
			{
				// check for presence in swap region
				int i;
				for(i=0; i<MAX_SWAP_SIZE; i++)
				{
					if((stab[i].p_fm_pid == lru_fr_pid) &&(stab[i].p_fm_vpn == lru_fr_vpn))
					{
						break;
					}
				}

				if(i<MAX_SWAP_SIZE)
				{
					// found a swap frame with latest copy
					ffs_fr_to_swap=PTE->pt_base;
					swap_fr = SWAP_START + i;
					PTE->pt_pres = 0;
					PTE->pt_base = swap_fr;
					//PTE->pt_avail = 1;
					#ifdef DEBUG_SWAPPING
						kprintf("eviction::ffs frame 0x%x, swap frame 0x%x\n",ffs_fr_to_swap,swap_fr);

					#endif
				}
				else
				{
					swap_fr = get_phy_fr_num(lru_fr_pid, SWAP, PAGE, lru_fr_vpn);
					//available_swap_size -= 1;
					ffs_fr_to_swap=PTE->pt_base;
					if(swap_fr == SYSERR)
						return SYSERR;

					bcopy(((FFS_START + lru_fr_index) << 12), ((swap_fr) << 12), PAGE_SIZE);

					PTE->pt_base = swap_fr;
					PTE->pt_pres = 0;
					//PTE->pt_avail = 1;
					#ifdef DEBUG_SWAPPING
						kprintf("eviction::ffs frame 0x%x, swap frame 0x%x\n",ffs_fr_to_swap,swap_fr);

					#endif
				}
			}
			else
			{
				//if dirty bit is set, ie need to copy data to disk and then proceed.				
				int i;
				for(i=0; i<MAX_SWAP_SIZE; i++)
				{
					if((stab[i].p_fm_pid == lru_fr_pid) &&(stab[i].p_fm_vpn == lru_fr_vpn))
					{
						break;
					}
				}

				if(i<MAX_SWAP_SIZE)
				{
					// found a swap frame with latest copy
					swap_fr = SWAP_START + i;
					//PTE->pt_pres = 0;
					//PTE->pt_base = swap_fr;
					//PTE->pt_avail = 1;
				}
				else
				{
					swap_fr = get_phy_fr_num(lru_fr_pid, SWAP, PAGE, lru_fr_vpn);
					//available_swap_size -= 1;
					if(swap_fr == SYSERR)
						return SYSERR;

				}

				bcopy(((FFS_START + lru_fr_index) << 12), ((swap_fr) << 12), PAGE_SIZE);
				ffs_fr_to_swap=PTE->pt_base;
				PTE->pt_base = swap_fr;
				PTE->pt_pres = 0;
				//PTE->pt_avail = 1;
				#ifdef DEBUG_SWAPPING
						kprintf("eviction::ffs frame 0x%x, swap frame 0x%x\n",ffs_fr_to_swap,swap_fr);

				#endif
			}
			/* Invalidate the FSS frame befor sending */
			ftab[lru_fr_index].p_fm_pid = pid;
			ftab[lru_fr_index].p_fm_isfree = 0;
			ftab[lru_fr_index].p_fm_type = PAGE;
			ftab[lru_fr_index].p_fm_vpn = vpn;
 			ftab[lru_fr_index].p_fm_latency=latency;
    			return (FFS_START + lru_fr_index);		
		}
		
	}

	else if(type==SWAP)
	{
		for(offset=0;offset<MAX_SWAP_SIZE;offset++)
		{
			//phy_fm_t* swptr=&stab[offset];
			if(stab[offset].p_fm_isfree)
			{
				//addr = swptr;
				stab[offset].p_fm_type = PAGE;
				stab[offset].p_fm_pid = pid;
				stab[offset].p_fm_isfree = 0;
				stab[offset].p_fm_vpn=vpn;
				addr=offset+SWAP_START;
				//addr=addr*PAGE_SIZE;
				break;
			}

		}	
	}
	else
	{
		//kprintf("unsupported page type\n");
		return SYSERR;
	}

	////kprintf("addr %d\n", addr);
	return addr;
}

syscall init_pde(pd_t* pd_base)
{
	int i;
	//pd_t* pd_base=(pd_t*) pd_base_addr;
	for(i=0;i<NUM_ENT_PPAGE;i++)
	{
		
		pd_base->pd_pres=0;	
 		pd_base->pd_write=0;
  		pd_base->pd_user=0;
  		pd_base->pd_pwt=0;
  		pd_base->pd_pcd=0;
  		pd_base->pd_acc=0;
 		pd_base->pd_mbz=0;
  		pd_base->pd_fmb=0; 
  		pd_base->pd_global=0;
  		pd_base->pd_avail=0;
  		pd_base->pd_base=0;
		////kprintf("pdbase %d\n",pd_base);		
		pd_base++;
			
	}
	
	return OK;
}


syscall set_pte(pt_t* pt_base1,uint32 pg_num)
{
	int i;
	//pt_t* pt_base1=(pt_t*) pt_base_addr;
	////kprintf("%d\n", pt_base1);
	////kprintf(pt_base1);
	//for(i=0;i<NUM_ENT_PPAGE;i++)
	//{
		//pt_t* pt_base=(pt_t*) (pt_base_addr+offset);
		pt_base1->pt_pres=1;	
	 	pt_base1->pt_write=1;
	  	pt_base1->pt_user=0;
	  	pt_base1->pt_pwt=0;
	  	pt_base1->pt_pcd=0;
  		pt_base1->pt_acc=0;
	 	pt_base1->pt_mbz=0;
	  	pt_base1->pt_dirty=0;
  		pt_base1->pt_global=0;
	  	pt_base1->pt_avail=0;
		pt_base1->pt_avail2=0;
		pt_base1->pt_avail3=0;
  		pt_base1->pt_base=pg_num;
		
		//pt_base1++;
	//}
	////kprintf("sum %d\n",offset*NUM_ENT_PPAGE+i);
	return OK;


}

syscall init_pte_no_base(pt_t* pt_base1)
{
	int i=0;
	for(i=0;i<NUM_ENT_PPAGE;i++)
	{
		//pt_t* pt_base=(pt_t*) (pt_base_addr+offset);
		pt_base1->pt_pres=0;	
	 	pt_base1->pt_write=0;
	  	pt_base1->pt_user=0;
	  	pt_base1->pt_pwt=0;
	  	pt_base1->pt_pcd=0;
  		pt_base1->pt_acc=0;
	 	pt_base1->pt_mbz=0;
	  	pt_base1->pt_dirty=0;
  		pt_base1->pt_global=0;
	  	pt_base1->pt_avail=0;
		pt_base1->pt_avail2=0;
		pt_base1->pt_avail3=0;
  		pt_base1->pt_base=0;
		pt_base1++;
	}
	////kprintf("sum %d\n",offset*NUM_ENT_PPAGE+i);
	return OK;

}




syscall init_pte(pt_t* pt_base1,uint32 pg_num)
{
	//int32 pt_base_addr=get_phy_fr_num(currpid,PT_ENTRY,PAGE);
	//init_pages(pt_base_addr);
	int i;
	//pt_t* pt_base1=(pt_t*) pt_base_addr;
	//kprintf("%d\n", pt_base1);
	////kprintf(pt_base1);
	//for(i=0;i<NUM_ENT_PPAGE;i++)
	//{
		//pt_t* pt_base=(pt_t*) (pt_base_addr+offset);
		pt_base1->pt_pres=0;	
	 	pt_base1->pt_write=0;
	  	pt_base1->pt_user=0;
	  	pt_base1->pt_pwt=0;
	  	pt_base1->pt_pcd=0;
  		pt_base1->pt_acc=0;
	 	pt_base1->pt_mbz=0;
	  	pt_base1->pt_dirty=0;
  		pt_base1->pt_global=0;
	  	pt_base1->pt_avail=0;
		pt_base1->pt_avail2=0;
		pt_base1->pt_avail3=0;
  		pt_base1->pt_base=pg_num;
		//pt_base1++;
	//}
//	//kprintf("sum %d\n",offset*NUM_ENT_PPAGE+i);
	return OK;

}

syscall set_pde(pd_t* pd_base,uint32 pt_base_fr)
{
	
	////kprintf("pd_entries %s\n",pt_base_addr);
	//set_pte(pt_base_addr);
	//int offset=0;
	//for(offset=0;offset<total_pde;offset++)
	//{
	//for(offset=0;offset<NUM_ENT_PPAGE;offset++)
		////kprintf("pde #%d\n",offset);
		//uint32 pt_base_fr=get_phy_fr_num(currpid,PT_ENTRY,PAGE_TABLE);
		//uint32 pt_base_addr=pt_base_fr*PAGE_SIZE;
		//pt_t* pt_base=(pt_t*)pt_base_addr;
		
	//pd_t* pd_base=(pd_t*) (pd_base_addr);
		
		//kprintf("pdbaase %d\n",pd_base);
		pd_base->pd_pres=1;	
 		pd_base->pd_write=1;
  		pd_base->pd_user=0;
  		pd_base->pd_pwt=0;
  		pd_base->pd_pcd=0;
  		pd_base->pd_acc=0;
 		pd_base->pd_mbz=0;
  		pd_base->pd_fmb=0;
  		pd_base->pd_global=0;
  		pd_base->pd_avail=0;
  		pd_base->pd_base=pt_base_fr;
		//pd_base++;
	//	set_pte(pt_base,offset);
	//}
	return OK;

}


uint32 free_ffs_pages()
{
	return available_heap_size;

}

uint32 free_swap_pages()
{

	return available_swap_size;
}

uint32 used_ffs_frames(pid32 pid)
{
	uint32 i,cnt=0;
	
	for(i=0;i<MAX_FFS_SIZE;i++)
	{
		//phy_fm_t* ffptr=&ftab[i];
		
		if(ftab[i].p_fm_isfree==0 && ftab[i].p_fm_pid==pid)
			cnt++;
	}
	return cnt;
}

uint32 used_swap_frames(pid32 pid)
{
	uint32 i,cnt=0;
	for(i=0;i<MAX_SWAP_SIZE;i++)
	{
		//phy_fm_t* swptr=&stab[i];
		if(stab[i].p_fm_isfree==0 && stab[i].p_fm_pid==pid)
			cnt++;
		
	}

	return cnt;
}

uint32 allocated_virtual_pages(pid32 pid)
{
	
	////kprintf( "from virtual_space %d\n",((((MAX_FFS_SIZE*PAGE_SIZE)-proctab[pid].v_freelist->vlength)/PAGE_SIZE)+((MAX_SWAP_SIZE*PAGE_SIZE)-proctab[pid].swap_size)));
	return (MAX_FFS_SIZE+(MAX_FFS_SIZE-proctab[pid].heap_size));
}


void print_free_list(pid32 pid)
{
	//struct virtual_memblk * curr=proctab[pid].v_freelist;
	//struct virtual_memblk* next=curr->vnext;
	return;

}

