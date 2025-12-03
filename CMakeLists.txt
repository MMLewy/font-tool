cmake_minimum_required(VERSION 4.2.0)

project(font_tool VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(3rd-party/glfw)
add_subdirectory(3rd-party/volk)


add_subdirectory(3rd-party/cglm)
set(CGLM_SHARED OFF CACHE BOOL "Shared build" FORCE)
set(CGLM_STATIC ON  CACHE BOOL "Static build" FORCE)
target_compile_definitions(cglm PUBLIC CGLM_FORCE_DEPTH_ZERO_TO_ONE CGLM_FORCE_LEFT_HANDED)

add_executable(font_tool src/main.c)

target_link_libraries(font_tool
                        PRIVATE glfw
                        PRIVATE volk
                        PRIVATE cglm
                     )
