
cmake_minimum_required(VERSION 3.10)

project(MY_PROJECT C)

set(FLAGS
 -g
 -O2
 # -DVALGRIND
 # -DMUDUO_STD_STRING
 -Wall
 -Wextra
 # -m32
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 # -MMD
#  -std=c++17
 -rdynamic
 )

include_directories(include src)
link_directories(lib)

set(LIBS
    pthread
)

file(GLOB_RECURSE SOURCES common/*.c common/*.h src/*.c)

set(TARGET main)
add_executable(${TARGET} main.c ${SOURCES})
target_link_libraries(${TARGET} ${LIBS})

add_executable(client client.c)
# target_link_libraries(${TARGET} ${LIBS})
