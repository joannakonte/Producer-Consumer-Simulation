# Producer-Consumer Simulation
This project implements a simulation of a producer-multiple consumer problem, where multiple consumers request random lines from a file. The system consists of a parent process and multiple child processes. The parent process spawns `K_children` number of child processes, where `K_children` is provided by the user. The segments of the file are sequentially stored in shared memory by the parent process. The parent process waits for all child processes to finish processing the current segment before proceeding to the next segment.

Each child process randomly selects a line from a segment and waits for that segment to be stored in shared memory before retrieving it. Each child process can make up to `M_requests` requests. The probability of requesting a line from a new segment is 0.3, while the probability of requesting a line from the previously requested segment is 0.7. The child processes log their requests, submission time, response time, and acquired lines in individual files.

## Usage
Use the `Makefile` to compile, run and clean using the following commands:

```bash
$ make 
$ make run
$ make clean
```
**Note:** `make run`, does the following: `./main $(INPUT)/many_strings.txt 100 10 10`

To customize the program's arguments, modify the **ARG** variable in the Makefile with your desired values. The available command-line arguments are:

```bash
$ ./main <file.txt> <N_lines> <K_children> <M_requests>
```

- **N_lines:** Number of lines in segment
- **K_children:** Number of children
- **M_requests:** Maximum number of child requests

Please note that this project has been tested and validated on Linux systems.

