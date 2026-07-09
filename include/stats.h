#ifndef STATS_H
#define STATS_H

#include <stddef.h>

/* Generic statistics functions over an array of doubles. These are kept
 * independent of ADCSample / channels so they are callable and testable in
 * isolation (as required by the "modular code structure" requirement) and
 * could be reused for any other numeric dataset (voltage, temperature,
 * etc).
 */

/* Arithmetic mean of values[0..count-1]. Returns 0.0 if count == 0. */
double stats_mean(const double *values, size_t count);

/* Minimum value. Caller must ensure count > 0. */
double stats_min(const double *values, size_t count);

/* Maximum value. Caller must ensure count > 0. */
double stats_max(const double *values, size_t count);

/* Population standard deviation, given a precomputed mean. This uses the
 * classic two-pass algorithm: pass 1 computes the mean (done by the
 * caller via stats_mean), pass 2 (this function) sums squared deviations
 * from that mean. A single-pass "sum and sum-of-squares" method exists but
 * is numerically unstable for real sensor data with large means and small
 * variances (catastrophic cancellation), so the two-pass method is used
 * here deliberately. Returns 0.0 if count == 0. */
double stats_stddev(const double *values, size_t count, double mean);

#endif /* STATS_H */
