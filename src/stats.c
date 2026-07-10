#include "stats.h"
#include <math.h>

double stats_mean(const double *values, size_t count) {
    if (count == 0) {
        return 0.0;
    }

    double sum = 0.0;
    const double *p     = values;
    const double *end   = values + count;

    while (p < end) {
        sum += *p;
        p++;
    }

    return sum / (double)count;
}

double stats_min(const double *values, size_t count) {
    const double *p     = values;
    const double *end   = values + count;
    double min_val = *p;
    p++;

    while (p < end) {
        if (*p < min_val) {
            min_val = *p;
        }
        p++;
    }
    return min_val;
}

double stats_max(const double *values, size_t count) {
    const double *p     = values;
    const double *end   = values + count;
    double max_val = *p;
    p++;

    while (p < end) {
        if (*p > max_val) {
            max_val = *p;
        }
        p++;
    }
    return max_val;
}

double stats_stddev(const double *values, size_t count, double mean) {
    if (count == 0) {
        return 0.0;
    }
    double sum_sq_diff  = 0.0;
    const double *p     = values;
    const double *end   = values + count;

    while (p < end) {
        double diff = *p - mean;
        sum_sq_diff += diff * diff;
        p++;
    }
    return sqrt(sum_sq_diff / (double)count);
}

