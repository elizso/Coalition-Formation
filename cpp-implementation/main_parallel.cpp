#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <set>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <sys/resource.h>
#include <iomanip>
#include "integer_partition.h"
#include "rewards.h"

auto reward = cournot_linear;
double worstVal = -1.0, bestVal = 1e6;
int min_parallel = 600, pthread = 30;

using namespace std;
using vi = vector<int>;
using mid = unordered_map<int,double>;
using mii = unordered_map<int,int>;

struct VectorHash {
    std::size_t operator()(const vi& v) const {
        std::size_t seed = 0;
        for (int x : v) { seed ^= std::hash<int>()(x) + 0x9e3779b9 + (seed << 6) + (seed >> 2); }
        return seed;
    }
};

using vmap = unordered_map<vi, mid, VectorHash>;

/* vecRemove
    assume: v is sorted in descending order
    return: a new vector obtained from v by removing the first occurence of q */
vi vecRemove(const vi v, int q){
    vi res = v;
    auto it = find(res.begin(), res.end(), q);
    if (it != res.end()) { res.erase(it); }
    return res;
}

/* vecAdd
    assume: v is sorted in descending order
    return: a new vector obtained from v by adding one occurrence of q;
            the resulting vector is sorted in descending order */
vi vecAdd(const vi v, int q){
    vi res = v;
    auto it = lower_bound(res.begin(), res.end(), q, greater<int>());
    res.insert(it, q);
    return res;
}

/* vecBreak
    assume: v is sorted in descending order
    return: a new vector obtained from v by removing the first occurrence of i, 
            then adding one occurrence of j and one occurrence of (i-j); 
            the resulting vector is sorted in descending order */
vi vecBreak(const vi v, int i, int j){
    return vecAdd(vecAdd(vecRemove(v,i),j),i-j);
}

/* initVec
    return: a vector with (n-k+1) as the first integer, followed by (k-1) 1's */
vi initVec(int n, int k){
    vi v(k,1);
    v[0] = n-k+1;
    return v;
}

set<vi> nextVecs(vmap* prev_passive, vi v){
    set<int> unique_q(v.begin(), v.end());
    set<vi> answer;
    for (int q : unique_q){
        double activeValue = worstVal;
        int bestMove;
        for (int r=1; r<q; r++){
            if ((*prev_passive)[vecBreak(v,q,r)][r] > activeValue){
                activeValue = (*prev_passive)[vecBreak(v,q,r)][r];
                bestMove = r;
            }
        }
        if (activeValue > reward(v,q)){ answer.insert(vecBreak(v,q,bestMove)); }
    }
    return answer;
}

void worker(vi vstart, vi vend, vmap* prev_passive, vmap& local_passive){
    vi v = vstart;
    do{
        // compute best breaking move (if exists) for each q in v
        set<int> unique_q(v.begin(), v.end());
        bool stable = true;
        mii bestMove;
        for (int q : unique_q){
            double activeValue = worstVal;
            for (int r=1; r<q; r++){
                if ((*prev_passive)[vecBreak(v,q,r)][r] > activeValue){
                    activeValue = (*prev_passive)[vecBreak(v,q,r)][r];
                    bestMove[q] = r;
                }
            }
            if (activeValue <= reward(v,q)){ bestMove[q] = 0; } else { stable = false; }
        }

        if (stable) {     // if dpcurr_stable[v] is empty, then v itself is a stable CS, and passive values equal to default values at v
            for (int q : unique_q){ local_passive[v][q] = reward(v,q); }
        }
        else{             // otherwise, passive values are the minimum passive value among all immediate children
            for (int q : unique_q){
                local_passive[v][q] = bestVal;
                vi vPi = vecRemove(v,q);
                set<int> unique_qq(vPi.begin(), vPi.end());
                for (int qq : unique_qq){
                    if (bestMove[qq] > 0){ local_passive[v][q] = min(local_passive[v][q], (*prev_passive)[vecBreak(v,qq,bestMove[qq])][q]); }
                }
                if (bestMove[q] > 0){
                    vi vPiPi = vecBreak(v,q,bestMove[q]);
                    local_passive[v][q] = min(local_passive[v][q], (*prev_passive)[vPiPi][bestMove[q]]);
                    local_passive[v][q] = min(local_passive[v][q], (*prev_passive)[vPiPi][q-bestMove[q]]);
                }
            }
        }

        if (v == vend){ break; }
    } while (nextIP(v));
}

int main(int argc, char* argv[]){
    int n = stoi(argv[1]), max_k = stoi(argv[2]);

    std::ofstream log_file("results_and_logs/log.txt", std::ios_base::app);
    if (!log_file) { cerr << "Failed to open log file." << endl; return 1; }

    std::ofstream result_file("results_and_logs/result.txt", std::ios_base::app);
    if (!result_file) { cerr << "Failed to open result file." << endl; return 1; }

    log_file << "n=" << n << endl;
    auto start = chrono::high_resolution_clock::now();

    vmap dp_passive;
    vi allOne(n,1);
    dp_passive[allOne][1] = reward(allOne,1);

    for (int k=n-1; k>=1; k--){
        vi v = initVec(n,k);

        vi vdup = v;
        int count = 0;
        do{ count++; } while (nextIP(vdup));

        if (count >= min_parallel){
            int batchSize = (count + pthread - 1) / pthread;

            vector<thread> threads;
            vector<vmap> local_passive(pthread);
            vi vdup = v, vstart, vend;
            int c = 0, t = 0;
            do{
                c++;
                if (c % batchSize == 1){ vstart = vdup; }
                if (c % batchSize == 0 || c == count){
                    vend = vdup;
                    threads.emplace_back(worker,vstart,vend,&dp_passive,ref(local_passive[t]));
                    t++;
                }
            } while (nextIP(vdup));

            for (auto& th : threads) { th.join(); }
            
            for (int i = 0; i < pthread; ++i) {
                for (const auto& pair : local_passive[i]) { dp_passive[pair.first] = pair.second; }
                local_passive[i].clear();
            }
        }
        else{
            worker(v,{-1},&dp_passive,ref(dp_passive));
        }
        
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        auto now = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed_so_far = now - start;
        log_file << "k=" << k << ", count=" << count << ", memory usage: " << fixed << setprecision(3)
                         << usage.ru_maxrss / 1024.0 / 1024.0 << " GB, elapsed: " << elapsed_so_far.count() << " seconds." << endl;

        if (k+1 > max_k){
            vi vv = initVec(n,k+1);
            do{ dp_passive.erase(vv); } while (nextIP(vv));
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    log_file << "Elapsed time: " << elapsed.count() << " seconds." << endl;

    result_file << "Stable descendant(s) of (" << n << ") is/are:" << endl;
    set<vi> queue_curr, queue_next;
    queue_curr.insert({n});
    while (!queue_curr.empty()) {
        if ((*queue_curr.begin()).size() > max_k) {
            log_file << "Error: Partition size exceeds max_k (" << max_k << ")." << endl;
            break;
        }
        for (vi v : queue_curr){
            set<vi> next = nextVecs(&dp_passive, v);
            queue_next.insert(next.begin(), next.end());
            if (next.empty()){
                for (int q : v){ result_file << q << " "; }
                result_file << endl;
            }
        }
        queue_curr.clear();
        queue_curr = queue_next;
        queue_next.clear();
    }

    log_file << "===================================================" << endl;
    log_file.close();
    result_file.close();
}