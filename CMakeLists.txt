cmake_minimum_required(VERSION 3.16)
project(GenesisEngine)

# Set C++ standard (SFML 3.0 requires C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set SFML directory to our local copy
set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML-3.0.0/lib/cmake/SFML")

# Find SFML package - SFML 3.0 uses different component names
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

# Add our source files
add_executable(GenesisEngine src/main.cpp)

# Link SFML to our executable - SFML 3.0 uses SFML:: namespace
target_link_libraries(GenesisEngine PRIVATE SFML::Graphics SFML::Window SFML::System)

# Copy SFML DLLs to output directory (Windows only)
if(WIN32)
    file(GLOB SFML_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/SFML-2.6.1/bin/*.dll")
    foreach(dll ${SFML_DLLS})
        add_custom_command(TARGET GenesisEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${dll} $<TARGET_FILE_DIR:GenesisEngine>)
    endforeach()
endif()