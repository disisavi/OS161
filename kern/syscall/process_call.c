
//Waitpid
#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>
#include <test.h>
#include <synch.h>


pid_t sys_waitpid(pid_t pid, int *status, int options)
{
    struct thread* curt = curthread;
	struct proc* curp = curt->t_proc;
	struct proc* childp;
    bool procfound = false;
    struct proc *tempHolder;
    tempHolder = curp->child_list;

    if(options != 0)
	{
		return EINVAL;
	}

    lock_acquire(curp->child_lock);
    for(int i=0;i<childlimit;i++)
    {
      
        if ((tempHolder+i)->pid == pid)
        {
            kprintf("Lets see... ");
            childp = (tempHolder+i);
            procfound = true;
            break;
        }
    }
    if(!procfound)
    {
        return ECHILD;
    }

    lock_release(curp->child_lock);
    if(childp->status == zombie)
    {
        lock_acquire(curp->child_lock);
        status = &childp->returnValue;
        //proc_destroy(childp); //cant be done here. To discuss this tomorrows
        lock_release(curp->child_lock);

        return 0;
    }

    P(childp->p_sem);

    lock_acquire(curp->child_lock);
    status = &childp->returnValue;
    childp->status = zombie;
    //proc_destroy(childp);
    lock_release(curp->child_lock);
    return 0;

}


void sys_exit(int exitcode)
{

    struct thread* curt = curthread;
	struct proc* curp = curt->t_proc;
	struct proc* childp;
    struct proc *tempHolder;
    tempHolder = curp->child_list;
    lock_acquire(curp->child_lock);
    for(int i=0;i<childlimit;i++)
    {
      
        if ((tempHolder+i) != NULL )
        {
            proc_destroy((tempHolder+i));
        }
        else if ((tempHolder+i)->status == zombie)
        {
            proc_destroy(tempHolder+i);
        }
    }
    lock_release(curp->child_lock);


    curp->returnValue = exitcode;
    V(childp->p_sem);


}
