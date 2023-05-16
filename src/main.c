#include "../include/child.h"
#include "../include/common.h"

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define INITIAL_VALUE 0

int N_lines;
int file_number_of_lines;    // Number of lines in the file
int number_of_segments;      // Number of segments

/* Count the lines of the given file */
static int count_lines(FILE* file){
    int number_of_lines = 0;
    int ch;

    while ((ch = getc(file)) != EOF) {
        if (ch == '\n')
            ++number_of_lines;
    }

    rewind(file);           // Reset the pointer to the start of the file
    return number_of_lines;
}

/* Generate unique names for each semaphore */
static void namegen(char *dst, int suffix){
    sprintf(dst, "/a-semaphores%u", suffix);
}

/* Add segment to shared memory */
void get_segment(FILE* file, int seg_number, SharedMemory* shared_mem) {
    int start = seg_number * N_lines + 1;  // Convert segment number to one-based indexing
    int end = min(start + N_lines, file_number_of_lines);

    // Move to the starting line in the file
    fseek(file, 0, SEEK_SET);
    for (int i = 1; i < start; i++) {
        if (fgets(shared_mem->shared_segment, MAX_LINE_SIZE, file) == NULL) {
            // Handle error or end of file condition
            break;
        }
    }

    // Read lines into shared memory segment
    int line_idx = 0;
    for (int i = start; i <= end; i++) {
        if (fgets(shared_mem->shared_segment + line_idx, MAX_LINE_SIZE, file) == NULL) {
            // Handle error or end of file condition
            break;
        }
        line_idx += MAX_LINE_SIZE;
    }

    // Check if there is enough space for the null-terminating character
    if (line_idx < MAX_LINES_PER_SEG * MAX_LINE_SIZE) {
        shared_mem->shared_segment[line_idx] = '\0';
    } else {
        printf("Error: Not enough space in shared_segment for null-terminating character\n");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char** argv) {
    srand(time(NULL));	                        // Seed the random number generator

    /* Read arguments from command line */
    if (argc != 5) {      /* Error check */
        printf("Give <file.txt> <N> <K> <M_requests>\n");
        exit(EXIT_FAILURE);  
    }

    FILE* readfile;
    N_lines = atoi(argv[2]);                    // Number of lines in segment
    int K_children = atoi(argv[3]);             // Number of children
    int M_requests = atoi(argv[4]);             // Number of max child requests


    /* ---------------------------------------- | FILE OPERATIONS | ---------------------------------------- */
	readfile = fopen(argv[1], "r");

    /* Check if the opening went correctly */
	if (!readfile){
		printf("Could not open file");
        exit(EXIT_FAILURE);
    }

    /* Count the number of lines in the file */
    file_number_of_lines = count_lines(readfile);

    if(file_number_of_lines < 1000) {
        printf("We need a txt with more than 1000 lines!\n");
        exit(EXIT_FAILURE);  
    }

    /* Calculate the number of segments */
    number_of_segments = ceil((double)file_number_of_lines / (double)N_lines);  

    /* ---------------------------------------- | SHARED MEMORY | ---------------------------------------- */
    key_t key;
    int shmid;
    SharedMemory* shared_mem;

    /* Make the key */
    if ((key = ftok(argv[1], 1)) == -1){ 
        perror("ftok");
        exit(1);
    } 

    /*  Create the segment */
    if ((shmid = shmget(key, sizeof(SharedMemory), 0666 | IPC_CREAT)) == -1) {  
        perror("shmget");
        exit(1);
    }
    
    /* Attach to the segment to get a pointer to it */
    if ((void *)(shared_mem = (SharedMemory *)shmat(shmid, NULL, 0)) == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    /* ---------------------------------------- | SEMAPHORES | ---------------------------------------- */

    /* Initialize as many semaphores as there are segments */
    sem_t* seg_semaphores[number_of_segments];

    for(int i = 0; i < number_of_segments; i++){
        char name[128];
        namegen(name, i);       // Give a unique name to each semaphore
        seg_semaphores[i] = sem_open(name, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);

        if (seg_semaphores[i] == SEM_FAILED){
            perror("sem_open(3) failed");
            exit(EXIT_FAILURE);
        }
    }

    /* -------------------------------------- | CHILD OPERATIONS | ------------------------------------- */
    pid_t child_ids[K_children];

    /* Initialize K children */
    for(int i = 0; i < K_children; i++){
        if ((child_ids[i] = fork()) < 0) {
            perror("fork(2) failed");
            exit(EXIT_FAILURE);
        }

        if (child_ids[i] == 0) {
            /* child process */
            child(seg_semaphores, M_requests, shared_mem);
            exit(0);        // We do not want the children to call the fork() function as well
        }
    }

    int nxt_seg = 0;        
    int children_finished = 0;
    while(1) {
        shared_mem->child_finished = 0;         // Collect how many children finished in each loop

        /* Add segment to shared memory */
        get_segment(readfile, nxt_seg, shared_mem);
        int value; 

        do {
            sem_getvalue(seg_semaphores[nxt_seg], &value);
            if (value > 0) {
                break;
            }
            sem_post(seg_semaphores[nxt_seg]);
            sleep(0.02);
        } while (1);
    
        sem_wait(seg_semaphores[nxt_seg]);
        nxt_seg = (nxt_seg + 1) % number_of_segments;  // Increment and wrap around, starting from 1 for one-based segments

        children_finished += shared_mem->child_finished;

        if (children_finished == K_children) {
            break;
        } 
    }
    
    /* Detach from the memory segment */
    if (shmdt(shared_mem) == -1) {
        perror("shmdt");
        exit(1);
    }

    /* Remove the shared memory segment from the system */
    if(shmctl(shmid, IPC_RMID, 0) == -1){
        perror("shmctl");
        exit(1);
    }

    /* Cleanup semaphore objects */
    for (int i = 0; i < number_of_segments; i++) {
        if (sem_close(seg_semaphores[i]) < 0) {
            perror("sem_close(0) failed");
            exit(EXIT_FAILURE);
        }

        char name[128];
        namegen(name, i);

        if (sem_unlink(name) < 0) {
            perror("sem_unlink(0) failed");
            exit(EXIT_FAILURE);
        }
    }

    /* Close file */
    fclose(readfile);
    
    return 0;
}
