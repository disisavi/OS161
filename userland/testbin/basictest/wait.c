
/*Simple program to check the whether 
  1. Each program is getting its own pid and 
  2. wait can give appropriate error if we are asking it to wait for a process which is not its child
  */



#include <unistd.h>

	int
main(void)
{
	int status;
	printf("\npid is %ld", getpid());
	pid_t k = fork();
	if(k==0)
	{	
		printf("in child");
	} 
	waitpid(k,&status,0);
	printf ("\nreturn from waitpid is %ld",status);
	return k;
}
