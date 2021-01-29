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




#TODO trigger build (and install or local linking) from plugin build
set(EXPAT_INSTALL_DIR "C:/Program Files/expat/" CACHE STRING "Install directory of expat xml C library")


set (CMAKE_PREFIX_PATH 
	"${MY_DEPENDENCY_DIR}/glm-0.9.9.8/glm" 
	"${MY_DEPENDENCY_DIR}/glew-2.2.0-win32/glew-2.2.0"
	"${MY_DEPENDENCY_DIR}/SDL2-devel-2.0.14-VC/SDL2-2.0.14"
	"${EXPAT_INSTALL_DIR}"
)



#set(ENV{SDL2DIR} "${MY_DEPENDENCY_DIR}/SDL2-devel-2.0.14-VC/SDL2-2.0.14")

#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./bin)

# -----------------------------------------------------------------------------

# cmake toolchain 
if(CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
endif(CMAKE_TOOLCHAIN_FILE)





# -----------------------------------------------------------------------------
# clusti (ClusterStitching) library source code


# share some code with OBS plugin. This project here 
# is the playground for non-OBS-related development, after all
set(clusti_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../deps/clusti/include")
set(clusti_SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../deps/clusti/src")


set(clusti_HEADERS_PUBLIC
    ${clusti_INCLUDE_DIR}/clusti.h
    ${clusti_INCLUDE_DIR}/clusti_types.h
    ${clusti_INCLUDE_DIR}/clusti_math.h
)


set(clusti_HEADERS_PRIVATE
    ${clusti_SOURCES_DIR}/clusti_types_priv.h
    ${clusti_SOURCES_DIR}/clusti_status_priv.h
    ${clusti_SOURCES_DIR}/clusti_mem_priv.h
)

set(clusti_SOURCES
    ${clusti_SOURCES_DIR}/clusti.c
    ${clusti_SOURCES_DIR}/clusti_readConfig.c
    ${clusti_SOURCES_DIR}/clusti_math.c
    ${clusti_SOURCES_DIR}/clusti_status.c
    ${clusti_SOURCES_DIR}/clusti_mem.c
)




# -----------------------------------------------------------------------------
# App source code

# app
set(APP_SOURCES
	frustumToEquirect.cpp
)






# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# Find external libraries


# -----------------------------------------------------------------------------
# graphene pure-C alternative to GLM math

# Dirty hack, after failing to integrate graphene's meson build into CMake'
# "ExternalProject_Add" workflow: 
# Because meson build needs the visual studio command prompt to detect the
# MSVC compiler, it must be invoked from  cmake somehow. Resarach gave
# some very ugly hardcode- results like 
# https://www.codeproject.com/Questions/706617/How-to-open-Visual-studio-command-Prompt-using-Win
# rendering any kind of partability useless. So I will just link to the
# headers and libs of the graphene install, which is expected to be 
# built & installed "by hand" before.

#TODO maybe outsource as FindGraphene.cmake...

if(WIN32)
	set(root_dir "C:")
else()
	set(root_dir "")
endif()


SET(graphene_SEARCH_PATHS
	"${root_dir}/bin"
	"${root_dir}/lib"
	"${root_dir}/include"
)
#message("${graphene_SEARCH_PATHS}")

FIND_PATH(graphene_INCLUDE_DIR graphene.h
	HINTS
	$ENV{grapheneDIR}
	PATH_SUFFIXES  graphene-1.0
	PATHS ${graphene_SEARCH_PATHS}
)


FIND_PATH(graphene_CONFIG_INCLUDE_DIR graphene-config.h
	HINTS
	$ENV{grapheneDIR}
	PATH_SUFFIXES graphene-1.0/include
	PATHS ${graphene_SEARCH_PATHS}
)


FIND_LIBRARY(graphene_LIBRARY
	NAMES graphene-1.0
	HINTS
	$ENV{grapheneDIR}
	PATH_SUFFIXES lib64 lib lib/x64
	PATHS ${graphene_SEARCH_PATHS}
)

# -----------------------------------------------------------------------------





# -----------------------------------------------------------------------------
#pure-C xml library 
find_package(EXPAT REQUIRED)


# -----------------------------------------------------------------------------
# opengl
find_package(OpenGL REQUIRED)

# -----------------------------------------------------------------------------
# glm
find_package(GLM REQUIRED)

# -----------------------------------------------------------------------------
# glew 
#set(GLEW_USE_STATIC_LIBS ON)
find_package(GLEW REQUIRED)


# -----------------------------------------------------------------------------
# sdl2 
if(WIN32 OR APPLE OR NOT ${USE_PKG_CHECK_SDL})
  find_package(SDL2 REQUIRED)
else()
  # find sdl2 on linux
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2)
endif()
#set(SDL2MAIN_LIBRARY "${PROJECT_SOURCE_DIR}/deps/SDL2-devel-2.0.14-VC/SDL2-2.0.14/lib/x64/SDL2main.lib"
#	CACHE STRING "some .lib required, though not set by find-script")







# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# create clusti library


add_library(clusti
    ${clusti_HEADERS_PUBLIC}
    ${clusti_HEADERS_PRIVATE}
    ${clusti_SOURCES}
)


set_target_properties(clusti PROPERTIES
    C_STANDARD 11
)

target_include_directories(clusti PRIVATE 
	${clusti_INCLUDE_DIR}
    ${clusti_SOURCES_DIR}
    ${EXPAT_INCLUDE_DIRS}
	${graphene_INCLUDE_DIR}
	${graphene_CONFIG_INCLUDE_DIR}
)

target_link_libraries(clusti 
	${EXPAT_LIBRARIES}
	${graphene_LIBRARY} 
)


# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# create executable


add_executable(FrustumToEquirect
    ${APP_SOURCES} 
)

# properties
set_target_properties(FrustumToEquirect PROPERTIES
    CXX_STANDARD 20
    C_STANDARD_REQUIRED YES
)





# include directories
#target_include_directories(FrustumToEquirect PRIVATE  ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
# SDL2_INCLUDE_DIR, without 'S'
target_include_directories(FrustumToEquirect PRIVATE 
	${SDL2_INCLUDE_DIR} 
	${GLEW_INCLUDE_DIRS} 
	${OPENGL_INCLUDE_DIRS} 
	${GLM_INCLUDE_DIRS}
	${EXPAT_INCLUDE_DIRS}
	${graphene_INCLUDE_DIR}
	${graphene_CONFIG_INCLUDE_DIR}
    ${clusti_INCLUDE_DIR}
)




# link libraries
target_link_libraries(FrustumToEquirect 
    clusti
	${GLM_LIBRARY_DIRS} 
	${OPENGL_LIBRARIES}
	${EXPAT_LIBRARIES}
	${graphene_LIBRARY} 
)



if(WIN32)
	# link specific win32 libraries
	## SDL makes problems...
	#target_link_libraries(FrustumToEquirect GLEW::GLEW SDL2::SDL2)
	target_link_libraries(FrustumToEquirect GLEW::glew ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY} )
	
else()
	# link linux libraries
	target_link_libraries(FrustumToEquirect ${GLEW_LIBRARIES} ${SDL2_LIBRARIES})
endif()

if (NOT MSVC)
	target_compile_options(FrustumToEquirect PRIVATE -Wno-narrowing)
endif()
# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------





####-------------
if(WIN32)

	# awful hacks to get the DLLs found by executable: just copy them to the location where the exe will be run in by visual studio...

	get_filename_component(GLEW_SHARED_LIBRARIES_directory ${GLEW_SHARED_LIBRARIES} DIRECTORY)
	add_custom_command(TARGET FrustumToEquirect POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${GLEW_SHARED_LIBRARIES_directory}/../../../bin/Release/x64/glew32.dll ${FrustumToEquirect_BINARY_DIR}/. )

	get_filename_component(SDL_SHARED_LIBRARIES_directory  ${SDL2MAIN_LIBRARY} DIRECTORY)
	add_custom_command(TARGET FrustumToEquirect POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SDL_SHARED_LIBRARIES_directory}/SDL2.dll ${FrustumToEquirect_BINARY_DIR}/. )

	set(graphene_SHARED_LIBRARY "${root_dir}/bin/graphene-1.0-0.dll")
	add_custom_command(TARGET FrustumToEquirect POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${graphene_SHARED_LIBRARY} ${FrustumToEquirect_BINARY_DIR}/. )
	
	add_custom_command(TARGET FrustumToEquirect POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${EXPAT_INSTALL_DIR}/bin/libexpat.dll ${FrustumToEquirect_BINARY_DIR}/. )
	
endif()

####-------------



