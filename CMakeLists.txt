cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Z-VultiplierLib)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(glfw3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/glfw-3.5.1.bin.WIN64/lib-vc2022/cmake/glfw3")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm QUIET)
if(NOT TARGET glm::glm)
    add_library(glm_manual INTERFACE)
    target_include_directories(glm_manual INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/glm-master")
    set(GLM_TARGET glm_manual)
else()
    set(GLM_TARGET glm::glm)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)



set(SOURCES
    #################################################
    src/Core/Logger.cpp
    src/Core/Initializer.cpp
    src/Core/Clock.cpp
    src/Core/Utils.cpp
    src/Window/Handle.cpp
    src/Window/WindowContext.cpp
    src/Render/Painter.cpp
    src/Audio/Audio.cpp
    src/Audio/Music.cpp
    src/Game/GameObject.cpp
    src/Game/Algos.cpp
    src/Game/Scene.cpp
    src/Render/Animation.cpp
    src/Physics/Physics.cpp
    #################################################
)
set(HEADERS
    #################################################
    include/Logger.hpp
    include/Initializer.hpp
    include/Clock.hpp
    include/Utils.hpp
    include/Handle.hpp
    include/WindowContext.hpp
    include/Painter.hpp
    include/Matrix.hpp
    include/Audio.hpp
    include/Music.hpp
    include/GameObject.hpp
    include/Algos.hpp
    include/Scene.hpp
    include/Animation.hpp
    include/Physics.hpp
    include/Z-Vultiplier.hpp
    #################################################
)
add_library(Z-VultiplierLib STATIC ${SOURCES} ${HEADERS})
if(MSVC)
    target_compile_options(Z-VultiplierLib PRIVATE /utf-8)
else()
    target_compile_options(Z-VultiplierLib PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
target_include_directories(Z-VultiplierLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/gli/gli>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/VEFontCache>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/miniaudio-0.11.25>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/box2d-3.1.1/include/box2d>
    ${HARFBUZZ_INCLUDE_DIRS}
)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./lib)
set(BOX2D_DIR "${CMAKE_CURRENT_SOURCE_DIR}/LIBRARIES/box2d-3.1.1")
set(BOX2D_LIB "${BOX2D_DIR}/build/src/libbox2d.a")
target_link_libraries(Z-VultiplierLib PUBLIC
    Vulkan::Vulkan
    glfw
    ${GLM_TARGET}
    ${HARFBUZZ_LIBRARIES}
    ${BOX2D_LIB}
)
add_executable(test_app tests/test.cpp)
target_link_libraries(test_app PRIVATE Z-VultiplierLib)