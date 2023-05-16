#include "../include/child.h"

int N_lines;                 // Number of lines in segment
int file_number_of_lines;    // Number of lines in the file
int number_of_segments;      // Number of segments

/* Get the desired line for the next request */
static int get_desired_line(int prev_line) {
    int new_line;
    if (prev_line == -1) {                                  // In first request, return random line 
        new_line = rand() % file_number_of_lines + 1;      
    } else {
        int prev_seg_num = prev_line / N_lines;
        int prev_seg_min_idx = prev_seg_num * N_lines + 1;

        if (rand() % 10 + 1 <= 7) { // 0.7 prob
            /* Generate new line in the segment of the previous line */
            if (prev_seg_num == number_of_segments - 1){
                int lines_in_last_seg = file_number_of_lines - (prev_seg_num * N_lines);
                new_line = rand() % lines_in_last_seg + prev_seg_min_idx + 1;
            } else {
                new_line = rand() % N_lines + prev_seg_min_idx + 1;
            }
        } else { // 0.3 prob, choose new segment
            int random_seg;
            /* Generate new segment number different from the segment of previous line */
            while ((random_seg = (rand() % number_of_segments)) == prev_seg_num);
            /* Generate a new line in the new segment */
            if (random_seg == number_of_segments - 1) {
                int lines_in_last_seg = file_number_of_lines - (random_seg * N_lines);
                new_line = rand() % lines_in_last_seg + random_seg * N_lines + 1;
            } else {
                new_line = rand() % N_lines + random_seg * N_lines + 1;
            }
        }    
    }
    new_line = min(new_line, file_number_of_lines - 1);
    return new_line;
}

/* Open a file for logging output */
static FILE* open_file(int pid) {
	char fileoutputname[15];

	sprintf(fileoutputname, "file[%d]", pid);
	return fopen(fileoutputname, "a");      // Open a file in append mode
}

void child(sem_t* seg_semaphores[], int M_requests, SharedMemory* shared_mem){
    srand(getpid());

    int requests = 0;                       // Initially the child has zero requests
    int desired_line = -1;

    FILE* writefile;
    writefile = open_file(getpid());
    if (!writefile)
		exit(EXIT_FAILURE);
    
    while(requests++ < M_requests) {

        /* Choose a random line */
        desired_line = get_desired_line(desired_line);

        /* Find the segment that this line belongs to */
        int seg_number = desired_line / N_lines;  

        /* Convert file_line to segment_line */
        int line_in_seg = desired_line % N_lines; 

        /* Write request in file */      
        fprintf(writefile, "Requesting: <%d,%d>\n", seg_number, line_in_seg);
        
        /* Child waits at the semaphore relevant to the desired segment */
        clock_t start = clock();
        fprintf(writefile, "Request start time: %ld\n", start);
        if(sem_wait(seg_semaphores[seg_number]) < 0){
            perror("sem_wait() failed");
            exit(EXIT_FAILURE);
        }

        clock_t stop = clock();
        fprintf(writefile, "Request finish time: %ld\n", stop);
        double elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
        fprintf(writefile, "Time elapsed in ms: %.3f\n", elapsed);

        /* Write given line in file */
        fprintf(writefile, "Answer: %s\n", shared_mem->shared_segment + MAX_LINE_SIZE * (line_in_seg - 1));
    }
    
    shared_mem->child_finished += 1;

    fclose(writefile);
}