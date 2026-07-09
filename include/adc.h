#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stddef.h>

/* ---- Engineering constants ---------------------------------------------
 * VREF and ADC_MAX_RAW come directly from the coursework brief:
 *   voltage = (raw_value / 4095.0) * Vref,   Vref = 3.3 V
 * 4095 = 2^12 - 1, i.e. the largest value a 12-bit ADC can produce.
 * -------------------------------------------------------------------- */
#define ADC_VREF            3.3
#define ADC_MAX_RAW         4095.0

#define OVERVOLTAGE_THRESHOLD   3.0
#define UNDERVOLTAGE_THRESHOLD  0.3

/* Bit 0 of status_flags = sensor fault (see brief, section 3.2) */
#define STATUS_FLAG_SENSOR_FAULT   0x01u
/* Bit 1 of status_flags = out-of-range flag reported by the ADC itself */
#define STATUS_FLAG_OUT_OF_RANGE   0x02u

/* ---- In-memory sample representation ------------------------------------
 * This is the struct requirement (#1). Note this is DELIBERATELY different
 * from the on-disk record layout in io.h: this struct also carries the
 * derived `voltage` field, and we do not care about padding here because
 * we never fread() directly into this struct - only the raw on-disk struct
 * (RawRecord, in io.h) is read with fread(), and its fields are copied
 * across into this one field-by-field. That copy step is what protects us
 * from struct-padding bugs.
 * -------------------------------------------------------------------- */
typedef struct {
    float    timestamp;        /* seconds since start of recording */
    uint8_t  channel_id;       /* 0..3 */
    uint16_t raw_value;        /* raw 12-bit ADC code, 0..4095 */
    double   voltage;          /* derived: raw_value converted to volts */
    int16_t  temperature;      /* tenths of a degree C, e.g. 245 = 24.5C */
    uint8_t  status_flags;     /* bit0 = sensor fault, bit1 = out-of-range */
    uint32_t sequence_number;  /* monotonically increasing record counter */
} ADCSample;

/* Per-channel statistics (requirement #6) */
typedef struct {
    unsigned int  channel_id;
    unsigned long sample_count;
    double        mean_voltage;
    double        min_voltage;
    double        max_voltage;
    double        stddev_voltage;
} ChannelStats;

/* Per-channel fault counts (requirement #7) */
typedef struct {
    unsigned int  channel_id;
    unsigned long overvoltage_count;
    unsigned long undervoltage_count;
    unsigned long sensor_fault_count;
    unsigned long total_fault_count; /* union of the three above, dedup'd per sample */
} FaultCounts;

/* Sampling integrity report (requirement #8) */
typedef struct {
    unsigned long missing_count;      /* number of gaps detected in sequence_number */
    unsigned long out_of_order_count; /* number of records whose sequence went backwards */
    unsigned long first_gap_before_seq; /* sequence_number at which the first gap was found (0 if none) */
} IntegrityReport;

/* ---- Analysis functions --------------------------------------------------
 * All of these take pointers and use pointer arithmetic internally
 * (requirement #4) rather than relying solely on array indexing.
 * -------------------------------------------------------------------- */

/* Requirement #5: convert every sample's raw_value into a voltage,
 * writing the result into sample->voltage in place. */
void adc_convert_voltages(ADCSample *samples, size_t count);

/* Requirement #6: compute mean/min/max/stddev of voltage for a single
 * channel. Scans the full `samples` array but only considers samples
 * whose channel_id matches `channel_id`. Returns the populated stats
 * struct via the out pointer. */
void adc_compute_channel_stats(const ADCSample *samples, size_t count,
                                unsigned int channel_id, ChannelStats *out);

/* Requirement #7: scan a single channel's samples and count fault
 * conditions (overvoltage, undervoltage, sensor-fault bit set). */
void adc_detect_faults(const ADCSample *samples, size_t count,
                        unsigned int channel_id, FaultCounts *out);

/* Requirement #8: walk the full sample array in file order and check
 * that sequence_number increases by exactly 1 each record. Reports the
 * first gap found (if any) and totals. */
void adc_check_integrity(const ADCSample *samples, size_t count,
                          IntegrityReport *out);

#endif /* ADC_H */
