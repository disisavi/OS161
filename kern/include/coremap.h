/*coremap.h 
 * It defines all the interfaces to interact with 
 * physical memory 
*/
#include <machine/vm.h>
#include<spinlock.h>
//  Page states 
typedef enum{
	M_FREE, // The page is free to be loaded
	M_USED, // The page is currently filled
	M_FIX, // The page wont be swapped come what may
} page_state;

// basic struct of coremap
struct coremap_struct{
	paddr_t page_adr;
	struct spinlock page_lock;
	page_state p_state;
	uint32_t chunk_size;
} *coremap;

uint32_t page_count;
struct spinlock coremap_lock;

void coremap_bootstrap(void);
paddr_t coremap_allocatenextn(int no);
void coremap_deallocate(uint32_t page_number); // discuss whether we need this...
uint32_t coremap_getnextfree(int no); // here n is the number of consicutive pages we want free. If we want only one free, we can just put n = 1;


