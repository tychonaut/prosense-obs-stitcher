project(prosense-obs-stitcher)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/prosense-obs-stitcher-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/prosense-obs-stitcher/config/prosense-obs-stitcher-config.h")

set(prosense-obs-stitcher_config_HEADERS
	"${CMAKE_BINARY_DIR}/plugins/prosense-obs-stitcher/config/prosense-obs-stitcher-config.h")

include_directories(${CMAKE_SOURCE_DIR}/deps/libcaption)



#TODO trigger build (and install or local linking) from plugin build
set(MY_EXPAT_DIR "C:/Program Files/expat/" CACHE STRING "Install directory of expat xml C library")
set (CMAKE_PREFIX_PATH 
	"${MY_EXPAT_DIR}"
)


#pure-C xml library
find_package(EXPAT REQUIRED)




set(prosense-obs-stitcher_SOURCES
	prosense-obs-stitcher.c)

add_library(prosense-obs-stitcher MODULE
	${prosense-obs-stitcher_SOURCES}
	${prosense-obs-stitcher_config_HEADERS})


# test port from C to C++20; successful, yet
# prefer sticking to C version now, in order to not
# get subtle compiler related issues later.
# Upgrading to C++ is easier than downgrading to C
#set_target_properties(prosense-obs-stitcher PROPERTIES
#    CXX_STANDARD 20
#    CXX_STANDARD_REQUIRED YES
#)


target_include_directories(prosense-obs-stitcher PRIVATE
	${EXPAT_INCLUDE_DIRS}
)

target_link_libraries(prosense-obs-stitcher
	libobs
	${EXPAT_LIBRARIES}
)


set_target_properties(prosense-obs-stitcher PROPERTIES FOLDER "plugins")


install_obs_plugin_with_data(prosense-obs-stitcher data)
