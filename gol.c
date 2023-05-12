/*
 * The Game of Life
 *
 * RULES:
 *  1. A cell is born, if it has exactly three neighbours.
 *  2. A cell dies of loneliness, if it has less than two neighbours.
 *  3. A cell dies of overcrowding, if it has more than three neighbours.
 *  4. A cell survives to the next generation, if it does not die of lonelines
 * or overcrowding.
 *
 * In this version, a 2D array of ints is used.  A 1 cell is on, a 0 cell is
 * off. The game plays a number of steps (given by the input), printing to the
 * screen each time. A 'x' printed means on, space means off.
 *
 */

#include "gol.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

/* Statistics */
stats_t statistics;

cell_t **allocate_board(int size) {
    cell_t **board = (cell_t **)malloc(sizeof(cell_t *) * size);
    int i;
    for (i = 0; i < size; i++)
        board[i] = (cell_t *)malloc(sizeof(cell_t) * size);

    statistics.borns = 0;
    statistics.survivals = 0;
    statistics.loneliness = 0;
    statistics.overcrowding = 0;

    return board;
}

void free_board(cell_t **board, int size) {
    int i;
    for (i = 0; i < size; i++)
        free(board[i]);
    free(board);
}

int adjacent_to(cell_t **board, int size, int i, int j) {
    int k, l, count = 0;

    int sk = (i > 0) ? i - 1 : i;
    int ek = (i + 1 < size) ? i + 1 : i;
    int sl = (j > 0) ? j - 1 : j;
    int el = (j + 1 < size) ? j + 1 : j;

    for (k = sk; k <= ek; k++)
        for (l = sl; l <= el; l++)
            count += board[k][l];
    count -= board[i][j];

    return count;
}
typedef struct {
    int size, start, jump;
    cell_t **board;
    cell_t **newboard;
} thread_args_t;

void *thread(void *args) {
    thread_args_t *params = (thread_args_t *)args;
    stats_t *thread_stats = (stats_t *)malloc(sizeof(stats_t));
    thread_stats->borns = 0;
    thread_stats->survivals = 0;
    thread_stats->loneliness = 0;
    thread_stats->overcrowding = 0;

    int i = params->start / params->size, j = params->start % params->size, a,
        counter = params->start;
    while (i < params->size) {
        
        a = adjacent_to(params->board, params->size, i, j);
        /* if cell is alive */
        if (params->board[i][j]) {
            /* death: loneliness */
            if (a < 2) {
                params->newboard[i][j] = 0;
                thread_stats->loneliness++;
            } else {
                /* survival */
                if (a == 2 || a == 3) {
                    params->newboard[i][j] = params->board[i][j];
                    thread_stats->survivals++;
                } else {
                    /* death: overcrowding */
                    if (a > 3) {
                        params->newboard[i][j] = 0;
                        thread_stats->overcrowding++;
                    }
                }
            }

        } else /* if cell is dead */
        {
            if (a == 3) /* new born */
            {
                params->newboard[i][j] = 1;
                thread_stats->borns++;
            } else /* stay unchanged */
                params->newboard[i][j] = params->board[i][j];
        }
        counter += params->jump;
        i = counter / params->size;
        j = counter % params->size;
    }

    free(args);
    pthread_exit((void *)thread_stats);
}

stats_t play(cell_t **board, cell_t **newboard, int size, int threads) {
    stats_t stats = {0, 0, 0, 0};
    pthread_t ths[threads];
    if (threads > size*size) {
        threads = size*size;
    }

    // Start threads with params
    for (size_t k = 0; k < threads; k++) {
        thread_args_t *params = (thread_args_t *)malloc(sizeof(thread_args_t));
        params->size = size;
        params->board = board;
        params->newboard = newboard;
        params->size = size;
        params->start = k;
        params->jump = threads;
        pthread_create(&ths[k], NULL, thread, (void *)params);
    }

    for (size_t k = 0; k < threads; k++) {
        void *thread_stats;
        pthread_join(ths[k], &thread_stats);
        stats.borns += ((stats_t *)thread_stats)->borns;
        stats.survivals += ((stats_t *)thread_stats)->survivals;
        stats.loneliness += ((stats_t *)thread_stats)->loneliness;
        stats.overcrowding += ((stats_t *)thread_stats)->overcrowding;
        free(thread_stats);
    }

    return stats;
}

void print_board(cell_t **board, int size) {
    int i, j;
    /* for each row */
    for (j = 0; j < size; j++) {
        /* print each column position... */
        for (i = 0; i < size; i++)
            printf("%c", board[i][j] ? 'x' : ' ');
        /* followed by a carriage return */
        printf("\n");
    }
}

void print_stats(stats_t stats) {
    /* print final statistics */
    printf("Statistics:\n\tBorns..............: %u\n\tSurvivals..........: "
           "%u\n\tLoneliness deaths..: %u\n\tOvercrowding deaths: %u\n\n",
           stats.borns, stats.survivals, stats.loneliness, stats.overcrowding);
}

void read_file(FILE *f, cell_t **board, int size) {
    char *s = (char *)malloc(size + 10);

    /* read the first new line (it will be ignored) */
    fgets(s, size + 10, f);

    /* read the life board */
    for (int j = 0; j < size; j++) {
        /* get a string */
        fgets(s, size + 10, f);

        /* copy the string to the life board */
        for (int i = 0; i < size; i++)
            board[i][j] = (s[i] == 'x');
    }

    free(s);
}