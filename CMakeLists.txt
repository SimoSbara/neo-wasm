cmake_minimum_required(VERSION 3.16)

#porcheria
set(CMAKE_TOOLCHAIN_FILE "C:/wasi-sdk/share/cmake/wasi-sdk.cmake" CACHE STRING "WASI" FORCE)

project(neo-wasm C)

IF(WIN32)
    include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
ENDIF()


add_executable(app main.c)

target_link_options(app PRIVATE
    "-mexec-model=reactor" 
    "-Wl,--no-entry" #no main()
    "-Wl,--export=draw_asteroids"
    "-Wl,--export=load_asteroids"
    "-Wl,--export=get_num_asteroids"
    "-Wl,--export=alloc_memory"
    "-Wl,--export=free_memory"
    "-Wl,--export=set_timestamp_since_epoch"
    "-Wl,--export=set_timestamp_since_j2000"
)

set_target_properties(app PROPERTIES SUFFIX ".wasm")