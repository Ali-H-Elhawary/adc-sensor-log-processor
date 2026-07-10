#include <stdio.h>
#include <stdlib.h>
#include "adc.h"
#include "io.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <adc_sensor_log.bin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_filename = argv[1];

    FILE *fp = fopen(input_filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open '%s'.\n", input_filename);
        return EXIT_FAILURE;
    }

    RawHeader header;
    if (io_read_header(fp, &header) != 0) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    ADCSample *samples = malloc((size_t)header.record_count * sizeof(ADCSample));
    if (samples == NULL) {
        fprintf(stderr, "Error: malloc failed for %u samples.\n", header.record_count);
        fclose(fp);
        return EXIT_FAILURE;
    }

    size_t records_read = io_read_records(fp, header.record_count, samples);
    fclose(fp);

    if (records_read == 0) {
        fprintf(stderr, "Error: no records could be read from the file.\n");
        free(samples);
        return EXIT_FAILURE;
    }

    adc_convert_voltages(samples, records_read);

    unsigned int channel_count = header.channel_count;

    ChannelStats *stats  = malloc(channel_count * sizeof(ChannelStats));
    FaultCounts  *faults = malloc(channel_count * sizeof(FaultCounts));
    if (stats == NULL || faults == NULL) {
        fprintf(stderr, "Error: malloc failed while allocating per-channel results.\n");
        free(samples);
        free(stats);
        free(faults);
        return EXIT_FAILURE;
    }

    for (unsigned int ch = 0; ch < channel_count; ch++) {
        adc_compute_channel_stats(samples, records_read, ch, &stats[ch]);
        adc_detect_faults(samples, records_read, ch, &faults[ch]);
    }

    IntegrityReport integrity;
    adc_check_integrity(samples, records_read, &integrity);

    if (io_write_results("results.txt", &header, stats, faults, channel_count, &integrity) != 0) {
        free(samples);
        free(stats);
        free(faults);
        return EXIT_FAILURE;
    }

    printf("Processed %zu records across %u channels.\n", records_read, channel_count);
    printf("Results written to results.txt\n");

    free(samples);
    free(stats);
    free(faults);

    return EXIT_SUCCESS;
}