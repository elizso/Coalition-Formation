#include <vector>
using vi = std::vector<int>;

double cournot_linear(vi v, int q){ return 1.0 / (v.size() + 1) / (v.size() + 1) / q; }