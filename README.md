# Multi-Threaded-DMV
DMV Simulation using multiple threads and semaphores for concurrency

## How to compile and Run
You'll compile this differently depending on which system you're on.

Note, that this project can't run on windows because Windows does not support POSIX threads and semaphores.

If you're on macOS, you can just run `gcc main.c -o main` to compile and use `./main` to execute the program.

If you're on linux, use `gcc -pthread main.c -o main -std=c99` to compile and use `./main` to execute the program.
