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
    "-Wl,--export=get_planet_by_position"
    "-Wl,--export=get_asteroid_name"
    "-Wl,--export=get_asteroid_min_diameter"
    "-Wl,--export=get_asteroid_max_diameter"
    "-Wl,--export=get_asteroid_miss_distance"
    "-Wl,--export=get_asteroid_velocity"
    "-Wl,--export=get_asteroid_timestamp"
    "-Wl,--export=is_asteroid_hazardous"
)

set_target_properties(app PROPERTIES SUFFIX ".wasm")