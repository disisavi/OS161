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
	int total_pages = ram_getsize()/PAGE_SIZE;
	int c_pages_count = sizeof(*coremap)*total_pages/PAGE_SIZE;
	if( sizeof(*coremap)*total_pages%PAGE_SIZE > 0 )
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

	int free_pages = total_pages - k_pages;
	
	for(int i = 0; i< k_pages;i++)
	{
		coremap[i].p_state = M_FIX;
	}

	for(int i = k_pages; i < free_pages; i++)
	{
		coremap[i].p_state = M_FREE;
	}

}


void coremap_allocate()
{
}


void coremap_deallocate()
{
}
