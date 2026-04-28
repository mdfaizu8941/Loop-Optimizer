#include <stdio.h>

int main() {
    int n = 5000;
    int a = 5, b = 10, c = 15;
    int arr[5000];

    int x, y, z, val, t;    
    // Worst Loop 1: Repeated invariant + useless ops
    x = a * b;
z = 0;
{
    int i = 0;
for (; i + 1 < n; i += 2) {
        y = x;
            arr[i] = x + y + i;
        i++;
        y = x;
            arr[i] = x + y + i;
}
for (; i < n; i++) {
    y = x;
    arr[i] = x + y + i;
}
}

    // Worst Loop 2: Multiple redundant loops (can be fused)
    {
    int i = 0;
for (; i + 1 < n; i += 2) {
        a += i;
        i++;
        a += i;
}
for (; i < n; i++) {
    a += i;
}
}

    {
    int i = 0;
for (; i + 1 < n; i += 2) {
        b += i;
        i++;
        b += i;
}
for (; i < n; i++) {
    b += i;
}
}

    {
    int i = 0;
for (; i + 1 < n; i += 2) {
        c += i;
        i++;
        c += i;
}
for (; i < n; i++) {
    c += i;
}
}

    // Worst Loop 3: Constant computation inside loop
    val = 10 * 20;
{
    int i = 0;
for (; i + 1 < n; i += 2) {
        arr[i] += val;
        i++;
        arr[i] += val;
}
for (; i < n; i++) {
    arr[i] += val;
}
}

    // Worst Loop 4: No strength reduction
    {
    int i = 0;
for (; i + 1 < n; i += 2) {
        arr[i] += i << 1;
        i++;
        arr[i] += i << 1;
}
for (; i < n; i++) {
    arr[i] += i << 1;
}
}

    // Worst Loop 5: Deep nested + redundant work
    t = a * b;
{
    int i = 0;
for (; i + 1 < 200; i += 2) {
        {
            int j = 0;
        for (; j + 1 < 200; j += 2) {
                arr[i] += t + j;
                j++;
                arr[i] += t + j;
        }
        for (; j < 200; j++) {
            arr[i] += t + j;
        }
        }
        i++;
        {
            int j = 0;
        for (; j + 1 < 200; j += 2) {
                arr[i] += t + j;
                j++;
                arr[i] += t + j;
        }
        for (; j < 200; j++) {
            arr[i] += t + j;
        }
        }
}
for (; i < 200; i++) {
    {
    int j = 0;
for (; j + 1 < 200; j += 2) {
        arr[i] += t + j;
        j++;
        arr[i] += t + j;
}
for (; j < 200; j++) {
    arr[i] += t + j;
}
}
}
}

    // Worst Loop 6: Dead code loop
    int temp = 0;
    {
    int i = 0;
for (; i + 1 < 10000; i += 2) {
        temp += i * 3;
        i++;
        temp += i * 3;
}
for (; i < 10000; i++) {
    temp += i * 3;
}
}

    // Worst Loop 7: Recomputing same expression
    {
    int i = 0;
for (; i + 1 < n; i += 2) {
        arr[i] += (a + b) * (a + b);
        i++;
        arr[i] += (a + b) * (a + b);
}
for (; i < n; i++) {
    arr[i] += (a + b) * (a + b);
}
}

    printf("%d\n", arr[0]);
    return 0;
}

