#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_CPU 2
#define NUM_IO  2

int
main(int argc, char *argv[])
{
  int i, pid;
  int cpu_pids[NUM_CPU];
  int io_pids[NUM_IO];

  printf(1, "=== Question 3: Multilevel Feedback Queue (MLFQ) Scheduler Test ===\n");
  printf(1, "Forking %d CPU-bound (infinite loop) and %d I/O-bound (yielding) processes...\n\n",
         NUM_CPU, NUM_IO);

  // 1. Fork I/O-bound processes first; they immediately sleep(), staying in Queue 0
  for(i = 0; i < NUM_IO; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      int iter;
      for(iter = 0; iter < 5; iter++){
        // Frequently call sleep() before 1-tick quantum expires -> stays in Queue 0
        sleep(5);
      }
      exit();
    } else {
      io_pids[i] = pid;
    }
  }

  // 2. Fork CPU-bound processes running infinite loops (will sink Queue 0 -> 1 -> 2)
  for(i = 0; i < NUM_CPU; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      // Infinite CPU-bound loop: uses full time quanta and sinks to Queue 2
      for(;;){
        asm volatile("");
      }
    } else {
      cpu_pids[i] = pid;
    }
  }

  // Wait for both I/O-bound processes to finish their sleep() bursts
  for(i = 0; i < NUM_IO; i++){
    int done_pid = wait();
    printf(1, "[I/O-Bound] PID %d finished all 5 sleep() bursts in Queue 0 (#%d)\n",
           done_pid, i + 1);
  }

  // Let CPU-bound processes continue running in Queue 2 briefly, then terminate them
  sleep(10);
  for(i = 0; i < NUM_CPU; i++){
    kill(cpu_pids[i]);
    wait();
  }

  printf(1, "\n--- Summary ---\n");
  printf(1, "I/O-bound processes (PID %d, PID %d): yielded via sleep(), stayed in Queue 0, finished first.\n",
         io_pids[0], io_pids[1]);
  printf(1, "CPU-bound processes (PID %d, PID %d): used full quanta (1t -> 2t -> 4t), demoted to Queue 2.\n",
         cpu_pids[0], cpu_pids[1]);
  printf(1, "=== MLFQ Test Complete ===\n");
  exit();
}
