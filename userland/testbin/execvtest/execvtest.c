
/*Simple program to check the whether 
 execv does anything at all
 */



#include <unistd.h>

	int
main(void)
{
	int status;
	printf("\nPARENT PID IS  %ld", getpid());
	pid_t k = fork();
	if(k==0)
	{
		printf("\nInside Child k is ....%ld ",k);	
		execv("testbin/palin",NULL );
	}
	else
	{
		printf("\n In parent... K value is %ld and get pid is %ld",k,getpid());
		waitpid(k,&status,0);
		printf ("\nreturn from waitpid is %ld",status);
	}
	return k;
}
