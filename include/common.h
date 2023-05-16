#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>      // for semaphores
#include <math.h>           // for ceil()

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>      // for keys
#include <sys/ipc.h>        // for shared segment & keys
#include <sys/shm.h>        // for shared segment
#include <time.h>           // time()
#include <signal.h>

#define MAX_LINE_SIZE 1000
#define MAX_SEGMENTS 100
#define MAX_LINES_PER_SEG 1000

extern int N_lines;

#define min(a,b) ((a) > (b) ? (b) : (a))

typedef struct SharedMemory {
    int child_finished;                                             // Number of child processes that have finished their requests
    char shared_segment[MAX_LINES_PER_SEG * MAX_LINE_SIZE];         // Shared memory
} SharedMemory;