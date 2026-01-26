cmake_minimum_required(VERSION 4.2.0)

project(font_tool VERSION 1.0.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(use_wayland "If this option is set, wayland will be used instead of X11. Default value ON." ON)

add_subdirectory(3rd-party/glfw)
add_subdirectory(3rd-party/volk)

add_subdirectory(3rd-party/cglm)
set(CGLM_SHARED OFF CACHE BOOL "Shared build" FORCE)
set(CGLM_STATIC ON  CACHE BOOL "Static build" FORCE)
target_compile_definitions(cglm PUBLIC CGLM_FORCE_DEPTH_ZERO_TO_ONE)

add_executable(font_tool
                  src/main.c
                  src/application.c
                  src/window/window.c
                  src/window/input_callbacks.c
                  include/application.h
                  include/window/window.h
                  include/window/window_errors.h
                  include/window/input_callbacks.h
               )

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      target_compile_definitions(font_tool PRIVATE DEBUG)
endif()

target_include_directories(font_tool PRIVATE include)
target_compile_definitions(font_tool PUBLIC GLFW_INCLUDE_NONE)
# New GCC doesn't include -std=c23 option if C23 standard is used, because it is default standard used by GCC. 
# Force the C standard option to get rid of the language server issues. 
target_compile_options(font_tool PRIVATE -std=c23)

if(WIN32)
   target_compile_definitions(font_tool PUBLIC GLFW_EXPOSE_NATIVE_WIN32 GLFW_EXPOSE_NATIVE_WGL)

   if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      target_link_options(font_tool PRIVATE -mwindows)
   endif()
elseif(APPLE)
   target_compile_definitions(font_tool PUBLIC GLFW_EXPOSE_NATIVE_COCOA GLFW_EXPOSE_NATIVE_NSGL)
else()
   if(${use_wayland})
      target_compile_definitions(font_tool PUBLIC GLFW_EXPOSE_NATIVE_WAYLAND USE_WAYLAND)
   else()
      target_compile_definitions(font_tool PUBLIC GLFW_EXPOSE_NATIVE_X11 GLFW_EXPOSE_NATIVE_GLX)
   endif()
endif()

target_link_libraries(font_tool
                        PRIVATE glfw
                        PRIVATE volk
                        PRIVATE cglm
                     )
