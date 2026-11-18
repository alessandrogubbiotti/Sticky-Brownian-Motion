/*
 * sticky.c -- a caricature of the sticky Brownian motion.
 *
 * A random walk in continuous time on {0, 1, ..., N}, where the state k is the
 * point k/N of [0,1]. The rates are
 *
 *     0 -> 1                         N^(a-1)
 *     1 -> 0                         N^a      (the annihilation rate)
 *     k -> k+1  for 1 <= k < N       1
 *     k -> k-1  for 2 <= k <= N      1
 *
 * so that N is a reflecting end. The invariant measure is pi(0) = 1/2,
 * pi(k) = 1/(2N) for k = 1, ..., N, for every a. Time is rescaled by N^2. As N
 * grows the walk converges to a Brownian motion on [0,1], reflected at 1, which
 * at the origin is
 *
 *     a > -1    sticky;
 *     a = -1    held for an exponential time;
 *     a < -1    reflected.
 *
 * At finite N, in the rescaled time, the walk enters the origin at rate about
 * N^(a+1) and each stay lasts N^-(a+1) on average. For a < -1 the entrances are
 * rare and the stays long, and they disappear only as N grows: with N = 300,
 * a = -1.3 and T = 10 most of the trajectories still visit the origin.
 *
 * The walk is simulated with the Gillespie algorithm: the time of the next
 * jump is exponential with parameter the sum of the rates out of the current
 * state, and the jump is chosen with probability proportional to its rate.
 *
 * Compile and run:
 *
 *     cc sticky.c -o sticky -lm
 *     ./sticky               writes trajectory.dat
 *     python3 plot.py        writes trajectory.png
 *
 * The parameters are the #define below. They can also be given at compile
 * time, for instance
 *
 *     cc -DN=300 -DA=-0.5 sticky.c -o sticky -lm
 *
 * N is an integer, T and A are real numbers (0.5, not 1/2).
 *
 * trajectory.dat has one line per jump, about T N^2 (1 + N^(a-1)) lines, and
 * at most MAX_STEPS. If the walk reaches MAX_STEPS jumps before the final time
 * the program says at which time the file ends and returns 1.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifndef MAX_STEPS
#define MAX_STEPS 10000000
#endif

/* Parameters */
#ifndef N
#define N 500           /* The walk lives on the N+1 states 0, 1, ..., N */
#endif
#ifndef T
#define T 10            /* Final macroscopic time */
#endif
#ifndef A
#define A -1.0          /* Exponent a */
#endif
#ifndef SEED
#define SEED 42         /* Random seed */
#endif

#if N < 2
#error "N must be at least 2: the states 1 and N have different rates"
#endif

/*
 * Random numbers. The generator is splitmix64 and not rand(), which was used
 * before. On some systems rand() has only 15 bits, which cannot resolve the
 * probability N^a/(1+N^a) of entering the origin. And rand()/RAND_MAX can be
 * exactly 0 or 1: the waiting time is then zero or infinite and, at the state
 * N, the walk jumps to N+1.
 */
static uint64_t rng_state = (uint64_t)(SEED);

static uint64_t rng_next(void) {
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* A uniform number in (0,1): the endpoints are never returned. The 52 highest
   bits are used, and not 53, because k + 0.5 is exact in double only for
   k < 2^52: with 53 bits the largest value would be rounded to 1. */
static double rand_uniform(void) {
    return ((double)(rng_next() >> 12) + 0.5) / 4503599627370496.0;   /* 2^52 */
}

/* An exponential time of parameter lambda. */
static double rand_exp(double lambda) {
    return -log(rand_uniform()) / lambda;
}

int main(void) {
    const int n = (N);

    /* Time is microscopic in the loop and is divided by N^2 in the output.
       The products are done in double: N * N * T overflows an int for large N. */
    const double n2 = (double)n * (double)n;
    const double tmax = n2 * (double)(T);
    if (!(tmax > 0.0)) {
        fprintf(stderr, "T must be positive\n");
        return 1;
    }

    /* The two rates at the origin. */
    const double rate_1_to_0 = pow((double)n, (double)(A));        /* N^a     */
    const double rate_0_to_1 = pow((double)n, (double)(A) - 1.0);  /* N^(a-1) */

    double time = 0.0;
    double time_at_origin = 0.0;
    long long jumps = 0;
    int stopped = 0;        /* 1 if MAX_STEPS is reached before the final time */
    int state = 1; /* Initial state */

    FILE* out = fopen("trajectory.dat", "w");
    if (!out) {
        perror("fopen");
        return 1;
    }

    /* Each line is: rescaled time, rescaled position. One line per jump. */
    fprintf(out, "# N=%d T=%.15g a=%.15g seed=%llu\n",
            n, (double)(T), (double)(A), (unsigned long long)(SEED));
    fprintf(out, "0.0 %.10f\n", (double)state / n);

    for (;;) {
        /* The rates out of the current state: to the left and to the right. */
        double left, right;
        if (state == 0) {
            left = 0.0;          right = rate_0_to_1;   /* 0 -> 1 */
        } else if (state == 1) {
            left = rate_1_to_0;  right = 1.0;           /* 1 -> 0, 1 -> 2 */
        } else if (state == n) {
            left = 1.0;          right = 0.0;           /* N -> N-1 */
        } else {
            left = 1.0;          right = 1.0;           /* x -> x-1, x -> x+1 */
        }
        const double total_rate = left + right;

        /* The time of the next jump. If it falls after the final time the walk
           is still in the current state at the final time, and we stop. */
        double dt = rand_exp(total_rate);
        if (time + dt >= tmax) {
            dt = tmax - time;
            if (state == 0) time_at_origin += dt;
            break;
        }
        if (jumps >= (long long)(MAX_STEPS)) {
            stopped = 1;
            break;
        }
        if (state == 0) time_at_origin += dt;
        time += dt;

        /* Pick transition: to the left with probability left / total_rate. */
        if (rand_uniform() * total_rate < left) {
            state -= 1;
        } else {
            state += 1;
        }
        jumps += 1;

        fprintf(out, "%.10f %.10f\n", time / n2, (double)state / n);
    }

    /* The last line is the state at the final time. */
    if (!stopped)
        fprintf(out, "%.10f %.10f\n", (double)(T), (double)state / n);

    /* A write that failed, for instance on a full disk, must not pass unnoticed. */
    int failed = ferror(out);
    if (fclose(out) != 0) failed = 1;
    if (failed) {
        fprintf(stderr, "trajectory.dat could not be written completely\n");
        return 1;
    }

    if (stopped) {
        fprintf(stderr, "MAX_STEPS = %lld jumps reached at the time %.10f, before the final time %.15g:\n"
                        "trajectory.dat ends there. To go on, compile with a larger limit, "
                        "for instance -DMAX_STEPS=%lld\n",
                (long long)(MAX_STEPS), time / n2, (double)(T), 10 * (long long)(MAX_STEPS));
        return 1;
    }

    printf("Simulation completed. Output written to trajectory.dat\n");
    printf("N = %d, a = %.15g, T = %.15g: %lld jumps, fraction of the time at the origin %.4f\n",
           n, (double)(A), (double)(T), jumps, time_at_origin / tmax);
    return 0;
}
