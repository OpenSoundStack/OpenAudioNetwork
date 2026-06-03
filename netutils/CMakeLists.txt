set(OAN_UTILS_SOURCES
        udp.cpp
        udp.h
        LowLatSocket.cpp
        LowLatSocket.h
)

if(NOT EMBEDDED_BUILD)
    list(APPEND OAN_UTILS_SOURCES
            platform/rt.h
            platform/rt.cpp
    )
endif()

if(OAN_HOST_BACKENDS)
    list(APPEND OAN_UTILS_SOURCES
            transport/ITransport.h
            transport/ITransport.cpp
            transport/SimTransport.h
            transport/SimTransport.cpp
            transport/RawMacTransport.h
            transport/RawMacTransport.cpp
    )
    if(NOT APPLE)
        list(APPEND OAN_UTILS_SOURCES
                transport/RawLinuxTransport.h
                transport/RawLinuxTransport.cpp
        )
    endif()
endif()

if(EMBEDDED_BUILD)
    add_library(oannetutils STATIC ${OAN_UTILS_SOURCES})
else ()
    add_library(oannetutils SHARED ${OAN_UTILS_SOURCES})
endif (EMBEDDED_BUILD)

target_include_directories(oannetutils PUBLIC ${PROJECT_SOURCE_DIR})

if(APPLE)
    target_link_options(oannetutils PRIVATE LINKER:-undefined,dynamic_lookup)
elseif(UNIX)
    # LowLatSocket calls NetworkMapper::get_mac_by_uid, which lives in
    # oancommon — and oancommon already links oannetutils, so we can't
    # close the cycle at the CMake level. macOS' dynamic_lookup option
    # papers over this; on Linux we tell ld to defer the symbol to load
    # time. The engine binary always loads both .so's so the symbol
    # resolves there.
    target_link_options(oannetutils PRIVATE LINKER:--unresolved-symbols=ignore-in-shared-libs)
endif()