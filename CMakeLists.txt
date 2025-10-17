cmake_minimum_required(VERSION 3.28)
project(strip-obu-dv)

option(ASAN "Compile with AddressSanitizer" OFF)
option(UBSAN "Compile with UndefinedBehaviorSanitizer" OFF)

add_executable(${PROJECT_NAME} main.c)

set(COMPILE_OPTS "-Wall" "-Werror")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND COMPILE_OPTS "-O3" "-flto")
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

# obuparse
include(ExternalProject)
set(OBUPARSE_SRC "${CMAKE_SOURCE_DIR}/obuparse")
set(OBUPARSE_BINARY_DIR "${CMAKE_BINARY_DIR}/obuparse")
file(COPY "${OBUPARSE_SRC}/" DESTINATION "${OBUPARSE_BINARY_DIR}/")
set(OBUPARSE_LIB "libobuparse.a")
set(OBUPARSE_LIB_PATH "${OBUPARSE_BINARY_DIR}/${OBUPARSE_LIB}")
ExternalProject_Add(
  obuparse
  SOURCE_DIR "${OBUPARSE_SRC}"
  BINARY_DIR "${OBUPARSE_BINARY_DIR}"
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_COMMAND make ${OBUPARSE_LIB}
  BUILD_BYPRODUCTS "${OBUPARSE_LIB_PATH}")
add_dependencies(${PROJECT_NAME} obuparse)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OBUPARSE_LIB_PATH})

if(ASAN AND UBSAN)
  message(FATAL_ERROR "cannot enable both ASAN and UBSAN")
elseif(ASAN)
  list(APPEND COMPILE_OPTS "-fsanitize=address")
elseif(UBSAN)
  list(APPEND COMPILE_OPTS "-fsanitize=undefined")
endif(ASAN AND UBSAN)

if(NOT COMPILE_OPTS STREQUAL "")
  target_compile_options(${PROJECT_NAME} PRIVATE "${COMPILE_OPTS}")
  target_link_options(${PROJECT_NAME} PRIVATE "${COMPILE_OPTS}")
endif(NOT COMPILE_OPTS STREQUAL "")
