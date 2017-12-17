cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_BUILD_TYPE Debug)
project (blocklandjs)
include(uv.cmake)
include(duktape.cmake)

ADD_LIBRARY(duv STATIC
  lib/dukluv/src/duv.c
  lib/dukluv/src/refs.c
  lib/dukluv/src/utils.c
  lib/dukluv/src/loop.c
  lib/dukluv/src/req.c
  lib/dukluv/src/handle.c
  lib/dukluv/src/timer.c
  lib/dukluv/src/stream.c
  lib/dukluv/src/tcp.c
  lib/dukluv/src/pipe.c
  lib/dukluv/src/tty.c
  lib/dukluv/src/fs.c
  lib/dukluv/src/misc.c
  lib/dukluv/src/utils.c
  lib/dukluv/src/uv_schema.c
  lib/dukluv/src/callbacks.c
  lib/dukluv/src/miniz.c
)

ADD_LIBRARY(dschema STATIC
  lib/dukluv/src/schema.c
)

set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
SET (BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE STRING "Directory where lib will install")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where lib will install")
SET (INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The directory the headers are installed in")

ADD_LIBRARY(BlocklandJS SHARED dllmain.cpp Torque.cpp duk_module_node.c)

SET_TARGET_PROPERTIES(BlocklandJS PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -g")
target_link_libraries(BlocklandJS duv dschema uv duktape)
