
/*
 Name: Andrew Hahn
 Email: ahahn1@crimson.ua.edu
 Course Section: CS 581
 Homework #: 4
 To Compile: mpicc -g -Wall -std=c99 -o hw4 hw4.c
 To Run: hw4 5000 5000 1 outputs/output.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

#define DEAD 0
#define ALIVE 1

// Function to count the number of alive neighbors for a given cell
int countAliveNeighbors(int **board, int x, int y) {
    int count = 0;
    // Loop through neighboring cells
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            // Exclude the cell itself
            if (i != x || j != y) {
                count += board[i][j];
            }
        }
    }
    return count;
}

// Function to evolve the board to the next generation
bool evolve(int **current, int **next, int rows, int cols) {
    bool changed = false; // Flag to check if the board has changed
    // Loop through each cell excluding ghost cells
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            int aliveNeighbors = countAliveNeighbors(current, i, j);
            if (current[i][j] == ALIVE) {
                // Apply the Game of Life rules
                next[i][j] = (aliveNeighbors == 2 || aliveNeighbors == 3) ? ALIVE : DEAD;
            } else {
                next[i][j] = (aliveNeighbors == 3) ? ALIVE : DEAD;
            }
            if (current[i][j] != next[i][j]) {
                changed = true; // Mark as changed if the cell state differs
            }
        }
    }
    return changed;
}

// Function to swap the current and next boards
void swapBoards(int ***board1, int ***board2) {
    int **temp = *board1;
    *board1 = *board2;
    *board2 = temp;
}

int main(int argc, char *argv[]) {
    // Initialize MPI environment
    MPI_Init(&argc, &argv);

    // Variables to store the rank and size
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Start timer
    double start_time = MPI_Wtime();

    // Check if the correct number of arguments are given
    if (argc != 5) {
        if (rank == 0) {
            printf("Usage: %s <size of board> <max generations> <number of processes> <output file path>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    // Parse command-line arguments
    int board_size = atoi(argv[1]);
    int maxGenerations = atoi(argv[2]);
    int numProcesses = atoi(argv[3]);
    char *outputFilePath = argv[4];

    // Check if the number of processes specified matches the MPI size
    if (numProcesses != size) {
        if (rank == 0) {
            printf("Error: Number of processes specified (%d) does not match number of MPI processes (%d)\n", numProcesses, size);
        }
        MPI_Finalize();
        return 1;
    }

    // Calculate rows per process and handle any remainder
    int rows_per_process = board_size / size;
    int remainder = board_size % size;

    // Arrays to hold the count of rows and displacements for each process
    int *sendcounts = (int *)malloc(size * sizeof(int));
    int *displs = (int *)malloc(size * sizeof(int));

    // Compute send counts and displacements
    for (int i = 0; i < size; i++) {
        sendcounts[i] = rows_per_process;
        if (i < remainder) {
            sendcounts[i]++; // Distribute the remainder
        }
    }

    displs[0] = 0; // First displacement is zero
    for (int i = 1; i < size; i++) {
        displs[i] = displs[i - 1] + sendcounts[i - 1];
    }

    // Determine the global start and end rows for each process
    int start_row = displs[rank] + 1; // Adding 1 for ghost cell
    int local_rows = sendcounts[rank];
    // int end_row = start_row + local_rows - 1;

    // Local board dimensions including ghost cells
    int local_board_rows = local_rows + 2; // Adding 2 for ghost rows
    int local_board_cols = board_size + 2; // Adding 2 for ghost columns

    // Allocate local boards dynamically with ghost cells
    int **current = (int **)malloc(local_board_rows * sizeof(int *));
    int **next = (int **)malloc(local_board_rows * sizeof(int *));
    for (int i = 0; i < local_board_rows; i++) {
        current[i] = (int *)malloc(local_board_cols * sizeof(int));
        next[i] = (int *)malloc(local_board_cols * sizeof(int));
    }

    // Initialize local boards to DEAD state
    for (int i = 0; i < local_board_rows; i++) {
        for (int j = 0; j < local_board_cols; j++) {
            current[i][j] = DEAD;
            next[i][j] = DEAD;
        }
    }

    // Prepare sendcounts and displacements for MPI_Scatterv
    int *sendcounts_bytes = (int *)malloc(size * sizeof(int));
    int *displs_bytes = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        sendcounts_bytes[i] = sendcounts[i] * board_size; // Number of elements
    }
    displs_bytes[0] = 0;
    for (int i = 1; i < size; i++) {
        displs_bytes[i] = displs_bytes[i - 1] + sendcounts_bytes[i - 1];
    }

    int *sendbuf = NULL;
    if (rank == 0) {
        // Initialize the full board with random values
        int full_board_rows = board_size + 2;
        int full_board_cols = board_size + 2;
        int **full_board = (int **)malloc(full_board_rows * sizeof(int *));
        for (int i = 0; i < full_board_rows; i++) {
            full_board[i] = (int *)malloc(full_board_cols * sizeof(int));
        }
        // Initialize the full board to DEAD
        for (int i = 0; i < full_board_rows; i++) {
            for (int j = 0; j < full_board_cols; j++) {
                full_board[i][j] = DEAD;
            }
        }
        // Seed the random number generator
        srand(52);
        // Initialize the inner cells randomly
        for (int i = 1; i <= board_size; i++) {
            for (int j = 1; j <= board_size; j++) {
                full_board[i][j] = rand() % 2;
            }
        }
        // Flatten the data into sendbuf
        int total_elements = board_size * board_size;
        sendbuf = (int *)malloc(total_elements * sizeof(int));
        int index = 0;
        for (int i = 1; i <= board_size; i++) {
            for (int j = 1; j <= board_size; j++) {
                sendbuf[index++] = full_board[i][j];
            }
        }
        // Free the full_board as we don't need it anymore
        for (int i = 0; i < full_board_rows; i++) {
            free(full_board[i]);
        }
        free(full_board);
    }

    // Each process allocates recvbuf to receive its portion
    int recv_elements = sendcounts[rank] * board_size; // Number of elements to receive
    int *recvbuf = (int *)malloc(recv_elements * sizeof(int));

    // MPI_Scatterv to distribute the data
    MPI_Scatterv(sendbuf, sendcounts_bytes, displs_bytes, MPI_INT,
                 recvbuf, recv_elements, MPI_INT, 0, MPI_COMM_WORLD);

    // Copy recvbuf into current
    int index = 0;
    for (int i = 1; i <= local_rows; i++) {
        for (int j = 1; j <= board_size; j++) {
            current[i][j] = recvbuf[index++];
        }
    }

    // Free temporary buffers
    free(recvbuf);
    free(sendcounts_bytes);
    free(displs_bytes);
    if (rank == 0) {
        free(sendbuf);
    }

    // Determine neighboring ranks for communication
    int up = rank - 1;
    int down = rank + 1;
    if (up < 0) up = MPI_PROC_NULL; // No neighbor above
    if (down >= size) down = MPI_PROC_NULL; // No neighbor below

    bool changed = true;
    int generation = 0;
    bool global_changed = true;

    // Main simulation loop
    while (generation < maxGenerations && global_changed) {
        // Exchange ghost rows with neighboring processes
        // Send first data row upwards, receive from down into bottom ghost row
        MPI_Sendrecv(current[1], local_board_cols, MPI_INT, up, 0,
                     current[local_board_rows - 1], local_board_cols, MPI_INT, down, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Send last data row downwards, receive from up into top ghost row
        MPI_Sendrecv(current[local_board_rows - 2], local_board_cols, MPI_INT, down, 1,
                     current[0], local_board_cols, MPI_INT, up, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Evolve the board to the next generation
        changed = evolve(current, next, local_board_rows, local_board_cols);

        // Swap the current and next boards
        swapBoards(&current, &next);

        // Check for global convergence using MPI_Allreduce
        int local_changed = changed ? 1 : 0;
        int sum_changed;
        MPI_Allreduce(&local_changed, &sum_changed, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        global_changed = (sum_changed > 0);

        generation++;
    }

    // Prepare to gather the final board at rank 0
    int *sendbuf_gather = (int *)malloc(local_rows * board_size * sizeof(int));
    // Flatten the local board data into send buffer
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < board_size; j++) {
            sendbuf_gather[i * board_size + j] = current[i + 1][j + 1];
        }
    }

    int *recvcounts = NULL;
    int *recvdispls = NULL;
    int *recvbuf_gather = NULL;

    // Rank 0 prepares receive buffers
    if (rank == 0) {
        recvcounts = (int *)malloc(size * sizeof(int));
        recvdispls = (int *)malloc(size * sizeof(int));

        for (int i = 0; i < size; i++) {
            recvcounts[i] = sendcounts[i] * board_size;
        }

        recvdispls[0] = 0;
        for (int i = 1; i < size; i++) {
            recvdispls[i] = recvdispls[i - 1] + recvcounts[i - 1];
        }

        recvbuf_gather = (int *)malloc(board_size * board_size * sizeof(int));
    }

    // Gather the final board at rank 0
    /* MPI_Gatherv(sendbuf_gather, local_rows * board_size, MPI_INT,
                recvbuf_gather, recvcounts, recvdispls, MPI_INT,
                0, MPI_COMM_WORLD);

    // Rank 0 writes the final board to the output file
    if (rank == 0) {
        FILE *fp = fopen(outputFilePath, "w");
        if (fp == NULL) {
            printf("Error opening output file %s\n", outputFilePath);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        // Write the final board to the file
        int index = 0;
        for (int i = 0; i < board_size; i++) {
            for (int j = 0; j < board_size; j++) {
                fprintf(fp, recvbuf_gather[index++] == ALIVE ? "O " : ". ");
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
        free(recvcounts);
        free(recvdispls);
        free(recvbuf_gather);
    }*/

    // Free dynamically allocated memory
    free(sendcounts);
    free(displs);
    free(sendbuf_gather);

    for (int i = 0; i < local_board_rows; i++) {
        free(current[i]);
        free(next[i]);
    }
    free(current);
    free(next);

    // End timer and print execution time
    double end_time = MPI_Wtime();
    if (rank == 0) {
        printf("Program exited after %d generations.\n", generation);
        printf("Time taken: %f seconds\n", end_time - start_time);
    }

    // Finalize MPI environment
    MPI_Finalize();

    return 0;
}
