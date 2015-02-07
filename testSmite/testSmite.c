/* Test system calls for Project 2
We spawn just over 100 child processes, tell them all to sit there churning away for a bit (twiddle), bu then immediately then smite 100 of them .
We then sleep ourselves for 5 seconds, and unsmite them.
Their total runtime should be greater than 5 seconds, showing they were smited and could twiddle..
*/

#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
//#define __USE_BSD
#include <time.h>
#include <sys/resource.h>
#include <string.h>
#include <limits.h>

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

long getTime(){
	struct timeval curTime;
	gettimeofday(&curTime, NULL);
	return (curTime.tv_sec*1000 + curTime.tv_usec/1000);
}

// wait for any children spawned by this process to finish, print their stats
#define NUM_CHILDREN 105 // test with 105 child processes. 
long startTime;
void waitForChildrenToFinish(){
	printf("Hanging for child processes...\n");
	int status;
	struct rusage current_usage;
	int pid;
	int numChildrenDead = 0;
	pid = wait4((pid_t)-1, &status, WNOHANG, &current_usage); // don't hang or wait for all children to finish
	while(pid !=0){ //Evaluates to a non-zero value if status was returned for a child process that terminated normally.
		if (pid <0 ){
			printf("\n\twait() found no children to wait for\n");
			break;
		}
		printf("\npid:%d for status:%d WIFSTATUS:%d, WEXITSTATUS:%d\n", pid, status, WIFEXITED(status), WEXITSTATUS(status));

		if (WIFEXITED(status) != 0){
			if (WEXITSTATUS(status) != 0){
				printf("\tBackground child process execution failed. (Invalid Command?)\n");
			}else{
				printf("\tBackground child process %d exited normally.\n", pid);
				printf("\tChild ran for %lu ms\n", (getTime()-startTime));
				numChildrenDead++;
			}
		}else{
			if (pid != -1){
				printf("\tChild process %d terminated.\n", pid);
			}else{
				printf("\tERROR: pid = -1 for correctly exited process,\n");
			}
		}
		pid = wait4((pid_t)-1, &status, WNOHANG, &current_usage); // don't hang (don't wait for all children to finish)
	}
	printf("\tFinished waiting for all %d children to die.\n", numChildrenDead);
}

// fork off hundreds of children processes for testing.
#define TARGET_UID 1001 // NOTE: There must be a user 1001 to target on the system!
void rabbit(){
	// fork off a hundreds of children to see which ones get smited, yay!!!

	char* strCommand = "sleep";
	char* strSleepTime ="2";
	char* strArgumentsList[] = {strCommand, strSleepTime, NULL};
	
	int p=0;
	long pid;
	printf("Spawning children...\n");
	startTime = getTime();
	for (p=0;p<NUM_CHILDREN;p++){
		pid = fork();
		if (pid==0){ //if we're the child
			setuid(TARGET_UID); // make the children's user be the target user.
			
			// twiddle
			/*int run;
			unsigned long i = LONG_MAX;
			for (i=0;i<UI_MAX;i++){
				char* myString = "Weeeeee!!!!!!";
				strcmp(myString, "Weeeeee!!!!!nope");
			}
			*/
			//printf("Child process donw twiddling\n");
			//exit(0);			
			execvp("sleep", strArgumentsList);
		}else{
			printf("\tchild pid %lu\n", pid);
		}
	}
	printf("This parent is pid %d\n", getpid());
}

// Main test program
#define n_processes 100
int main(int argc, const char* argv[]){
	printf("Testing system calls...!\n");
	printf("The return values of the system calls are:\n");
	
	unsigned short target_uid = TARGET_UID;
	int num_pids_smited=0;
	int smited_pids[n_processes];
	long pid_states[n_processes];
	
	rabbit();
	
	long syscall1result = testCall1(&target_uid, &num_pids_smited, smited_pids, pid_states); // SMITE
	printf("\tcs3013_syscall1: %ld\n", syscall1result);
	if (syscall1result == 0){
		printf("\n\nThere were %d processes smited:\n", num_pids_smited);
		int i = 0;
		for (i=0;i<num_pids_smited;i++){
			printf("\tpid %d\tstate %lu\n",smited_pids[i], pid_states[i]);
		}
		
		printf("Letting the smitten sleep...\n");
		sleep(5); // sleep for a bit while the smitten process are stopped
		
		printf("\tcs3013_syscall2: %ld\n", testCall2(&num_pids_smited, smited_pids, pid_states)); // UN-SMITE
	}else{
		printf("Not calling syscall2 because syscall1 failed.\n");
	}
	printf("\tcs3013_syscall3: %ld\n", testCall3());

	// wait for the hundreds of children to die
	waitForChildrenToFinish();
	
	return 0;
}
