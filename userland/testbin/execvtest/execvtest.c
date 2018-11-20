
/*Simple program to check the whether 
 execv does anything at all
 */



#include <unistd.h>
static char *hargv[5] = { (char *)"Hello","Is there","Anybody","Abhijeet", NULL };
	int
main(void)
{
	int status;
	printf("\nPARENT PID IS  %ld", getpid());
	pid_t k = fork();
	if(k==0)
	{
		char args[2];
		printf("\nInside Child k is ....%ld ",k);	
		execv("testbin/argtest",hargv );
	}
	else
	{
		printf("\n In parent... K value is %ld and get pid is %ld",k,getpid());
		waitpid(k,&status,0);
		printf ("\nreturn from waitpid is %ld",status);
	}
	return k;
}

