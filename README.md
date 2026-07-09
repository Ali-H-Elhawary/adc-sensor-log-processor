# ADC Sensor Log Processor

UFMFGT-15-1 Programming for Engineers — Resit Coursework
Embedded Sensor Data Processor

## What this program does

Reads a binary log (`adc_sensor_log.bin`) produced by a 4-channel, 12-bit
ADC sampling at 1 kHz, and:

- Validates the file header (magic number, record count)
- Parses every fixed-size record with `fread()`
- Converts each raw 12-bit ADC code into a voltage
- Computes mean / min / max / standard deviation of voltage per channel
- Detects fault conditions (overvoltage, undervoltage, sensor-fault bit)
- Checks the `sequence_number` field for gaps or out-of-order records
- Writes a structured `results.txt` report

## Project structure

```
.
├── CMakeLists.txt
├── README.md
├── include/
│   ├── adc.h      # ADCSample struct, analysis function declarations
│   ├── io.h        # on-disk RawHeader/RawRecord structs, file I/O declarations
│   └── stats.h     # generic mean/min/max/stddev helpers
└── src/
    ├── main.c      # entry point only — no analysis logic
    ├── adc.c        # voltage conversion, per-channel stats, faults, integrity
    ├── io.c         # binary parsing + results.txt writer
    └── stats.c      # mean/min/max/stddev implementations
```

## Building in CLion

1. Open this folder in CLion via **File → Open**, selecting the folder
   containing `CMakeLists.txt`.
2. CLion will detect the CMake project automatically and configure it
   (check the "CMake" tool window at the bottom for the configure log).
3. Select the `adc_processor` run configuration (top-right dropdown).
4. Build with **Build → Build Project** (or the hammer icon).
5. To run with the dataset as a command-line argument:
   **Run → Edit Configurations… → Program arguments**, enter the path to
   `adc_sensor_log.bin` (e.g. `../adc_sensor_log.bin` if the binary lives
   one level up), then **Run**.

## Building from the command line (gcc)

```bash
mkdir -p build
cd build
cmake ..
make
./adc_processor ../adc_sensor_log.bin
```

Or, without CMake at all, directly with gcc:

```bash
gcc -std=c99 -Wall -Wextra -Iinclude \
    src/main.c src/adc.c src/io.c src/stats.c \
    -o adc_processor -lm
./adc_processor adc_sensor_log.bin
```

## Output

Running the program produces `results.txt` in the working directory,
containing the file summary, per-channel voltage statistics, fault
counts, and the sampling integrity report.

## GitHub repository

<!-- Replace this with the actual URL after you create the repo -->
https://github.com/Ali-H-Elhawary/adc-sensor-log-processor

## Notes on binary parsing / struct padding

The on-disk header and record layouts (`RawHeader`, `RawRecord` in
`io.h`) are declared with `#pragma pack(push, 1)` so the compiler does not
insert padding bytes between members. Without this, natural alignment
would pad `RawRecord` out to 20 bytes instead of the file's actual 16,
and every `fread()` would silently read misaligned garbage. See the
comments in `io.h` for the full explanation.
