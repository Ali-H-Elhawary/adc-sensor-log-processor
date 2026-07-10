#ifndef STATS_H
#define STATS_H

#include <stddef.h>

double stats_mean(const double *values, size_t count);

double stats_min(const double *values, size_t count);

double stats_max(const double *values, size_t count);

double stats_stddev(const double *values, size_t count, double mean);

#endif