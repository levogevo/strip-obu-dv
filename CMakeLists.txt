cmake_minimum_required(VERSION 3.28)
project(strip-obu-dv)

add_executable(${PROJECT_NAME} main.c)

# obuparse
include(ExternalProject)
set(OBUPARSE_SRC "${CMAKE_SOURCE_DIR}/obuparse")
set(OBUPARSE_BINARY_DIR "${CMAKE_BINARY_DIR}/obuparse")
file(
    COPY "${OBUPARSE_SRC}/"
    DESTINATION "${OBUPARSE_BINARY_DIR}/"
)
ExternalProject_add(
    obuparse
    SOURCE_DIR "${OBUPARSE_SRC}"
    BINARY_DIR "${OBUPARSE_BINARY_DIR}"
    # BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)
set(OBUPARSE_LIB "${OBUPARSE_BINARY_DIR}/libobuparse.a")
target_link_libraries(${PROJECT_NAME} PRIVATE ${OBUPARSE_LIB})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)