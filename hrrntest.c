#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 4

// CPU-bound workload function
void
cpu_work(int loops)
{
  volatile int i, j;
  for(i = 0; i < loops; i++){
    for(j = 0; j < 100000; j++){
      asm volatile("");
    }
  }
}

int
main(int argc, char *argv[])
{
  int i, pid;
  // Mix of long and short CPU-bound processes:
  // Child 1: Long job (burst = 35), created early so it accumulates wait time
  // Child 2: Very short job (burst = 5), runs 1st because short jobs are favored initially
  // Child 3: Medium-short job (burst = 25), created later; Child 1 (burst = 35) beats Child 3
  //          for 2nd place because Child 1's accumulated wait time raises its Response Ratio!
  // Child 4: Very long job (burst = 80), runs 4th
  int bursts[NUM_PROCS] = {35, 5, 25, 80};
  int delays_after_fork[NUM_PROCS] = {800, 150, 20, 20};
  int order = 1;

  printf(1, "=== Question 2: Non-Preemptive HRRN Scheduler Test ===\n");

  for(i = 0; i < NUM_PROCS; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      int mypid = getpid();
      set_burst(mypid, bursts[i]);
      cpu_work(900);
      printf(1, "[Child %d | PID %d] (burst_time = %d) finished at tick %d\n",
             i + 1, mypid, bursts[i], uptime());
      exit();
    } else {
      set_burst(pid, bursts[i]);
      printf(1, "[Parent] Created Child %d -> PID %d (burst_time = %d) at tick %d\n",
             i + 1, pid, bursts[i], uptime());
      cpu_work(delays_after_fork[i]);
    }
  }

  // Give parent burst_time = 1 so it immediately reaps each child as soon as it exits
  set_burst(getpid(), 1);

  printf(1, "\n--- Starting HRRN Scheduling ---\n");
  for(i = 0; i < NUM_PROCS; i++){
    int done_pid = wait();
    printf(1, "Completion #%d -> PID %d\n", order++, done_pid);
  }

  printf(1, "=== HRRN Test Complete ===\n");
  exit();
}
