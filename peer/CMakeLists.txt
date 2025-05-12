add_executable(oanpeer
        main.cpp
        peer_conf.h
)

find_package(jsoncpp REQUIRED)

target_link_libraries(oanpeer oannetutils oancommon)
target_link_libraries(oanpeer jsoncpp)
target_include_directories(oanpeer PUBLIC ${PROJECT_SOURCE_DIR})