#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stddef.h>

#define ADC_VREF                    3.3
#define ADC_MAX_RAW                 4095.0

#define OVERVOLTAGE_THRESHOLD       3.3
#define UNDERVOLTAGE_THRESHOLD      0.3

#define STATUS_FLAG_SENSOR_FAULT    0x01u
#define STATUS_FLAG_OUT_OF_RANGE    0x02u

typedef struct {
    float       timestamp;
    uint8_t     channel_id;
    uint16_t    raw_value;
    double      voltage;
    int16_t     temperature;
    uint8_t     status_flags;
    uint32_t    sequence_number;
}ADCSample;

typedef struct {
    unsigned int    channel_id;
    unsigned long   sample_count;
    double          mean_voltage;
    double          min_voltage;
    double          max_voltage;
    double          stddev_voltage;
} ChannelStats;

typedef struct {
    unsigned int    channel_id;
    unsigned long   overvoltage_count;
    unsigned long   undervoltage_count;
    unsigned long   sensor_fault_count;
    unsigned long   total_fault_count;
} FaultCounts;

typedef struct {
    unsigned long   missing_count;
    unsigned long   out_of_order_count;
    unsigned long   first_gap_before_seq;
} IntegrityReport;

void adc_convert_voltages(ADCSample *samples, size_t count);

void adc_compute_channel_stats(const ADCSample *samples, size_t count,
                                unsigned int channel_id, ChannelStats *out);

void adc_detect_faults(const ADCSample *samples, size_t count,
                        unsigned int channel_id, FaultCounts *out);

void adc_check_integrity(const ADCSample *samples, size_t count, IntegrityReport *out);

#endif