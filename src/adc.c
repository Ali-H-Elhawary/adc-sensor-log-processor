#include "adc.h"
#include "stats.h"
#include <stdlib.h>
#include <stdio.h>

void adc_convert_voltages(ADCSample *samples, size_t count) {
    ADCSample *p   = samples;
    ADCSample *end = samples + count;

    while (p < end) {
        /* 12-bit ADC: raw_value ranges 0..4095. Dividing by 4095.0 gives a
         * fraction of full-scale, multiplying by Vref gives volts. This
         * must be done in floating point: raw_value/4095 done as integer
         * division would truncate to 0 for almost every reading. */
        p->voltage = ((double)p->raw_value / ADC_MAX_RAW) * ADC_VREF;
        p++;
    }
}

void adc_compute_channel_stats(const ADCSample *samples, size_t count,
                                unsigned int channel_id, ChannelStats *out) {
    out->channel_id   = channel_id;
    out->sample_count = 0;
    out->mean_voltage = 0.0;
    out->min_voltage  = 0.0;
    out->max_voltage  = 0.0;
    out->stddev_voltage = 0.0;

    /* First pass: count how many samples belong to this channel so we can
     * allocate exactly the right size buffer of voltages to hand to the
     * stats_* helpers (which expect a plain array of doubles). */
    size_t channel_sample_count = 0;
    const ADCSample *p   = samples;
    const ADCSample *end = samples + count;
    while (p < end) {
        if (p->channel_id == channel_id) {
            channel_sample_count++;
        }
        p++;
    }

    if (channel_sample_count == 0) {
        return; /* no samples for this channel; leave stats at zero */
    }

    double *voltages = malloc(channel_sample_count * sizeof(double));
    if (voltages == NULL) {
        fprintf(stderr, "Error: malloc failed while computing channel %u stats.\n",
                channel_id);
        return;
    }

    double *dest = voltages;
    p = samples;
    while (p < end) {
        if (p->channel_id == channel_id) {
            *dest = p->voltage;
            dest++;
        }
        p++;
    }

    out->sample_count   = channel_sample_count;
    out->mean_voltage   = stats_mean(voltages, channel_sample_count);
    out->min_voltage    = stats_min(voltages, channel_sample_count);
    out->max_voltage    = stats_max(voltages, channel_sample_count);
    out->stddev_voltage = stats_stddev(voltages, channel_sample_count, out->mean_voltage);

    free(voltages);
}

void adc_detect_faults(const ADCSample *samples, size_t count,
                        unsigned int channel_id, FaultCounts *out) {
    out->channel_id         = channel_id;
    out->overvoltage_count  = 0;
    out->undervoltage_count = 0;
    out->sensor_fault_count = 0;
    out->total_fault_count  = 0;

    const ADCSample *p   = samples;
    const ADCSample *end = samples + count;

    while (p < end) {
        if (p->channel_id == channel_id) {
            int is_overvoltage  = (p->voltage > OVERVOLTAGE_THRESHOLD);
            int is_undervoltage = (p->voltage < UNDERVOLTAGE_THRESHOLD);
            /* Bitwise AND with the bit-0 mask isolates just that bit,
             * regardless of what the other 7 bits contain. This is the
             * standard way to read a single flag out of a bitmask register:
             * (status_flags & MASK) is nonzero iff that bit is set. */
            int is_sensor_fault = (p->status_flags & STATUS_FLAG_SENSOR_FAULT) != 0;

            if (is_overvoltage)  out->overvoltage_count++;
            if (is_undervoltage) out->undervoltage_count++;
            if (is_sensor_fault) out->sensor_fault_count++;

            if (is_overvoltage || is_undervoltage || is_sensor_fault) {
                out->total_fault_count++;
            }
        }
        p++;
    }
}

void adc_check_integrity(const ADCSample *samples, size_t count, IntegrityReport *out) {
    out->missing_count         = 0;
    out->out_of_order_count    = 0;
    out->first_gap_before_seq  = 0;

    if (count < 2) {
        return;
    }

    const ADCSample *prev = samples;
    const ADCSample *p    = samples + 1;
    const ADCSample *end  = samples + count;

    while (p < end) {
        uint32_t expected = prev->sequence_number + 1;

        if (p->sequence_number < prev->sequence_number) {
            out->out_of_order_count++;
        } else if (p->sequence_number > expected) {
            if (out->missing_count == 0) {
                out->first_gap_before_seq = prev->sequence_number;
            }
            /* Number of missing records is the size of the gap. */
            out->missing_count += (p->sequence_number - expected);
        }

        prev = p;
        p++;
    }
}
