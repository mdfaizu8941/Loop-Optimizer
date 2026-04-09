#include <stdio.h>

int main() {
    int n = 5000;
    int a = 5, b = 10, c = 15;
    int arr[5000];

    int x, y, z, val, t;    
    // Worst Loop 1: Repeated invariant + useless ops
    for (int i = 0; i < n; i++) {
        x = a * b;          // invariant
        y = a * b;          // duplicate invariant
        z = i * 0;          // useless
        arr[i] = x + y + i;
    }

    // Worst Loop 2: Multiple redundant loops (can be fused)
    for (int i = 0; i < n; i++) {
        a += i;
    }

    for (int i = 0; i < n; i++) {
        b += i;
    }

    for (int i = 0; i < n; i++) {
        c += i;
    }

    // Worst Loop 3: Constant computation inside loop
    for (int i = 0; i < n; i++) {
        val = 10 * 20;   // constant
        arr[i] += val;
    }

    // Worst Loop 4: No strength reduction
    for (int i = 0; i < n; i++) {
        arr[i] += i * 2;     // should become i<<1
    }

    // Worst Loop 5: Deep nested + redundant work
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < 200; j++) {
            t = a * b;        // invariant
            arr[i] += t + j;
        }
    }

    // Worst Loop 6: Dead code loop
    int temp = 0;
    for (int i = 0; i < 10000; i++) {
        temp += i * 3;   // never used
    }

    // Worst Loop 7: Recomputing same expression
    for (int i = 0; i < n; i++) {
        arr[i] += (a + b) * (a + b);  // invariant repeated
    }

    printf("%d\n", arr[0]);
    return 0;
}

