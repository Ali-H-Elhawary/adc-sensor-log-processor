#include "io.h"
#include <stdio.h>
#include <string.h>

int io_read_header(FILE *fp, RawHeader *header_out) {
    size_t items_read = fread(header_out, sizeof(RawHeader), 1, fp);

    if (items_read != 1) {
        fprintf(stderr,
                "Error: could not read %zu-byte file header (file too short or unreadable).\n",
                sizeof(RawHeader));
        return -1;
    }

    if (header_out->magic != ADC_MAGIC_NUMBER) {
        fprintf(stderr,
                "Error: bad magic number. Expected 0x%08X, got 0x%08X. "
                "This does not look like a valid ADC log file.\n",
                ADC_MAGIC_NUMBER, header_out->magic);
        return -2;
    }

    if (header_out->version != 1) {
        fprintf(stderr,
                "Warning: unexpected file version %u (expected 1). Continuing anyway.\n",
                header_out->version);
    }

    if (header_out->record_count == 0) {
        fprintf(stderr, "Error: header reports zero records; nothing to process.\n");
        return -3;
    }

    return 0;
}

size_t io_read_records(FILE *fp, uint32_t record_count, ADCSample *samples) {
    RawRecord raw;
    ADCSample *dest = samples; /* pointer used for iteration, not indexing */
    uint32_t records_read = 0;

    while (records_read < record_count) {
        size_t items_read = fread(&raw, sizeof(RawRecord), 1, fp);
        if (items_read != 1) {
            /* Short/truncated file: stop here and report how many we got. */
            fprintf(stderr,
                    "Warning: expected %u records but file ended after %u. "
                    "File may be truncated.\n",
                    record_count, records_read);
            break;
        }

        /* Copy field-by-field from the packed on-disk struct into the
         * in-memory ADCSample. This explicit copy is what protects us from
         * any layout mismatch: even if RawRecord and ADCSample had
         * different padding, each field is assigned individually so there
         * is no risk of misaligned bytes leaking into the wrong field. */
        dest->timestamp       = raw.timestamp;
        dest->channel_id      = raw.channel_id;
        dest->raw_value       = raw.raw_value;
        dest->voltage         = 0.0; /* computed later by adc_convert_voltages */
        dest->temperature     = raw.temperature;
        dest->status_flags    = raw.status_flags;
        dest->sequence_number = raw.sequence_number;

        dest++;
        records_read++;
    }

    return (size_t)records_read;
}

int io_write_results(const char *filename,
                      const RawHeader *header,
                      const ChannelStats *stats,
                      const FaultCounts *faults,
                      unsigned int channel_count,
                      const IntegrityReport *integrity) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open '%s' for writing.\n", filename);
        return -1;
    }

    fprintf(fp, "==========================================================\n");
    fprintf(fp, " ADC SENSOR LOG - ANALYSIS RESULTS\n");
    fprintf(fp, "==========================================================\n\n");

    fprintf(fp, "Source file summary\n");
    fprintf(fp, "----------------------------------------------------------\n");
    fprintf(fp, "  File version     : %u\n", header->version);
    fprintf(fp, "  Channel count    : %u\n", header->channel_count);
    fprintf(fp, "  Record count     : %u\n", header->record_count);
    fprintf(fp, "  Sample rate (Hz) : %u\n\n", header->sample_rate_hz);

    fprintf(fp, "Per-channel statistics (voltage, V)\n");
    fprintf(fp, "----------------------------------------------------------\n");
    fprintf(fp, "%-8s%-10s%-12s%-12s%-12s%-12s\n",
            "Chan", "Samples", "Mean", "Min", "Max", "StdDev");

    const ChannelStats *sp = stats;
    for (unsigned int i = 0; i < channel_count; i++, sp++) {
        fprintf(fp, "%-8u%-10lu%-12.4f%-12.4f%-12.4f%-12.4f\n",
                sp->channel_id, sp->sample_count,
                sp->mean_voltage, sp->min_voltage, sp->max_voltage,
                sp->stddev_voltage);
    }
    fprintf(fp, "\n");

    fprintf(fp, "Fault detection\n");
    fprintf(fp, "----------------------------------------------------------\n");
    fprintf(fp, "%-8s%-14s%-14s%-14s%-14s\n",
            "Chan", "Overvoltage", "Undervoltage", "SensorFault", "TotalFaults");

    const FaultCounts *fp_c = faults;
    for (unsigned int i = 0; i < channel_count; i++, fp_c++) {
        fprintf(fp, "%-8u%-14lu%-14lu%-14lu%-14lu\n",
                fp_c->channel_id, fp_c->overvoltage_count,
                fp_c->undervoltage_count, fp_c->sensor_fault_count,
                fp_c->total_fault_count);
    }
    fprintf(fp, "\n");

    fprintf(fp, "Sampling integrity\n");
    fprintf(fp, "----------------------------------------------------------\n");
    fprintf(fp, "  Missing records (gaps) : %lu\n", integrity->missing_count);
    fprintf(fp, "  Out-of-order records   : %lu\n", integrity->out_of_order_count);
    if (integrity->missing_count > 0) {
        fprintf(fp, "  First gap detected after sequence_number %lu\n",
                integrity->first_gap_before_seq);
    } else {
        fprintf(fp, "  No gaps detected: sequence numbers are contiguous.\n");
    }

    fclose(fp);
    return 0;
}
