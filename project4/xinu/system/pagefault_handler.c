#include <xinu.h>

syscall	pagefault_handler()
{
	//intmask 	mask;
	//mask = disable();
	//kprintf("in pg\n");
	write_cr3(proctab[0].pdbr);
	virt_addr_t* virt_addr;
	uint32 ptr=(uint32)read_cr2();
	//kprintf("ptr after read_cr2 %d\n",ptr);
	pt_t* pte;
	uint32 pt_frame;
	pd_t* pde;
	
	uint32 vpn=(ptr)>>12;
	virt_addr->pg_offset=((ptr)<<22)>>22;
	virt_addr->pt_offset=((ptr)<<10)>>22;
	virt_addr->pd_offset=(ptr)>>22;
	//kprintf("pd_offset=%d,pt_offset=%d,pg_offset=%d\n",virt_addr->pd_offset,virt_addr->pt_offset,virt_addr->pg_offset);
	uint32 pt_offset=virt_addr->pt_offset;
	uint32 pd_offset=virt_addr->pd_offset;
	uint32 pg_offset=virt_addr->pg_offset;
	////kprintf("vpn=%d\n",vpn);
	pde=proctab[currpid].pdbr;
	////kprintf("pdbr=%d\n",pde);
	uint32 ffs_end=FFS_END;
	uint32 pg_size=PAGE_SIZE;
	uint32 vheap_limit=ffs_end*pg_size;
	uint32 pr_frame;
	uint32 ffs_frame;
	//print_free_list(currpid);
	if((uint32)virt_addr>(uint32)vheap_limit)
	{
		kprintf("Segmentation fault illegal access in swap area by page faul handler accessed memory =%d , legal limit=%d \n",(uint32)virt_addr,(uint32)vheap_limit);
		write_cr3(proctab[currpid].pdbr);
		kill(currpid);
		return SYSERR;
	}
	int i,j;
	pd_t* pdbr1=(pd_t *)proctab[currpid].pdbr;
	pd_t* pde1;
	uint32 pt_base_fr1;
	pt_t *pt_base1;
	pt_t *pte1;
	
	pde1=(pd_t *)(pdbr1+((pd_offset-1)%4));
	pt_base_fr1=pde1->pd_base;
	pt_base1=(pt_t*)((pt_base_fr1)<<12);
	pte1=(pt_t *)(pt_base1+pt_offset);
	int check_bit;
	if((pd_offset-1)<=8)
	{
		check_bit=pte1->pt_avail;
	//	kprintf("here %d\n",pte1);
	}
	else if((pd_offset-1)<=12)
		check_bit=pte1->pt_avail2;
	else if((pd_offset-1)<=16)
		check_bit=pte1->pt_avail3;
	if(check_bit)
	{
		kprintf("segmentation fault, illegal access in free list area\n");
		kprintf("%d::%d::%d::%d, virt mem=%d\n",pte1->pt_base,pte1->pt_avail,pte1->pt_avail2,pte1->pt_avail3,ptr);
		print_free_list(currpid);
		write_cr3(proctab[currpid].pdbr);
		//restore(mask);
		kill(currpid);
		return SYSERR;
	}
	
	
	/*if page directory is not present, create a new page table frame and add it to page directory*/
	if((pde+pd_offset)->pd_pres==0)
	{
		////kprintf("directory not present, creating\n");
		if(available_heap_size==0 && available_swap_size==0)
		{
			kprintf("out of heap and swap space to allocate memory\n");
			write_cr3(proctab[currpid].pdbr);
			//restore(mask);
			kill(currpid);
			return SYSERR;
		}
		if(available_heap_size==0)
		{
			/*evict frames*/
			pt_frame=get_phy_fr_num(currpid,PT_ENTRY,PAGE,-1);
			pte=(pt_t *)(pt_frame*PAGE_SIZE);
			init_pte_no_base(pte);
			////kprintf("pde = %d\n",pde);	
			////kprintf("pde+virt_addr->pd_offset=%d\n",pd_offset);
			set_pde((pde+pd_offset),pt_frame);
			ffs_frame=get_phy_fr_num(currpid,FFS,PAGE,vpn);
			if(ffs_frame==SYSERR)
			{	
				kprintf("no memory left in process swap\n");

				//available_heap_size+=1;
				write_cr3(proctab[currpid].pdbr);
				//restore(mask);
				kill(currpid);
				return SYSERR;
			}
			set_pte(pte+pt_offset,ffs_frame);
			available_swap_size-=1;

		}
		else
		{
			/*allocate heap space */
			available_heap_size-=1;
			pt_frame=get_phy_fr_num(currpid,PT_ENTRY,PAGE,-1);
			////kprintf("pt_frame=%d\n",pt_frame);
			pte=(pt_t *)(pt_frame*PAGE_SIZE);
			init_pte_no_base(pte);
			////kprintf("pde = %d\n",pde);	
			////kprintf("pde+virt_addr->pd_offset=%d\n",pd_offset);
			set_pde((pde+pd_offset),pt_frame);
			ffs_frame=get_phy_fr_num(currpid,FFS,PAGE,vpn);
			//kprintf("ffs frame=%d\n",ffs_frame);
			if(ffs_frame==SYSERR)
			{
				kprintf("no memory left in process heap\n");

				available_heap_size+=1;
				write_cr3(proctab[currpid].pdbr);
				//restore(mask);
				kill(currpid);
				return SYSERR;
			}
			////kprintf("pte=%d\n",pte);
			////kprintf("pt_offset=%d,pg_num=%d\n",pt_offset,ffs_frame);
			set_pte(pte+pt_offset,ffs_frame);
		}	
	}
	else
	{
		uint32 sw_frame,swap_start,pg_size;
		swap_start=SWAP_START;
		pg_size=PAGE_SIZE;
		sw_frame=-1;
		pt_t* pt_entry=(pt_t *)(((pde+pd_offset)->pd_base)*PAGE_SIZE);
		if((pt_entry+pt_offset)->pt_pres==0)
		{
		//if page is not in ffs , look in SWAP AREA
			////kprintf("pd found but pt not found \n");
			int i=0;
			for(i=0;i<MAX_SWAP_SIZE;i++)
			{
				if(stab[i].p_fm_pid==currpid && stab[i].p_fm_vpn==vpn)
				{
					//				
					sw_frame=(swap_start+i)*pg_size;
					//kprintf("sw_frame found = %d\n",sw_frame);	
				}
				
			}
			if(sw_frame==-1)
			{
				//kprintf("not in swap space\n");
				/*not present in swap area too, get new ffs space*/
				if(available_heap_size<0 && available_swap_size<0)
				{
					kprintf("out of global heap size\n");
					kprintf("%d =ptr\n",ptr);
					write_cr3(proctab[currpid].pdbr);
					//restore(mask);
					kill(currpid);
					return SYSERR;
				}
				
				ffs_frame=get_phy_fr_num(currpid,FFS,PAGE,vpn);
				if(ffs_frame==SYSERR)
				{
					kprintf("process out of heap space\n");
					write_cr3(proctab[currpid].pdbr);
					//restore(mask);
					kill(currpid);
					return(SYSERR);
				}
				//kprintf("new frame allocated %d\n",ffs_frame);
				if(available_heap_size==0)
					available_swap_size-=1;
				else
					available_heap_size-=1;
		//pte=(pt_t *)(pt_frame*PAGE_SIZE);
		//init_pte_no_base(pte);
		//set_pde(pde+virt_addr->pd_offset,pt_frame);
		//ffs_frame=get_phy_fr_num(currpid,FFS,PAGE,vpn);
				////kprintf("pt_entry+pt_offset=%d,%d\n",pt_entry+pt_offset,ffs_frame);
				set_pte(pt_entry+pt_offset,ffs_frame);
				
			}
			else
			{
			/* frame is present in disk copy contents from disk to ffs*/
				ffs_frame=get_phy_fr_num(currpid,FFS,PAGE,vpn);
				if(ffs_frame==SYSERR)
				{
					kprintf("process out of heap space\n");
					write_cr3(proctab[currpid].pdbr);
					//restore(mask);
					kill(currpid);
					return(SYSERR);
				}
				//available_heap_size-=1;
				bcopy((sw_frame),(ffs_frame*pg_size),pg_size);
				#ifdef DEBUG_SWAPPING
					kprintf("swapping:: swap frame 0x%o, ffs frame 0x%0\n",((uint32)(sw_frame))>>12,ffs_frame);
				#endif
				set_pte(pt_entry+virt_addr->pt_offset,ffs_frame);
			


			}

		}
		

	}
	////kprintf("returning from page fault handler\n");
	write_cr3(proctab[currpid].pdbr);
	////restore(mask);
	return OK;
	
}
