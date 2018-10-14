
/*Simple program to check the whether 
1. Each program is getting its own pid and 
2. wait can give appropriate error if we are asking it to wait for a process which is not its child
*/



#include <unistd.h>

int
main(void)
{
	int status;
	pid_t k = waitpid(1,&status,0);
	return k;
}
