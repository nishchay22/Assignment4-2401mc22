# Comparative Analysis: SRTF, HRRN, and MLFQ Schedulers in xv6 (Lab Assignment 4)

## 1. Summary of Scheduling Algorithms

| Metric / Feature | **Q1: Preemptive SRTF** | **Q2: Non-Preemptive HRRN** | **Q3: Preemptive 3-Level MLFQ** |
| :--- | :--- | :--- | :--- |
| **Scheduling Mode** | Preemptive (on timer tick if shorter remaining time arrives) | Non-Preemptive (runs selected job to completion) | Preemptive (time-sliced with variable quanta: `1, 2, 4` ticks) |
| **Requires Prior Burst Estimate?** | Yes (`set_burst(pid, bt)`) | Yes (`set_burst(pid, bt)`) | **No** (learns behavior dynamically from CPU usage) |
| **Selection Metric** | Minimum `remaining_time` | Maximum $\text{RR} = \frac{(W + B) \times 100}{B}$ | Lowest `queue_level` (`0` $\to$ `1` $\to$ `2`), Round-Robin within queue |
| **Turnaround Time (TAT)** | **Optimal (Lowest)** among all burst-based schedulers | **Low to Moderate** (slightly higher than SJF/SRTF, much better than FCFS) | **Moderate** (short jobs finish quickly in Queue 0/1; long jobs time-slice in Queue 2) |
| **Response Time** | **Excellent for short jobs**, poor for long jobs | **Poor for interactive arrivals** (must wait for current running job to finish) | **Best** (new and I/O-bound jobs always run immediately in Queue 0) |
| **Starvation Prevention** | **None** (continuous short arrivals starve long jobs indefinitely) | **Built-in** ($W$ grows linearly with ticks, raising $\text{RR}$ without bound) | **Partial** (favors I/O jobs; requires periodic priority boost if Queue 0/1 are saturated) |

---

## 2. Trade-off Analysis Across Mixed Workloads

### A. Turnaround Time (TAT)
$$\text{TAT} = T_{\text{completion}} - T_{\text{arrival}}$$
* **SRTF (Best Average Turnaround Time)**: By preempting a currently executing long job (`burst = 90`) whenever a newly arrived job has a strictly smaller remaining time (`burst = 35`, then `burst = 10`), SRTF guarantees that the shortest remaining work always finishes earliest. In `srtftest`, even though PID 4 (`burst = 90`) started first, PID 6 (`burst = 10`) and PID 5 (`burst = 35`) preempted it and completed in strict order `PID 6 -> PID 5 -> PID 4`.
* **HRRN (Balanced Turnaround Time)**: HRRN sacrifices a small amount of SJF/SRTF's theoretical minimum turnaround time whenever an older long job's response ratio overtakes a newer medium-short job. However, it still prioritizes very short bursts (`burst = 5` in `hrrntest`) immediately because $B$ is in the denominator of $\frac{W + B}{B} = 1 + \frac{W}{B}$.
* **MLFQ (Practical Turnaround Without Oracle Knowledge)**: Unlike SRTF and HRRN—which require processes to know and truthfully declare `set_burst()` in advance—MLFQ achieves near-SJF turnaround times for short jobs automatically because short jobs finish within the 1-tick (Queue 0) or 2-tick (Queue 1) quanta before sinking to Queue 2.

### B. Response Time (Interactive & I/O-Bound Workloads)
$$\text{Response Time} = T_{\text{first CPU execution}} - T_{\text{arrival}}$$
* **MLFQ (Best Response Time)**: Every new process enters **Queue 0** (highest priority, 1-tick quantum). Furthermore, interactive and I/O-bound processes (such as `sleep()`-yielding children in `mlfqtest`) relinquish the CPU before exhausting their 1-tick allotment, so they **remain pinned in Queue 0** while CPU-bound infinite loops rapidly sink to Queue 1 (after 1 tick) and Queue 2 (after 2 more ticks). Whenever an I/O-bound process wakes up, it preempts Queue 2 CPU hogs on the very next timer tick.
* **SRTF (Good for Short Jobs, Poor Without Burst Declaration)**: A newly arrived short job preempts a long job on the next timer interrupt (`trap.c`), giving sub-tick response time—but *only* if the process explicitly declares a small `burst_time`.
* **HRRN (Worst Response Time for Mixed Workloads)**: Because HRRN is **non-preemptive**, if a long CPU-bound job (`burst = 80`) is already running on the CPU when an interactive or short job arrives, the newly arrived job cannot preempt it and suffers high response delay until the running job exits or sleeps.

### C. Starvation Prevention
* **HRRN (Strongest Mathematical Guarantee)**: In HRRN, the Response Ratio is:
  $$\text{RR} = \frac{(\text{Wait Time} + \text{Burst Time}) \times 100}{\text{Burst Time}} = 100 + 100 \cdot \frac{\text{Wait Time}}{\text{Burst Time}}$$
  Because `Wait Time` (`ticks - ctime`) increases monotonically every tick a `RUNNABLE` process waits, even a process with a very large `Burst Time` eventually accumulates a higher `RR` than newly arriving shorter jobs. In `hrrntest`, PID 4 (`burst = 35`, created first) accumulated enough `wait_time` while PID 5 (`burst = 5`) ran that its `RR` exceeded PID 6 (`burst = 25`), running 2nd instead of starving.
* **SRTF (Vulnerable to Starvation)**: A steady stream of short jobs (`remaining_time = 5..10`) will perpetually preempt a partially executed long job (`remaining_time = 70`), causing indefinite starvation.
* **MLFQ**: Within each queue (`0, 1, 2`), Round-Robin tie-breaking prevents any single process from starving peers at the same level, and increasing quanta (`1, 2, 4` ticks) amortize context-switch overhead for CPU-bound jobs in Queue 2.
