
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

    //lock_acquire(curp->child_lock);
    for(int i=0;i<curp->n_child;i++)
    {
        if((tempHolder+i) == NULL)
        {
            kprintf("\n No process");
        }
        else if ((tempHolder+i)->pid == pid)
        {
            kprintf("\nChild Process found %s",(tempHolder+i)->p_name);
            childp = (tempHolder+i);
            procfound = true;
            break;
        }
    }
    if(!procfound)
    {
        return ECHILD;
    }

    //lock_release(curp->child_lock);
    if(childp->p_status ==P_ZOMBIE)
    {
        //lock_acquire(curp->child_lock);
        *status = childp->returnValue;
        //proc_destroy(childp); //cant be done here. To discuss this tomorrows
        //lock_release(curp->child_lock);

        return 0;
    }

    P(childp->p_sem);

    //lock_acquire(curp->child_lock);
    *status = childp->returnValue;
    childp->p_status = P_ZOMBIE;
    //proc_destroy(childp);
    //lock_release(curp->child_lock);
    return 0;

}


void sys_exit(int exitcode)
{

    struct thread* curt = curthread;
    struct proc* curp = curt->t_proc;
    //struct proc* childp;
    struct proc *tempHolder;
    tempHolder = curp->child_list;
    lock_acquire(curp->child_lock);
    for(int i=0;i<curp->n_child;i++)
    {
      
        if ((tempHolder+i) != NULL )
        {
            if ((tempHolder+i)->p_status == P_ZOMBIE)
            {
                proc_destroy(tempHolder+i);
            }
            else
            {
               (tempHolder+i)->p_status = P_ORPHAN; 
            }
        }
    }
    lock_release(curp->child_lock);


    curp->returnValue = exitcode;
    V(curp->p_sem);


}
