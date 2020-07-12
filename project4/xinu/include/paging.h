/* paging.h */
#ifndef _PAGING_H_
#define _PAGING_H_

/* Macros */

#define PAGE_SIZE       4096    /* number of bytes per page                              */
#define MAX_SWAP_SIZE   8192    /* size of swap space (in frames)                        */
#define MAX_FFS_SIZE    4096    /* size of FFS space  (in frames)                        */
#define MAX_PT_SIZE     1024    /* size of space used for page tables (in frames)        */
#define XINU_PAGES 	4096
#define PT_START	XINU_PAGES		/*Starting frame number of the start space for page tables*/
#define PT_END		PT_START+MAX_PT_SIZE	/*Ending frame number of the start space for page tables*/
#define FFS_START	PT_END			/*Starting frame number of the start space for ffs area*/
#define FFS_END		FFS_START+MAX_FFS_SIZE	/*Ending frame number of the start space for ffs area*/
#define SWAP_START	FFS_END			/*Starting frame number of the start space for SWAPP area*/
#define SWAP_END	SWAP_START+MAX_SWAP_SIZE	/*Ending frame number of the start space for SWAPP area*/
#define NUM_ENT_PPAGE	PAGE_SIZE/4		/*NUMBER OF entries per page */


	
/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 1;		/* for programmer's use		*/
  unsigned int pt_avail2 : 1;		/* for programmer's use		*/
  unsigned int pt_avail3 : 1;		/* for programmer's use		*/		
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

/* Structure for a virtual address */

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

/* Structure for a physical address */

typedef struct{
  unsigned int fm_offset : 12;		/* frame offset			*/
  unsigned int fm_num : 20;		/* frame number			*/
} phy_addr_t;

typedef enum {PAGE_DIRECTORY, PAGE_TABLE, PAGE} tab_type;
/* structure for frames in physical space */
typedef struct{
	tab_type p_fm_type; 	/* type of frame */
	pid32 p_fm_pid;					/* process id to which the frame belongs to */
	uint32 p_fm_vpn;					/* virtual page number corresponding to the frame */
	uint32 p_fm_isfree;					/* free list tracking */
	uint32 p_fm_dirty;					/* is dirty?*/
	uint32 p_fm_latency;					/* for lru*/
} phy_fm_t;
typedef enum {PT_ENTRY, FFS, SWAP} frame_type;
extern uint32 allocated_virtual_pages(pid32);
extern uint32 used_swap_pages(pid32);
extern uint32 used_ffs_pages(pid32);
extern uint32 free_swap_pages();
extern uint32 free_ffs_pages();
extern uint32 available_heap_size;
extern uint32 available_swap_size;
extern uint32 latency;
extern phy_fm_t ptab[];	/*tables for page table */
extern phy_fm_t stab[];	/*tables for swap area */
extern phy_fm_t ftab[];	/* table for ffs area */
extern syscall init_frames();
extern uint32 get_phy_fr_num(pid32,frame_type,tab_type,uint32);
extern syscall init_pde(pd_t*);
extern syscall set_pde(pd_t*,uint32);
extern syscall init_pte(pt_t*,uint32);
extern syscall set_pte(pt_t*,uint32);
extern syscall init_pte_no_base(pt_t*);
extern void print_free_list(pid32);
/* Functions to manipulate control registers and enable paging (see control_reg.c)	 */

unsigned long read_cr0(void);

unsigned long read_cr2(void);

unsigned long read_cr3(void);

unsigned long read_cr4(void);

void write_cr0(unsigned long n);

void write_cr3(unsigned long n);

void write_cr4(unsigned long n);

void enable_paging();

#endif
