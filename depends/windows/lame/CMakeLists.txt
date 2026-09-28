cmake_minimum_required(VERSION 3.5)
project(lame)

file(RENAME ${PROJECT_SOURCE_DIR}/configMS.h ${PROJECT_SOURCE_DIR}/config.h)

# there is an issue with using "Whole program optimization" so disable it
foreach(flag CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG
             CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELEASE
             CMAKE_C_FLAGS_RELWITHDEBINFO)
  if(${flag} MATCHES "/GL")
    string(REPLACE "/GL" "" ${flag} "${${flag}}")
  endif()
endforeach()

add_definitions(-DTAKEHIRO_IEEE754_HACK -D_WINDOWS -DWIN32 -DHAVE_CONFIG_H)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/libmp3lame ${PROJECT_SOURCE_DIR}/mpglib)

set(MP3LAME_SOURCES libmp3lame/bitstream.c
                    libmp3lame/encoder.c
                    libmp3lame/fft.c
                    libmp3lame/gain_analysis.c
                    libmp3lame/id3tag.c
                    libmp3lame/lame.c
                    libmp3lame/mpglib_interface.c
                    libmp3lame/newmdct.c
                    libmp3lame/presets.c
                    libmp3lame/psymodel.c
                    libmp3lame/quantize.c
                    libmp3lame/quantize_pvt.c
                    libmp3lame/reservoir.c
                    libmp3lame/set_get.c
                    libmp3lame/tables.c
                    libmp3lame/takehiro.c
                    libmp3lame/util.c
                    libmp3lame/vbrquantize.c
                    libmp3lame/VbrTag.c
                    libmp3lame/version.c
                    libmp3lame/vector/xmm_quantize_sub.c)

add_library(mp3lame ${MP3LAME_SOURCES})

set(HEADERS ${PROJECT_SOURCE_DIR}/include/lame.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/lame)
install(TARGETS mp3lame DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
