/* Test system calls for Project 2
*/
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// These values MUST match the unistd_32.h modifications:
#define __NR_cs3013_syscall1 355
#define __NR_cs3013_syscall2 356
#define __NR_cs3013_syscall3 357

long testCall1 (unsigned short* p_target_uid, int* p_num_pids_smited, int* p_smited_pids, long* p_pid_states) {
	return (long) syscall(__NR_cs3013_syscall1, p_target_uid, p_num_pids_smited, p_smited_pids, p_pid_states);
}
long testCall2 (int* p_num_pids_smited, int* p_smited_pids, long* p_pid_states) {
	return (long) syscall(__NR_cs3013_syscall2, p_num_pids_smited, p_smited_pids, p_pid_states);
}
long testCall3 ( void) {
	return (long) syscall(__NR_cs3013_syscall3);
}

// Main test program
#define n_processes 100
int main(int argc, const char* argv[]){
	printf("Testing system calls...!\n");
	printf("The return values of the system calls are:\n");
	
	unsigned short target_uid = 1;
	int num_pids_smited=0;
	int smited_pids[n_processes];
	long pid_states[n_processes];
	
	if (0 == printf("\tcs3013_syscall1: %ld\n", testCall1(&target_uid, &num_pids_smited, smited_pids, pid_states))){
		printf("\tcs3013_syscall2: %ld\n", testCall2(&num_pids_smited, smited_pids, pid_states));
	}else{
		printf("Not calling syscall2 because syscall1 failed.\n");
	}
	printf("\tcs3013_syscall3: %ld\n", testCall3());

	return 0;
}
