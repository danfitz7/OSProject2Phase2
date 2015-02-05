#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/list.h>
#include <asm/errno.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/pid.h>
unsigned long **sys_call_table;

// useful links
// http://www.makelinux.net/books/lkd2/ch03lev1sec1

// init vars to store results before returning them to the caller.
#define n_processes 100 		// we only deal with lists of 100 processes
unsigned short target_uid = -1; 	// the uid of the current target process to smite or unsmite
int num_pids_smited = 0;		// the number of processes to smite or unsmite
int smited_pids[n_processes];	// the pids of the smited processes
long pid_states[n_processes];	// the previous statuses of the smited processes

struct task_struct *task;		// pointer to the task_struct we're trying to smite/unsmite
struct list_head *list;

struct pid* pid_struct;

// Our new kernel module function for SMITING
asmlinkage long (*ref_sys_cs3013_syscall1)(void); // store the old function pointer
asmlinkage long new_sys_cs3013_syscall1(unsigned short* p_target_uid, int* p_num_pids_smited, int* p_smited_pids, long* p_pid_states) {
	//get the current task_struct
	struct task_struct* current_task_struct = get_current();
	
	// only smite if we're root
	if(current_task_struct->real_cred->uid.val != 0){
		printk(KERN_INFO "\"SMITE ERROR: Caller not root!\" -- Smiter\n");
		return EACCES;
	}
	
	// Get the user ID to smite
	// check validity of arguments passed by caller. If we couldn't copy any bytes (didn't return 0 bytes not copied), exit with an error code.
	if (copy_from_user(&target_uid, p_target_uid, sizeof(unsigned short)) != 0){
		printk(KERN_INFO "\"SMITE ERROR: Invalid pointer to target UID!\" -- Smiter\n");
		return EFAULT;
	}
	if (target_uid <= 0){
		printk(KERN_INFO "\"Smiting users of ID %d\" -- Smiter\n", target_uid);
	}else{
		printk(KERN_INFO "\"SMITING ERROR: Invalid target UID.\" -- Smiter\n");
		return EINVAL;
	}
	

	
	// search for processes with this user ID.  
	num_pids_smited = 0; // keep track of the number of processes added to the array (reset to 0)
	
	// TODO: this doesn't traveerse the entire linked list of processes and their children.
	// Doing that BFS will require more work
	// but it's a start for what we need to do with a process once we've found it.
	
	// http://stackoverflow.com/questions/11986893/when-does-task-struct-used
	list_for_each(list, &current->children){
		task = list_entry(list, struct task_struct, sibling);
		/* task now points to one of current's children */
		
		num_pids_smited++;
		smited_pids[num_pids_smited-1] = (long)get_task_pid(task, PIDTYPE_PID);// record the pid
		pid_states[num_pids_smited-1] = task->state;	// record the original state
		set_task_state(task, TASK_STOPPED);  			// SMITE!
	}

	
	// pass information back to the invoking user space call by filling the given pointers.
	if (   (copy_to_user(p_num_pids_smited, &num_pids_smited, sizeof(int)) != 0)
		|| (copy_to_user(p_smited_pids, &smited_pids, num_pids_smited*sizeof(int*)) != 0)
		|| (copy_to_user(p_pid_states, &pid_states, num_pids_smited*sizeof(long*))) != 0){
			printk(KERN_INFO "\"SMITE ERROR: Invalid return pointers.\" -- Smiter\n");
			return EFAULT;
	}
	
	return 0;
}

// Our new kernel module function for UNSMITING
asmlinkage long (*ref_sys_cs3013_syscall2)(void); // store the old one
asmlinkage long new_sys_cs3013_syscall2(int* p_num_pids_smited, int* p_smited_pids, long* p_pid_states) {
	// check validity of arguments passed by caller.
	// If any of them can't copy any bytes (didn't return 0 bytes not copied), return an error code.
	// Note the order for short-circuit of the if statement
	if (   (copy_from_user(&num_pids_smited, p_num_pids_smited, sizeof(int)) != 0)
		|| (copy_from_user(&smited_pids, p_smited_pids, num_pids_smited*sizeof(int*)) != 0)
		|| (copy_from_user(&pid_states, p_pid_states, num_pids_smited*sizeof(long*))) != 0){
			printk(KERN_INFO "\"UNSMITE ERROR: Invalid argument pointers.\" -- Unsmiter\n");
			return EFAULT;
	}
	
	
	printk(KERN_INFO "\"Un-Smiting processes.\" -- Unsmiter\n");
	
	// loop through the smited processes, unsmite them
	while (num_pids_smited>0){
		// This might work...still confusion on PIDs vs pid structures and which functions are best to use
		// (get_pid, get_pid_ns, find_get_pid, pid_task, 
		
		// http://tuxthink.blogspot.com/2012/07/module-to-find-task-from-its-pid.html
		rcu_read_lock();
		pid_struct = find_get_pid((pid_t)smited_pids[num_pids_smited-1]); // find the pid struct from the pid number
		rcu_read_unlock();
		task = pid_task(pid_struct, PIDTYPE_PID); 	//get the task_struct from the pid struct.
		
		//pid_struct = pid_task(find_vpid(pid), PIDTYPE_PID); // http://stackoverflow.com/questions/8547332/kernel-efficient-way-to-find-task-struct-by-pid
		
		//pid_struct =  find_pid((pid_t)smited_pids[num_pids_smited-1])
		
		task->state = pid_states[num_pids_smited-1];		// reset to previous state
		num_pids_smited--;
	}

	return 0;
}

// Finds the address of the system call table so we can replace some entries with our own functions.
// Don't need to modify
static unsigned long **find_sys_call_table(void) {
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;
	while (offset < ULLONG_MAX) {
		sct = (unsigned long **) offset;
		if (sct[__NR_close] == (unsigned long *) sys_close) {
			printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX",
					(unsigned long) sct);
			return sct;
		}
		offset += sizeof(void *);
	}
	return NULL;
}

//Use just before a system call table modification
//Don't need to modify
static void disable_page_protection(void) {
	/*
	 Control Register 0 (cr0) governs how the CPU operates.
	 Bit #16, if set, prevents the CPU from writing to memory marked as
	 read only. Well, our system call table meets that description.
	 But, we can simply turn off this bit in cr0 to allow us to make
	 changes. We read in the current value of the register (32 or 64
	 bits wide), and AND that with a value where all bits are 0 except
	 the 16th bit (using a negation operation), causing the write_cr0
	 value to have the 16th bit cleared (with all other bits staying
	 the same. We will thus be able to write to the protected memory.
	 It’s good to be the kernel!
	 */
	write_cr0(read_cr0() & (~0x10000));
}

//Use just after a system call table modification
//Don't need to modify
static void enable_page_protection(void) {
	/*
	 See the above description for cr0. Here, we use an OR to set the
	 16th bit to re-enable write protection on the CPU.
	 */
	write_cr0(read_cr0() | 0x10000);
}

//finds the system call table, saves the address of the existing cs3013 syscall1
//in a pointer, disables the paging protections, replaces the cs3013 syscall1’s entry in the page table with a
//pointer to our new function, then reenables the page protections and prints a note to the kernel system log
static int __init interceptor_start(void) {
	/* Find the system call table */
	if(!(sys_call_table = find_sys_call_table())) {
		/* Well, that didn’t work.
		 Cancel the module loading step. */
		 printk(KERN_INFO "\"Smite Module couldn't find the sys call table!\r\n");
		return -1;
	}
	/* Store a copy of all the existing functions */
	ref_sys_cs3013_syscall1 = (void *)sys_call_table[__NR_cs3013_syscall1];
	ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];
	
	/* Replace the existing system calls */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)new_sys_cs3013_syscall1;
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)new_sys_cs3013_syscall2;
	enable_page_protection();
	
	/* And indicate the load was successful */
	printk(KERN_INFO "\"Loaded interceptors!\"\r\n");
	return 0;
}

//reverts the changes of the interceptor start function. It
//uses the saved pointer value for the old cs3013 syscall1 and puts that back in the system call table in the
//right array location
static void __exit interceptor_end(void) {
	/* If we don’t know what the syscall table is, don’t bother. */
	if(!sys_call_table){
		printk(KERN_INFO "\"Smite Loader Module Unload Failed!\"\r\n");
		return;
	}
	/* Revert all system calls to what they were before we began. */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall1] = (unsigned long *) ref_sys_cs3013_syscall1;
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *) ref_sys_cs3013_syscall2;
	enable_page_protection();
	
	printk(KERN_INFO "\"Unloaded interceptor!\"\r\n");
}

MODULE_LICENSE("GPL");
module_init( interceptor_start);
module_exit( interceptor_end);
