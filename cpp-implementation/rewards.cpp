#include <vector>
using namespace std;
using vi = vector<int>;

double cournot_linear(vi v, int q){ return 1.0 / (v.size() + 1) / (v.size() + 1) / q; }

/*
double shapley(vi v, int q){
    int n = 0;
    for (int x : v) { n += x; }
    
    vector<vi> dp(v.size()+1, vi(n + 1, 0));

    dp[0][0] = 1;
    for (int i=1; i<=v.size()+1; i++) {
        for (int j=v[i+1]; j<=n; j++) {
            dp[i][j] = 
        }
    }
}
*/