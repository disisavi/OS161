
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
#include <limits.h>
#include<copyinout.h>

pid_t sys_waitpid(pid_t pid, int *status, int options)
{
	struct thread* curt = curthread;
	struct proc* curp = curt->t_proc;
	struct proc* childp;
	bool procfound = false;

	if(options != 0)
	{
		return EINVAL;
	}

	if(proclist_isempty(&curp->p_child))
	{
		return ECHILD;
	}

	lock_acquire(curp->child_lock);

	struct proc *itervar;
	PROCLIST_FORALL(itervar, curp->p_child) {
		if (itervar->pid == pid) {
			procfound = true;
			childp = itervar;
			break;
		}
	}
	lock_release(curp->child_lock);
	if(!procfound)
	{
		return ECHILD;
	}

	if(childp->p_status ==P_ZOMBIE)
	{
		lock_acquire(curp->child_lock);
		*status = childp->returnValue;
		//proc_destroy(childp); //cant be done here. To discuss this tomorrows
		lock_release(curp->child_lock);

		return 0;
	}

	P(childp->p_sem);

	*status = childp->returnValue;

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
			if(!proclist_isempty(&(childp->p_child)))
			{
				proc_destroy(childp);
			}
			else
			{
				childp->p_status = P_ORPHAN;	
			}
		}
		else
		{
			childp->p_status = P_ORPHAN;
		}
	}
	curp->p_status = P_ZOMBIE;
	lock_release(curp->child_lock);


	curp->returnValue = exitcode;
	V(curp->p_sem);

	//	proc_remthread(curt);
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


	return 0;
}


	int
sys_execv(char *pname, char **args)
{
	struct addrspace *as;
	struct proc *curp = curproc;
	struct vnode *v;
	int result; 
	vaddr_t entrypoint, stackptr;
	char* progname = kmalloc(PATH_MAX);
	char **karg;
	char *oldstr = kmalloc(PATH_MAX);
	char* oldptr;
	int argc=0;
	int arg_size=0;
	//int size_remaining = ARG_MAX;
	int padding;

	if(args == NULL)
	{
		return EFAULT;
	}
	else 
	{
		karg  = (char **)kmalloc((argc)*sizeof(char*));
		while(*(args+argc*sizeof(userptr_t)))
		{
			result = copyinstr((userptr_t)(args[argc]),oldstr,PATH_MAX,NULL);
			if(result)
			{
				return result;	
			}
			karg[argc] = kmalloc(strlen(oldstr)*sizeof(char*));
			karg[argc] = oldstr;
			argc++;
		}

			
	}

	if(pname == NULL)
		return ENOEXEC;



	karg[argc] = NULL;

	result = copyinstr((userptr_t)pname,progname,PATH_MAX,NULL);
	if(result)
	{
		kfree(progname);
		return result;
	}
	result = vfs_open(progname, O_RDONLY, 0, &v);

	if (result) {
		return result;
	}

	if (curp->p_addrspace) {

		if (curp == curproc) {
			as = proc_setas(NULL);
			as_deactivate();
		}
		else {
			as = curp->p_addrspace;
			curp->p_addrspace = NULL;
		}
		as_destroy(as);
	}


	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();


	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	// new addr space user stack
	vaddr_t  uargs[argc];
	//Add the arguments in the user stack.
	for(int i=argc-1;i>=0;i--)
	{
		arg_size = strlen(karg[i]) + 1;
//		padding = ((arg_size / 4 ) + 1)*4;

		stackptr = stackptr - arg_size;
		
		//Do a copyout and check if success/fail
		result = copyoutstr(karg[i],(userptr_t) stackptr,arg_size+1,NULL);
		if(result){
			return result;
		}

		uargs[i] = stackptr;

	}
//	stackptr-=1;
//	result = copyoutstr(karg[argc],(userptr_t) stackptr,1,NULL);
	

	uargs[argc] = NULL; 
	int mod = stackptr%4;

	if(mod)
	{
		stackptr-=mod;
	}
 
	for(int i = argc; i>=0 ; i--){
		stackptr= stackptr- 4;
		result = copyout(&(uargs[i]),(userptr_t) stackptr, 4);
		if(result){
			return result;
		}
	}

	/* Warp to user mode. */
	enter_new_process(argc /*argc*/,(userptr_t)stackptr /*userspace addr of argv*/,
			NULL /*userspace addr of environment*/,
			stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;

}
