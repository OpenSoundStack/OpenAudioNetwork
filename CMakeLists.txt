cmake_minimum_required(VERSION 3.20)
project(OpenAudioNetwork)

set(CMAKE_CXX_STANDARD 20)

if(NO_THREADS)
    add_compile_definitions(NO_THREADS)
endif (NO_THREADS)

# OAN_HOST_BACKENDS: switches in the host-side transport seam (SimTransport,
# RawLinuxTransport, RawMacTransport). The define is normally injected by
# the parent OALS build; mirror the logic here so OAN is self-buildable
# (e.g. for CI of OAN-only changes). Forced ON on macOS — there is no
# other working transport on Apple at the moment.
option(OAN_HOST_BACKENDS "Build host dev transports + RT shim" OFF)
if(APPLE)
    set(OAN_HOST_BACKENDS ON CACHE BOOL "Build host dev transports + RT shim" FORCE)
endif()
if(OAN_HOST_BACKENDS)
    add_compile_definitions(OAN_HOST_BACKENDS)
endif()

add_subdirectory(common)
add_subdirectory(netutils)

option(OAN_BUILD_TESTS "Build OAN unit tests" ON)
if(OAN_BUILD_TESTS AND NOT EMBEDDED_BUILD)
    enable_testing()
    add_subdirectory(test)
endif()