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

# Our sources — both C++ (UI / glue) and C (pctl_ops.c service layer, unchanged
# from the v2.0.0 text-console build). CONFIGURE_DEPENDS makes CMake re-glob on
# every build so newly added source files don't need an explicit reconfigure.
file(GLOB_RECURSE NX_PCTL_SRC CONFIGURE_DEPENDS source/*.cpp source/*.c)

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
# (nacptool → assemble RomFS → elf2nro).
#
# RomFS layout: borealis's built-in resources (hints.json for `"hints/cancel"_i18n`
# lookups, MaterialIcons-Regular.ttf for built-in widget glyphs, sys icons …)
# go in first, then our app resources overlay on top. We strip:
#   - resources/font/        — we use the system shared font instead (~3 MB saved)
#   - resources/img/pokemon/ — demo-only (~1.6 MB saved)
#   - resources/xml/         — demo-only (built-in widgets use inlined XML strings)
if (PLATFORM_SWITCH)
    set(BOREALIS_RESOURCES ${BOREALIS_LIBRARY}/../resources)
    add_custom_target(${PROJECT_NAME}.nro ALL DEPENDS ${PROJECT_NAME}
        # Display name shown in hbmenu (the actual repo / project name is still
        # NX-Pctl-Manager — see set(PROJECT_NAME ...) above).
        COMMAND ${NX_NACPTOOL_EXE} --create "Pctl Manager" "${PROJECT_AUTHOR}"
                "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
                ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
        # Borealis built-ins first …
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${BOREALIS_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/xml
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/img/pokemon
        # … then our app resources overlay (overrides anything we both define).
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro
                --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp
                --romfsdir=${CMAKE_BINARY_DIR}/resources
    )
endif ()
