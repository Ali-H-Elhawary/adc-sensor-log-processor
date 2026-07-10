cmake_minimum_required(VERSION 3.10)
project(adc_processor C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

include_directories(include)

add_executable(adc_processor
        src/main.c
        src/adc.c
        src/io.c
        src/stats.c
)

target_link_libraries(adc_processor m)