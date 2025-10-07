# File: CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(pio_joystick C)

set(CMAKE_C_STANDARD 11)

# Find the required libraries
find_library(PIOLIB_LIBRARY NAMES piolib)
find_library(EVDEV_LIBRARY NAMES evdev)
find_library(RT_LIBRARY NAMES rt) # For usleep

if(NOT PIOLIB_LIBRARY OR NOT EVDEV_LIBRARY OR NOT RT_LIBRARY)
    message(FATAL_ERROR "Could not find required libraries: piolib, libevdev, or librt. Please ensure they are installed.")
endif()

# Define the executable
add_executable(pio_joystick main.c)

# Link the libraries to the executable
target_link_libraries(pio_joystick ${PIOLIB_LIBRARY} ${EVDEV_LIBRARY} ${RT_LIBRARY})

# Installation (optional)
install(TARGETS pio_joystick DESTINATION /usr/local/bin)
