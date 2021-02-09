cmake_minimum_required(VERSION 3.1)

# -----------------------------------------------------------------------------
project(prosense-obs-stitcher)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# this file doesn't seem to do anything ..
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/prosense-obs-stitcher-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/prosense-obs-stitcher/config/prosense-obs-stitcher-config.h")

set(prosense-obs-stitcher_config_HEADERS
	"${CMAKE_BINARY_DIR}/plugins/prosense-obs-stitcher/config/prosense-obs-stitcher-config.h")
# -----------------------------------------------------------------------------



# Doesn't seem to be used ...
#include_directories(${CMAKE_SOURCE_DIR}/deps/libcaption)


# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# Find external libraries


# -----------------------------------------------------------------------------
# Expat: pure C XML library

#TODO trigger build (and install or local linking) from plugin build
set(EXPAT_INSTALL_DIR "C:/Program Files/expat/" CACHE STRING "Install directory of expat xml C library")
set (CMAKE_PREFIX_PATH 
	"${EXPAT_INSTALL_DIR}"
)

find_package(EXPAT REQUIRED)



# -----------------------------------------------------------------------------
# Graphene: pure C linear Algebra alternative to GLM

# Dirty hack, after failing to integrate graphene's meson build into CMake'
# "ExternalProject_Add" workflow: 
# Because meson build needs the visual studio command prompt to detect the
# MSVC compiler, it must be invoked from  cmake somehow. Research gave
# some very ugly hardcode- results like 
# https://www.codeproject.com/Questions/706617/How-to-open-Visual-studio-command-Prompt-using-Win
# rendering any kind of partability useless. So I will just link to the
# headers and libs of the graphene install, which is expected to be 
# built & installed "by hand" via meson before. 
# See https://github.com/ebassi/graphene for how to do this.

#TODO maybe outsource as FindGraphene.cmake...

if(WIN32)
	set(my_root_dir "C:")
else()
	set(my_root_dir "")
endif()


SET(graphene_SEARCH_PATHS
	"${my_root_dir}/bin"
	"${my_root_dir}/lib"
	"${my_root_dir}/include"
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
# clusti (ClusterStitching) library:
# own little helper lib, intended to be useable also with other
# video software. Heavy work in progress, as interplay with OBS
# will very likely change a lot. It is just an attempt to decouple
# the calibration/configuration and math from the renderer.

# sources:

# share some code with OBS plugin. This project here 
# is the playground for non-OBS-related development, after all
set(clusti_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/clusti/include")
set(clusti_SOURCES_DIR "${PROJECT_SOURCE_DIR}/deps/clusti/src")

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
# -------------------------------------

# create library:

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
# create OBS plugin library

set(prosense-obs-stitcher_SOURCES
	prosense-obs-stitcher.c)

add_library(prosense-obs-stitcher MODULE
	${prosense-obs-stitcher_SOURCES}
	${prosense-obs-stitcher_config_HEADERS})


set_target_properties(prosense-obs-stitcher PROPERTIES
    C_STANDARD 11
)
# test port from C to C++20; successful, yet
# prefer sticking to C version now, in order to not
# get subtle compiler related issues later.
# Upgrading to C++ is easier than downgrading to C
#set_target_properties(prosense-obs-stitcher PROPERTIES
#    CXX_STANDARD 20
#    CXX_STANDARD_REQUIRED YES
#)


target_include_directories(prosense-obs-stitcher PRIVATE
	${clusti_INCLUDE_DIR}
	${EXPAT_INCLUDE_DIRS}
	${graphene_INCLUDE_DIR}
	${graphene_CONFIG_INCLUDE_DIR}
)

target_link_libraries(prosense-obs-stitcher
	libobs
	clusti
	${graphene_LIBRARY} 
	${EXPAT_LIBRARIES}
)
# -----------------------------------------------------------------------------




# -----------------------------------------------------------------------------
# "Post processing": IDE config, file copy ...

# tell IDE where to show the plugin files in the IDE's project tree
set_target_properties(prosense-obs-stitcher PROPERTIES FOLDER "plugins")


# install plugin's binaries, shader files etc. to the respective folders
install_obs_plugin_with_data(prosense-obs-stitcher data)


# -------------------------------------
if(WIN32)

	# Awful hacks to copy the plugin's dependency-DLLs
	# so that they are found by the OBS executable
	# TODO find a more elegant way

	set(expat_SHARED_LIBRARY "${EXPAT_INSTALL_DIR}/bin/libexpat.dll")
	set(graphene_SHARED_LIBRARY "${my_root_dir}/bin/graphene-1.0-0.dll")


	# copy DLLs to development run directories
	obs_helper_copy_dir(prosense-obs-stitcher ALL
		"${expat_SHARED_LIBRARY}"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/${OBS_EXECUTABLE_DESTINATION}/.")
	obs_helper_copy_dir(prosense-obs-stitcher ALL
		"${graphene_SHARED_LIBRARY}"
		"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/${OBS_EXECUTABLE_DESTINATION}/.")

	# copy DLLs to intermediate directory, which in turn
	# will be installed appropriately when the install target is executed:
	set(addiInstallDir "${CMAKE_SOURCE_DIR}/additional_install_files")
	if(DEFINED ENV{obsAdditionalInstallFiles})
		set(addfdir "$ENV{obsAdditionalInstallFiles}")
	endif()
	# only do 64 bit, don't see any need for 32 bit
	obs_helper_copy_dir(prosense-obs-stitcher ALL
		"${expat_SHARED_LIBRARY}"
		"${addiInstallDir}/exec64/.")
	obs_helper_copy_dir(prosense-obs-stitcher ALL
		"${graphene_SHARED_LIBRARY}"
		"${addiInstallDir}/exec64/.")

endif()
# -------------------------------------



