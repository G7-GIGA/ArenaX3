cmake_minimum_required(VERSION 3.10.2)
project(ArenaX3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(arenax3 SHARED
    native-lib.cpp
    Emulator.cpp
    ppu_core.cpp
    Platform_Utils/Logger.cpp
    Platform_Utils/Input/Touch_Controls.cpp
    Platform_Utils/Input/Gamepad_Handler.cpp
    Platform_Utils/Audio/Oboe_Backend.cpp
    Platform_Utils/Loader/Elf_Loader.cpp
    Platform_Utils/Loader/Self_Decryptor.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(arenax3
    ${log-lib}
    ${android-lib}
)

target_include_directories(arenax3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform_Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform_Utils/Input
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform_Utils/Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform_Utils/Loader
)