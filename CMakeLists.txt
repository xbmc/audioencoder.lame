project(audioencoder.lame)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(xbmc REQUIRED)
find_package(Lame REQUIRED)

include_directories(${XBMC_INCLUDE_DIR}
                    ${LAME_INCLUDE_DIRS})

set(LAME_SOURCES src/EncoderLame.cpp)

set(DEPLIBS ${LAME_LIBRARIES})

build_addon(audioencoder.lame LAME DEPLIBS)
