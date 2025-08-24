#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_STEPS 10000000

// Parameters
#define N 500           // Number of states
#define T 10           // Final macroscopic time
#define A -1.0           // Exponent a
#define SEED 42         // Random seed

double rand_exp(double lambda) {
    double u = (double) rand() / RAND_MAX;
    return -log(1.0 - u) / lambda;
}

int main() {
    srand(SEED);
    double time = 0.0;
    int state = 1; // Initial state
    double tmax = N * N * T;

    FILE* out = fopen("trajectory.dat", "w");
    if (!out) {
        perror("fopen");
        return 1;
    }

    fprintf(out, "0.0 %.10f\n", (double) state / N);

    for (int step = 0; step < MAX_STEPS && time < tmax; ++step) {
        double rates[3] = {0.0}; // left, stay, right
        double total_rate = 0.0;

        if (state == 0) {
            rates[2] = pow(N, A - 1); // 0 → 1
        } else if (state == 1) {
            rates[0] = pow(N, A);     // 1 → 0
            rates[2] = 1.0;           // 1 → 2
        } else if (state == N) {
            rates[0] = 1.0;           // N → N-1
        } else {
            rates[0] = 1.0;           // x → x-1
            rates[2] = 1.0;           // x → x+1
        }

        total_rate = rates[0] + rates[2];

        if (total_rate == 0.0)
            break;

        double dt = rand_exp(total_rate);
        time += dt;

        // Pick transition
        double r = ((double) rand() / RAND_MAX) * total_rate;
        if (r < rates[0]) {
            state -= 1;
        } else {
            state += 1;
        }

        fprintf(out, "%.10f %.10f\n", time / (N * N ), (double) state / N);
    }

    fclose(out);
    printf("Simulation completed. Output written to trajectory.dat\n");
    return 0;
}

