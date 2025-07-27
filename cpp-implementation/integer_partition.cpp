#include "integer_partition.h"
#include <vector>

using namespace std;
using vi = vector<int>;

/* nextIP
    assume: v is an integer partition (IP) of some positive integer n, with k parts;
            v is sorted in descending order
            for example, v = {12,6,6,4,1,1,1} is an IP of 31 with 7 parts
    impact: v is updated in-place to be the next IP of n with k parts, in reverse lexicographical order
            for example, if input v = {12,6,6,4,1,1,1}, the next few updates to v by nextIP are:
            {12,6,6,3,2,1,1}, {12,6,6,2,2,2,1}, {12,6,5,5,1,1,1}, {12,6,5,4,2,1,1},
            {12,6,5,3,3,1,1}, {12,6,5,3,2,2,1}, {12,6,5,2,2,2,2}, {12,6,4,4,3,1,1}
    return: true, unless the input v is the last IP of n with k parts

    algorithm high-level description:
        Let l(v,j) denote the vector obtained from v by keeping only the last j parts. Let s(v,j) denote the sum of integers in l(v,j).
        For example, if v = {12,6,6,4,1,1,1}, then l(v,5) = {6,4,1,1,1}, s(v,5) = 13, l(v,2) = {1,1} and s(v,2) = 2.
        We say v is *j-saturated* if l(v,j) is the final IP when all IPs of s(v,j) with j parts are sorted in reverse lexicographical order.
        (When we say an IP is first/final, we always refer to reverse lexicographical order.)
        It is not hard to prove that:
            (i)  v is always 1-saturated
            (ii) for j>=2, v is j-saturated iff v is (j-1)-saturated and the first integer of l(v,j) equals to ceil(s(v,j)/j),
        
        The algorithm proceeds by:
            Step 1: find the smallest j>1 such that v is not j-saturated
            Step 2: If v is (v.size())-saturated (so no j found in Step 1), then v is final IP of n with k parts, so return false.
            Step 3: Otherwise, reduce the j-th integer of v (counted from the back) by 1 (call this new integer q),
                    and update the last (j-1) integers of v to be the first IP among all IPs of s(v,j-1)+1 with (j-1) parts and each such part at most q. */
bool nextIP(vi& v){
    if (v.empty()){ return false; }

    //Step 1 in the "algorithm high-level description" above
    vi::iterator begin = v.begin(), end = v.end(), ptr = v.end();
    int sum = 0, l = 0, r;
    do{
        ptr--;
        sum += *ptr;
        l++;
        r = (sum+l-1)/l;
    } while ((*ptr == r) && (ptr != begin));

    //Step 2 in the "algorithm high-level description" above
    if (*ptr == r){ return false; }

    //Step 3 in the "algorithm high-level description" above
    int cap = *ptr-1; // cap is same as q in the description
    while (l > 0){
        *ptr = min(sum-l+1,cap); // if the cap constraint does not exist, the largest possible value of *ptr is (sum-l+1), and the remaining (l-1) parts of v are all 1's
        cap = min(cap,*ptr);
        sum -= *ptr;
        l--;
        ptr++;
    }
    return true;
}