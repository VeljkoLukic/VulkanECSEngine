cmake_minimum_required(VERSION 3.20)
project(VulkanECSEngine VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

find_package(Vulkan REQUIRED)

find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
    set(GLFW_TARGET glfw)
else()
    set(GLFW_TARGET glfw3)
endif()

find_package(glm QUIET)
if(NOT glm_FOUND)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glm)
endif()

if(TARGET glm::glm)
    set(GLM_TARGET glm::glm)
elseif(TARGET glm)
    set(GLM_TARGET glm)
else()
    set(GLM_TARGET "")
endif()

find_program(GLSLC glslc
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"
        ${Vulkan_GLSLC_EXECUTABLE}
)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Make sure the Vulkan SDK is installed and VULKAN_SDK env var is set.")
endif()

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SHADER_SOURCES
    ${SHADER_SRC_DIR}/*.vert
    ${SHADER_SRC_DIR}/*.frag
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV ${SHADER_BIN_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT  ${SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BIN_DIR}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${SPV})
endforeach()
add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

file(GLOB_RECURSE ENGINE_SOURCES ${CMAKE_SOURCE_DIR}/engine/*.cpp)

add_library(Engine STATIC ${ENGINE_SOURCES})
add_dependencies(Engine Shaders)

target_include_directories(Engine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
)

if(GLM_TARGET STREQUAL "")
    target_include_directories(Engine PUBLIC ${glm_SOURCE_DIR})
endif()

target_link_libraries(Engine PUBLIC
    Vulkan::Vulkan
    ${GLFW_TARGET}
    ${GLM_TARGET}
)

target_compile_definitions(Engine PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    SHADER_DIR="${SHADER_BIN_DIR}/"
    $<$<CONFIG:Debug>:VULKAN_VALIDATION>
)

target_compile_options(Engine PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
)

file(GLOB_RECURSE GAME_SOURCES ${CMAKE_SOURCE_DIR}/game/*.cpp)

add_executable(Game ${GAME_SOURCES})
add_dependencies(Game Shaders)

target_include_directories(Game PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(Game PRIVATE Engine)

target_compile_options(Game PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
)