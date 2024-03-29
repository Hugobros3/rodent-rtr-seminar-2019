set(CPUINFO_PATH "/proc/cpuinfo" CACHE STRING "Path to the CPU info file on the system")
set(TARGET_PLATFORM "" CACHE STRING "Target platform for the converter tool. Leave empty to autodetect CPU. Use converter --help to list possible platforms.")
set(TARGET_DEVICE "" CACHE STRING "Target device on the selected platform. Leave empty to use the default.")
set(SCENE_FILE "${PROJECT_SOURCE_DIR}/testing/cornell_box.obj" CACHE FILEPATH "Absolute path to an OBJ scene")
set(MEGAKERNEL_FUSION ON CACHE BOOL "Set to true to enable simple shader fusion for megakernel mappings")
set(MAX_PATH_LEN "64" CACHE STRING "Maximum path length")
set(DISABLE_GUI OFF CACHE BOOL "Set to true to disable GUI")
set(USE_FAST_BUILDER ON CACHE BOOL "Set to false to use the default rodent SBVH builder")
set(SPP "4" CACHE STRING "Samples per pixel")
if (SCENE_FILE STREQUAL "")
    message(FATAL_ERROR "Please specify a valid OBJ scene in the SCENE_FILE variable")
endif ()
set(CONVERTER_OPTIONS "")
if (NOT TARGET_PLATFORM STREQUAL "")
    set(CONVERTER_OPTIONS "--target" "${TARGET_PLATFORM}")
endif ()
if (NOT TARGET_DEVICE STREQUAL "")
    set(CONVERTER_OPTIONS ${CONVERTER_OPTIONS} "--device" "${TARGET_DEVICE}")
    if (MEGAKERNEL_FUSION AND (TARGET_PLATFORM STREQUAL "nvvm-megakernel" OR TARGET_PLATFORM STREQUAL "amdgpu-megakernel"))
        set(CONVERTER_OPTIONS ${CONVERTER_OPTIONS} "--fusion")
    endif ()
endif ()

set(RODENT_SRCS
        core/color.impala
        core/common.impala
        core/cpu_common.impala
        core/matrix.impala
        core/random.impala
        core/sort.impala
        core/vector.impala
        render/image.impala
        render/camera.impala
        render/geometry.impala
        render/light.impala
        render/material.impala
        render/renderer.impala
        render/scene.impala
        render/driver.impala
        render/mapping_cpu.impala
        render/mapping_gpu.impala
        traversal/intersection.impala
        traversal/stack.impala
        traversal/mapping_cpu.impala
        traversal/mapping_gpu.impala)

set(DRIVER_SRCS
        driver/driver.cpp
        driver/interface.cpp
        driver/interface.h
        driver/obj.cpp
        driver/obj.h
        driver/image.cpp
        driver/image.h
        driver/bvh.h
        driver/float2.h
        driver/float3.h
        driver/float4.h
        driver/file_path.h
        driver/common.h
        driver/color.h)

set(CONVERTER_SRCS
        converter/converter_interface.h
        driver/converter.cpp
        driver/obj.cpp
        driver/obj.h
        driver/file_path.h
        driver/interface.h
        driver/bvh.h)

set(CONVERTER_IMPALA_SRCS
        converter/converter.impala
        core/common.impala
        core/vector.impala
        )

anydsl_runtime_wrap(RODENT_OBJS
        NAME "rodent"
        CLANG_FLAGS ${CLANG_FLAGS}
        IMPALA_FLAGS --log-level info
        FILES ${RODENT_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/main.impala)

anydsl_runtime_wrap(DISCARD_TMP_OBJS
        NAME "interface"
        FILES ${RODENT_SRCS} dummy_main.impala
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/driver/interface)

anydsl_runtime_wrap(CONVERTER_OBJS
        NAME "converter"
        CLANG_FLAGS ${CLANG_FLAGS}
        IMPALA_FLAGS --log-level info
        FILES ${BVH_BUILDER_LIBRARY_IMPALA_FILES} ${CONVERTER_IMPALA_SRCS}
        #INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/converter/converter_interface
        )

message("${BVH_BUILDER_LIBRARY_IMPALA_FILES} ")

if (NOT DISABLE_GUI)
    if (PREFER_SDL2)
        find_package(SDL2 REQUIRED)
    else ()
        find_package(SDL REQUIRED)
    endif ()
endif ()
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(TBB REQUIRED)
find_package(LZ4 REQUIRED)

if (USE_FAST_BUILDER)
    message("Using fast builder")
    add_definitions(-DUSE_FAST_BUILDER)
else ()
    message("Using high-quality SBVH builder")
endif ()

add_executable(converter ${CONVERTER_SRCS} ${CONVERTER_OBJS})
target_include_directories(converter PUBLIC ${LZ4_INCLUDE_DIR})
target_link_libraries(converter ${LZ4_LIBRARY} bvh_debug ${AnyDSL_runtime_LIBRARIES})
target_compile_definitions(converter PUBLIC -DCPUINFO_PATH="${CPUINFO_PATH}")
if (COLORIZE)
    target_compile_definitions(converter PUBLIC -DCOLORIZE)
endif ()
if (EMBREE_FOUND)
    target_include_directories(converter PUBLIC ${EMBREE_ROOT_DIR}/include ${EMBREE_ROOT_DIR} ${EMBREE_LIBRARY_DIR})
    target_link_libraries(converter ${EMBREE_DEPENDENCIES})
    target_compile_definitions(converter PUBLIC ${EMBREE_DEFINITIONS} -DENABLE_EMBREE_BVH)
endif ()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.impala
        COMMAND converter ${SCENE_FILE} ${CONVERTER_OPTIONS} --max-path-len ${MAX_PATH_LEN} --samples-per-pixel ${SPP}
        COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_BINARY_DIR}/main.impala ${CMAKE_CURRENT_BINARY_DIR}/main.impala
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${SCENE_FILE} converter)

add_custom_target(convert DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/main.impala)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/main.impala PROPERTIES GENERATED TRUE)

add_library(driver ${DRIVER_SRCS})
target_include_directories(driver PUBLIC ${LZ4_INCLUDE_DIR})
if (EMBREE_FOUND)
    target_include_directories(driver PUBLIC ${EMBREE_ROOT_DIR}/include ${EMBREE_ROOT_DIR} ${EMBREE_LIBRARY_DIR} ${PNG_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS} ${LZ4_INCLUDE_DIR})
    target_link_libraries(driver ${EMBREE_DEPENDENCIES})
    target_compile_definitions(driver PUBLIC -DENABLE_EMBREE_DEVICE)
endif ()
if (NOT DISABLE_GUI)
    if (PREFER_SDL2)
        target_include_directories(driver PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(driver ${SDL2_LIBRARY})
    else ()
        target_include_directories(driver PUBLIC ${SDL_INCLUDE_DIRS})
        target_link_libraries(driver ${SDL_LIBRARY})
    endif ()
else ()
    target_compile_definitions(driver PUBLIC -DDISABLE_GUI)
endif ()
if (COLORIZE)
    target_compile_definitions(driver PUBLIC -DCOLORIZE)
endif ()

add_executable(rodent ${RODENT_OBJS})
target_link_libraries(rodent driver ${AnyDSL_runtime_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${TBB_LIBRARIES} ${LZ4_LIBRARY})

if (SCENE_FILE STREQUAL "${PROJECT_SOURCE_DIR}/testing/cornell_box.obj")
    # Test rodent when the cornell box is used
    add_test(NAME rodent_cornell COMMAND ${CMAKE_COMMAND} -DRODENT=$<TARGET_FILE:rodent> -DIM_COMPARE=${ImageMagick_compare_EXECUTABLE} "-DRODENT_ARGS=--eye;0;1;2.7;--dir;0;0;-1;--up;0;1;0" -DTESTING_DIR=${PROJECT_SOURCE_DIR}/testing -DRODENT_DIR=${CMAKE_BINARY_DIR} -DRODENT_OUTPUT=rodent-cornell-output -P ${PROJECT_SOURCE_DIR}/cmake/test/run_rodent.cmake)
endif ()
