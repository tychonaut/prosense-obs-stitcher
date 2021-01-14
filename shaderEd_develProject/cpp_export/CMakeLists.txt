cmake_minimum_required(VERSION 3.1)

project(FrustumToEquirect)


# Heavily modified after some pretty buggy export as c++ project by ShaderEd.
# I hade to export a this  stage, as I need to be able to calculate ViewProjection matrices from frustum data.
# This COULD be done in the shader at a HUGE performance hit 
# (and at the cost of re-implementing functionality that already exist for basically every language except shader languages).
# So I decided to leave the editor and to continue
# I don't wanna complain, I was expacting this, an a messy export is better than none at all.

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")


set(MY_DEPENDENCY_DIR "${PROJECT_SOURCE_DIR}/deps" CACHE STRING "Directory to external dependencies (extract deps.7z)")

set (CMAKE_PREFIX_PATH 
	"${MY_DEPENDENCY_DIR}/glm-0.9.9.8/glm" 
	"${MY_DEPENDENCY_DIR}/glew-2.2.0-win32/glew-2.2.0"
	"${MY_DEPENDENCY_DIR}/SDL2-devel-2.0.14-VC/SDL2-2.0.14"
)



set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./bin)

# source code
set(SOURCES
	frustumToEquirect.cpp
)

# cmake toolchain 
if(CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
endif(CMAKE_TOOLCHAIN_FILE)

# opengl
find_package(OpenGL REQUIRED)

# glm
find_package(GLM REQUIRED)


# glew
#set(GLEW_USE_STATIC_LIBS ON)
find_package(GLEW REQUIRED)



# sdl2
if(WIN32 OR APPLE OR NOT ${USE_PKG_CHECK_SDL})
  find_package(SDL2 REQUIRED)
else()
  # find sdl2 on linux
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2)
endif()




# create executable
add_executable(FrustumToEquirect ${SOURCES})

# properties
set_target_properties(FrustumToEquirect PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

# include directories
#target_include_directories(FrustumToEquirect PRIVATE  ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
# SDL2_INCLUDE_DIR, without 'S'
target_include_directories(FrustumToEquirect PRIVATE  ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})


# link libraries
target_link_libraries(FrustumToEquirect ${GLM_LIBRARY_DIRS} ${OPENGL_LIBRARIES})

if(WIN32)
	# link specific win32 libraries
	## SDL makes problems...
	#target_link_libraries(FrustumToEquirect GLEW::GLEW SDL2::SDL2)
	target_link_libraries(FrustumToEquirect GLEW::glew ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY})
	
else()
	# link linux libraries
	target_link_libraries(FrustumToEquirect ${GLEW_LIBRARIES} ${SDL2_LIBRARIES})
endif()

if (NOT MSVC)
	target_compile_options(FrustumToEquirect PRIVATE -Wno-narrowing)
endif()

####-------------
# awful hack to get the DLLs found: just copy them to the location where the exe will be run in by visual studio...
get_filename_component(GLEW_SHARED_LIBRARIES_directory ${GLEW_SHARED_LIBRARIES} DIRECTORY)
add_custom_command(TARGET FrustumToEquirect POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${GLEW_SHARED_LIBRARIES_directory}/../../../bin/Release/x64/glew32.dll ${FrustumToEquirect_BINARY_DIR}/. )

get_filename_component(SDL_SHARED_LIBRARIES_directory ${SDL2_LIBRARY} DIRECTORY)
add_custom_command(TARGET FrustumToEquirect POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SDL_SHARED_LIBRARIES_directory}/SDL2.dll ${FrustumToEquirect_BINARY_DIR}/. )
####-------------



