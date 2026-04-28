#include <stdio.h>

int main() {
    int n = 100;  // increase this to make it even worse
    long long count = 0;

    for (int a = 0; a < n; a++) {
        for (int b = 0; b < n; b++) {
            for (int c = 0; c < n; c++) {
                for (int d = 0; d < n; d++) {
                    for (int e = 0; e < n; e++) {
                        for (int f = 0; f < n; f++) {
                            // Worst-case dummy operation
                            count++;
                        }
                    }
                }
            }
        }
    }

    printf("Total iterations: %lld\n", count);
    return 0;
}