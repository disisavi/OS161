/* Implement all coremap functions from coremap.h
*/
 

#include <types.h>
#include <lib.h>
#include <vm.h>
#include <mainbus.h>
#include <coremap.h>

vaddr_t firstfree;   /* first free virtual address; set by start.S */

void coremap_bootstrap()
{
	page_count = ram_getsize()/PAGE_SIZE;
	int c_pages_count = sizeof(*coremap)*page_count/PAGE_SIZE;
	if( sizeof(*coremap)*page_count%PAGE_SIZE > 0 )
	{
		c_pages_count++;
	}

	paddr_t coremap_addr =  ram_stealmem(c_pages_count);
	coremap = PADDR_TO_KVADDR(coremap_addr);
	int k_pages = coremap_addr/PAGE_SIZE + c_pages_count; //calculating the number of pages used till now, including used by the coremap itself
	if (coremap_addr % PAGE_SIZE > 0)
	{
		k_pages++;


	}

	
	for(int i = 0; i< k_pages;i++)
	{	
		coremap[i].page_adr = i*PAGE_SIZE;
		coremap[i].p_state = M_FIX;
		spinlock_init(&coremap[i].page_lock);

	}
	
	int free_pages = page_count - k_pages;

	for(int i = k_pages; i < free_pages; i++)
	{
		coremap[i].page_adr = i*PAGE_SIZE;
		coremap[i].p_state = M_FREE;
		spinlock_init(&coremap[i].page_lock);
	}

	spinlock_init(&coremap_lock);

}


uint32_t coremap_getnextfree(int no)
{
	spinlock_acquire(&coremap_lock);
	bool page_found =  false ;
 	uint32_t i = 70 ;
	while(i<page_count)
	{
		if(coremap[i].p_state == M_FREE)
		{
			if(i == page_count || (i+no-1) >= page_count)
			{
				break;
			}
			int count = 0;
			for(int j=i+1; j< i+no;j++ )
			{
				if(coremap[j].p_state != M_FREE)
				{
					i = j;
					break;
				}
				count++;
			}
			if(count == no-1)
			{
				page_found = true;
			}
			
		}
		else
		{
			i++;
		}
		if(page_found)
			break;
		
	}

	if(page_found)
	{	
		for(int j = i;j<i+no;j++)
		{
			spinlock_acquire(&coremap[j].page_lock);
		}

		spinlock_release(&coremap_lock);
		return i;
	}
	
	spinlock_release(&coremap_lock);	
	return -1;
	
}

paddr_t coremap_allocatenextn(int no)
{
	int index =  coremap_getnextfree(no);
	int i,j;
	if(index>0)
	{
		for(i = index,j = 0; i<no+index;i++,j++)
		{
			coremap[i].chunk_size = no-j;
			coremap[i].p_state = M_USED;
			spinlock_release(&coremap[i].page_lock);
		}
		return coremap[index].page_adr;
	}

	return 0;
}

void coremap_deallocate(uint32_t page_number)
{
	spinlock_acquire(&coremap_lock);
	uint32_t size = coremap[page_number].chunk_size;
	for(uint32_t i = page_number; i < size+page_number;i++ )
	{
		coremap[i].p_state = M_FREE; 
		coremap[i].chunk_size = 0;
	}
	spinlock_release(&coremap_lock);
}

