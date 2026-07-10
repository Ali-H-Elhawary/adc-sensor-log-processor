#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdio.h>
#include "adc.h"

#define ADC_MAGIC_NUMBER 0xADC1BEEFu

//#pragma pack(push, 1)

typedef struct {
    uint32_t    magic;
    uint16_t    version;
    uint16_t    channel_count;
    uint32_t    record_count;
    uint32_t    sample_rate_hz;
    uint8_t     reserved[8];
} RawHeader;

typedef struct {
    float       timestamp;
    uint8_t     channel_id;
    uint16_t    raw_value;
    int16_t     temperature;
    uint8_t     status_flags;
    uint32_t    sequence_number;
    uint8_t     reserved[2];
} RawRecord;

//#pragma pack(pop)

int io_read_header(FILE *fp, RawHeader *header_out);

size_t io_read_records(FILE *fp, uint32_t record_count, ADCSample *samples);

int io_write_results(const char *filename,
    const RawHeader *header,
    const ChannelStats *stats,
    const FaultCounts *faults,
    unsigned int channel_count,
    const IntegrityReport *integrity);

#endif