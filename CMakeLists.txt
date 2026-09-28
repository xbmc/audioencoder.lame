cmake_minimum_required(VERSION 3.5)
project(audioencoder.lame)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(Iconv REQUIRED)
find_package(Lame REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                    ${iconv_INCLUDE_DIRS}
                    ${LAME_INCLUDE_DIRS})

set(LAME_SOURCES src/EncoderLame.cpp)

set(DEPLIBS ${LAME_LIBRARIES} ${ICONV_LIBRARY})

build_addon(audioencoder.lame LAME DEPLIBS)

include(CPack)
