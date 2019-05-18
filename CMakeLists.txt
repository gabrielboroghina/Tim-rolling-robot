cmake_minimum_required(VERSION 3.13)
project(Tim)

set(NAME tim)

set(MCU atmega324p)

set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-g++)

set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

add_compile_options(
        -mmcu=${MCU}
        -Os
        -Wall
)

add_executable(${NAME}.elf main.cpp usart.c esp8266.cpp esp8266.h)

add_custom_command(TARGET ${NAME}.elf POST_BUILD COMMAND
        avr-objcopy -j .text -j .data -O ihex ${NAME}.elf ${NAME}.hex)
