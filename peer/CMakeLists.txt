add_executable(oanpeer
        main.cpp
        NetworkMapper.cpp
        NetworkMapper.h
        peer_conf.h
)

find_package(jsoncpp REQUIRED)

target_link_libraries(oanpeer oannetutils oancommon)
target_link_libraries(oanpeer jsoncpp)
target_include_directories(oanpeer PUBLIC ${CMAKE_SOURCE_DIR})