project(audioencoder.lame)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(Lame REQUIRED)
if(WIN32)
  find_package(MPGHIP REQUIRED)
endif()

include_directories(${KODI_INCLUDE_DIR}
                    ${LAME_INCLUDE_DIRS})

set(LAME_SOURCES src/EncoderLame.cpp)

set(DEPLIBS ${LAME_LIBRARIES})
if(WIN32)
  list(APPEND DEPLIBS ${MPGHIP_LIBRARIES})
endif()

build_addon(audioencoder.lame LAME DEPLIBS)

include(CPack)
