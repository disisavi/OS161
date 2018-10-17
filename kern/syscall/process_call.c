
//Waitpid
#include <types.h>
#include <kern/errno.h>
#include <cdefs.h>
#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <mips/trapframe.h>
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
#include <proclist.h>
#include <lib.h>

pid_t sys_waitpid(pid_t pid, int *status, int options)
{
	struct thread* curt = curthread;
	struct proc* curp = curt->t_proc;
	struct proc* childp;
	bool procfound = false;
	int counter = 0;
	struct proclistnode plist_node;

	if(options != 0)
	{
		return EINVAL;
	}

	lock_acquire(curp->child_lock);
	if(proclist_isempty(&curp->p_child))
	{
		return ECHILD;
	}

	plist_node = curp->p_child.pl_head;
	while(counter <curp->p_child.pl_count)
	{
		if (plist_node.pln_self->pid == pid)
		{
			procfound = true;
			childp = plist_node.pln_self;
			break;
		}
		if(plist_node.pln_next != NULL)
		{
			plist_node =*plist_node.pln_next;
			counter++;
		}
		else{
			break;
		}
	}

	if(!procfound)
	{
		return ECHILD;
	}

	lock_release(curp->child_lock);
	if(childp->p_status ==P_ZOMBIE)
	{
		lock_acquire(curp->child_lock);
		*status = childp->returnValue;
		//proc_destroy(childp); //cant be done here. To discuss this tomorrows
		lock_release(curp->child_lock);

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
	struct proclist plistchild = curp->p_child;
	struct proc *childp;

	lock_acquire(curp->child_lock);

	while(!proclist_isempty(&plistchild))
	{
		childp = proclist_remhead(&plistchild);
		if(childp->p_status == P_ZOMBIE)
		{
			proc_destroy(childp);
		}
		else
		{
			childp->p_status = P_ORPHAN;
		}
	}

	lock_release(curp->child_lock);


	curp->returnValue = exitcode;
	V(curp->p_sem);
	thread_exit();


}

	int
sys_getpid(int *retval)
{
	*retval = curproc->pid;
	return 0;
}

/*
 * Enter user mode for a newly forked process.
 */
	void
enter_forked_process(struct trapframe *tf)
{
	// Return value for child is 0
	tf->tf_v0   = 0;
	// Indicate a successful return
	tf->tf_a3   = 0;
	// Increment pc so that fork is not called again
	tf->tf_epc += 4;

	struct trapframe child_tf;
	bzero(&child_tf, sizeof(child_tf));
	memcpy(&child_tf, tf, sizeof(child_tf));

	kfree(tf);

	as_activate();
	mips_usermode(&child_tf);
}

/*
 * Create a copy of the existing process
 */
	int
sys_fork(struct trapframe *proc_tf, int *retval)
{
	/* Ref to current process */
	struct proc *proc = curproc;

	if (proc->p_child.pl_count <= MAX_CHILDREN) {
		/* Copy trap frame of current process */
		struct trapframe *child_tf = kmalloc(sizeof(*proc_tf));
		if (child_tf == NULL) {
			return ENOMEM;
		}
		memcpy(child_tf, proc_tf, sizeof(*proc_tf));

		/* Copy proc structure of current process */
		struct proc **child_proc = kmalloc(sizeof(struct proc*));
		int err = proc_fork(child_proc);
		if (err) {
			kfree(child_tf);
			return err; 
		}

		/* Adding the child proc in parent childrens list */
		proclist_addhead(&proc->p_child, *child_proc);        

		/* Copy addrspace of current process */
		struct addrspace *proc_addrspace = proc_getas();
		struct addrspace **child_addrspace = 
			kmalloc(sizeof(struct addrspace*)); 
		err = as_copy(proc_addrspace, child_addrspace);
		if (err) {
			kfree(child_proc);
			kfree(child_tf);
			return err;
		}
		(*child_proc)->p_addrspace = *child_addrspace;

		/* Copy current thread */
		err = thread_fork("child", *child_proc,
				enter_forked_process,
				child_tf, 0);
		if (err) {
			kfree(child_addrspace);
			kfree(child_proc);
			kfree(child_tf);
			return err;
		}


		/* Return value of current process */
		*retval = (*child_proc)->pid;
	}
	else {
		kprintf("Too many processes\n");
		return EMPROC;
	}

	proc->pl_count = proc->pl_count+1;

	return 0;
}
