#include <bits/stdc++.h>
using namespace std;

const int MAXA = 100005;

vector<int> getDivisors(int x) {
    vector<int> divisors;
    for (int d = 1; d * d <= x; ++d) {
        if (x % d == 0) {
            divisors.push_back(d);
            if (x / d != d) divisors.push_back(x / d);
        }
    }
    return divisors;
}

struct SegmentTree {
    int N;
    vector<int> tree;

    SegmentTree(const vector<int>& bonus) {
        N = bonus.size();
        tree.resize(4 * N);
        build(1, 0, N - 1, bonus);
    }

    void build(int node, int l, int r, const vector<int>& bonus) {
        if (l == r) {
            tree[node] = bonus[l];
            return;
        }
        int mid = (l + r) / 2;
        build(2 * node, l, mid, bonus);
        build(2 * node + 1, mid + 1, r, bonus);
        tree[node] = max(tree[2 * node], tree[2 * node + 1]);
    }

    int query(int node, int l, int r, int ql, int qr) {
        if (qr < l || ql > r) return 0;
        if (ql <= l && r <= qr) return tree[node];
        int mid = (l + r) / 2;
        return max(query(2 * node, l, mid, ql, qr),
                   query(2 * node + 1, mid + 1, r, ql, qr));
    }

    int getMax(int l, int r) {
        return query(1, 0, N - 1, l, r);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<int> A(N), Bonus(N);
    for (int i = 0; i < N; ++i) cin >> A[i];
    for (int i = 0; i < N; ++i) cin >> Bonus[i];

    SegmentTree seg(Bonus);
    vector<int> last_seen(MAXA, -1);

    long long totalXP = 0;

    for (int i = N - 1; i >= 0; --i) {
        int val = A[i];
        int bestR = N;

        vector<int> divisors = getDivisors(val);
        int tempR = last_seen[A[i]];
        
        if (tempR != -1 && A[tempR] % A[i] == 0)
            bestR = tempR;
        
        for (int d : divisors) {
            last_seen[d] = i;
        }

        if (bestR < N) {
            totalXP += seg.getMax(i, bestR);
        }
    }

    cout << totalXP << "\n";
    return 0;
}
