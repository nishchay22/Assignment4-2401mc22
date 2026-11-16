#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 3

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
  // Staggered arrivals with decreasing burst times:
  // Child 1 arrives first with burst = 90 (long job)
  // Child 2 arrives later with burst = 35 (medium job -> preempts Child 1)
  // Child 3 arrives last  with burst = 10 (short job  -> preempts Child 2)
  int bursts[NUM_PROCS] = {90, 35, 10};
  int work_loops[NUM_PROCS] = {1600, 700, 250};
  int order = 1;

  printf(1, "=== Question 1: Preemptive SRTF Scheduler Test ===\n");

  // Keep parent at remaining_time = 1 so it can wake up on staggered intervals
  // to fork the next arriving process and immediately yield back via sleep().
  set_burst(getpid(), 1);

  for(i = 0; i < NUM_PROCS; i++){
    // Small delay loop before forking
    cpu_work(30);
    set_burst(getpid(), 1);

    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }

    if(pid == 0){
      int mypid = getpid();
      set_burst(mypid, bursts[i]);
      printf(1, "[Child %d | PID %d] Started on CPU at tick %d (declared burst_time = %d)\n",
             i + 1, mypid, uptime(), bursts[i]);
      cpu_work(work_loops[i]);
      printf(1, "[Child %d | PID %d] Finished at tick %d (initial burst_time = %d)\n",
             i + 1, mypid, uptime(), bursts[i]);
      exit();
    } else {
      set_burst(pid, bursts[i]);
      printf(1, "[Parent] Child %d (PID %d, burst_time = %d) arrived at tick %d\n",
             i + 1, pid, bursts[i], uptime());
      // Let currently running child execute for several ticks before next arrival
      if(i < NUM_PROCS - 1)
        sleep(8);
    }
  }

  printf(1, "\n--- Waiting for Completion Order (Expected: PID of burst 10, then 35, then 90) ---\n");
  for(i = 0; i < NUM_PROCS; i++){
    set_burst(getpid(), 1);
    int done_pid = wait();
    printf(1, "Completion #%d -> PID %d\n", order++, done_pid);
  }

  printf(1, "=== SRTF Test Complete ===\n");
  exit();
}
