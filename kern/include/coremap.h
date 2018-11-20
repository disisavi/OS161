/*coremap.h 
 * It defines all the interfaces to interact with 
 * physical memory 
*/
#include <machine/vm.h>

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
} coremap;


void coremap_bootsrap(void);
void coremap_allocate(void);
void coremap_deallocate(void);

