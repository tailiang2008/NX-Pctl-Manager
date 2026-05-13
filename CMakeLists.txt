cmake_minimum_required(VERSION 3.10)

# Borealis library lives in extern/ as a git submodule (xfangfang/wiliwili).
set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/extern/borealis/library)

# Pulls in PLATFORM_DESKTOP / PLATFORM_SWITCH / USE_DEKO3D / etc. options.
include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)

# Sets up the devkitPro cross-toolchain when PLATFORM_SWITCH=ON.
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

project(nx_pctl_manager LANGUAGES CXX C)

# App metadata — kept in sync with the legacy Makefile.
set(VERSION_MAJOR "3")
set(VERSION_MINOR "0")
set(VERSION_ALTER "0")
set(PROJECT_TITLEID "")
set(PROJECT_AUTHOR "Taylor")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# Optional: include the read-only "Dump current config" diagnostic item
# (mirrors the legacy `make PROBE=1`). Off by default; pass -DPCTL_PROBE=ON.
option(PCTL_PROBE "Include the read-only diagnostic dump menu item" OFF)

# Build borealis as a static library at <build>/borealis_library/.
add_subdirectory(${BOREALIS_LIBRARY} ${CMAKE_BINARY_DIR}/borealis_library)

find_package(Threads REQUIRED)
list(APPEND APP_PLATFORM_LIB ${CMAKE_THREAD_LIBS_INIT})

# Our C++ sources. Step (a) only wires main.cpp + the MainActivity stub;
# pctl_ops.{h,c} comes back into the build at step (b).
file(GLOB_RECURSE NX_PCTL_SRC source/*.cpp)

if (PLATFORM_SWITCH)
    # libnx applet/main wrapper from borealis (handles HOME / nvServices / etc.).
    list(APPEND NX_PCTL_SRC ${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c)
endif ()

# `program_target` from borealis's toolchain knows how to make an .elf vs .exe.
program_target(${PROJECT_NAME} "${NX_PCTL_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

target_include_directories(${PROJECT_NAME} PRIVATE source ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections ${APP_PLATFORM_OPTION})
target_link_libraries(${PROJECT_NAME} PRIVATE borealis ${APP_PLATFORM_LIB})

if (PCTL_PROBE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PCTL_PROBE=1)
endif ()

# .nro packaging — mirrors extern/borealis/CMakeLists.txt's Switch branch
# (nacptool → copy resources → strip font/ → elf2nro).
if (PLATFORM_SWITCH)
    add_custom_target(${PROJECT_NAME}.nro ALL DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "NX Pctl Manager" "${PROJECT_AUTHOR}"
                "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
                ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        # Drop bundled font — use the system shared font instead (saves ~3 MB).
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro
                --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp
                --romfsdir=${CMAKE_BINARY_DIR}/resources
    )
endif ()
