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
    # oancommon — and oancommon already PUBLIC-links oannetutils, so we
    # can't close the cycle by linking the other way at CMake level.
    # Mac papers over this with -undefined,dynamic_lookup; the Linux
    # equivalent is -z undefs (allow undefined symbols in this .so;
    # they'll resolve at load time when the engine pulls in both .so's).
    target_link_options(oannetutils PRIVATE LINKER:-z,undefs)
endif()