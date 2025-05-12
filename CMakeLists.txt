cmake_minimum_required(VERSION 3.28.2)

include(../sdk/config.cmake)
project(sieelf_time_zone_picker C)

include_directories("../libsie/src/include")
link_libraries(crt crt_helper c)
file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(TimeZonePicker_ELKA ${SOURCES})
target_sdk_setup(TimeZonePicker_ELKA ELKA)
target_link_directories(TimeZonePicker_ELKA PUBLIC "../libsie/build/lib/ELKA")
target_compile_options(TimeZonePicker_ELKA BEFORE PUBLIC -std=c11 -Wall -Werror)
