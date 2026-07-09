#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdio.h>
#include "adc.h"

#define ADC_MAGIC_NUMBER 0xADC1BEEFu

/* ---- On-disk layout ------------------------------------------------------
 * These structs mirror the binary file format EXACTLY, byte for byte, as
 * specified in the coursework brief (sections 3.1 and 3.2).
 *
 * WHY #pragma pack(push, 1):
 * The compiler is normally free to insert padding bytes between struct
 * members so that each member starts at an address matching its own
 * alignment requirement (e.g. a uint32_t often wants to start on a 4-byte
 * boundary). That is great for runtime speed, but it means sizeof(struct)
 * in memory will not necessarily match the byte layout used in a file that
 * was written by some other tool with a fixed, documented layout.
 *
 * Concretely: without packing, RawRecord below would be padded out to 20
 * bytes on most platforms (a gap after channel_id to align raw_value, and
 * a gap after status_flags to align sequence_number), but the file format
 * defines the record as exactly 16 bytes. Reading with fread() straight
 * into an unpacked struct would silently shift every field after the first
 * padding gap, corrupting every value without any indication of an error.
 *
 * #pragma pack(push, 1) tells the compiler "do not insert any padding" so
 * sizeof(RawRecord) == 16 and sizeof(RawHeader) == 24, matching the file
 * exactly. This is a compiler extension (supported by GCC/Clang/MSVC) but
 * is the standard, expected way to solve this problem in C.
 * -------------------------------------------------------------------- */
#pragma pack(push, 1)

typedef struct {
    uint32_t magic;            /* must equal ADC_MAGIC_NUMBER */
    uint16_t version;          /* expected 1 */
    uint16_t channel_count;    /* expected 4 */
    uint32_t record_count;     /* number of RawRecord entries following */
    uint32_t sample_rate_hz;   /* expected 1000 */
    uint8_t  reserved[8];      /* unused, read and discard */
} RawHeader;

typedef struct {
    float    timestamp;
    uint8_t  channel_id;
    uint16_t raw_value;
    int16_t  temperature;
    uint8_t  status_flags;
    uint32_t sequence_number;
    uint8_t  reserved[2];      /* unused, read and discard */
} RawRecord;

#pragma pack(pop)

/* Read and validate the 24-byte file header. Returns 0 on success, or a
 * negative error code on failure (bad magic, short read, etc). Prints a
 * descriptive error message to stderr before returning on failure. */
int io_read_header(FILE *fp, RawHeader *header_out);

/* Read `record_count` fixed-size records from `fp` (which must be
 * positioned immediately after the header) into the pre-allocated
 * `samples` array, converting each RawRecord into an ADCSample as it goes.
 * The `voltage` field of each ADCSample is left at 0.0 here; conversion to
 * voltage is a separate analysis step (adc_convert_voltages).
 * Returns the number of records successfully read. */
size_t io_read_records(FILE *fp, uint32_t record_count, ADCSample *samples);

/* Write the full results.txt report: header summary, per-channel stats,
 * fault counts, and the integrity report. Returns 0 on success, non-zero
 * if the file could not be opened for writing. */
int io_write_results(const char *filename,
                      const RawHeader *header,
                      const ChannelStats *stats,
                      const FaultCounts *faults,
                      unsigned int channel_count,
                      const IntegrityReport *integrity);

#endif /* IO_H */
