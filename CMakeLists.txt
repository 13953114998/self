cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0048 NEW)
project(base_master VERSION 0.1.0.0)

# options
option(DEBUG "enable debug" OFF)
if (DEBUG STREQUAL "ON")
	message("-- defined debug")
endif()


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -g -Wno-unused-but-set-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-unused-function -Wno-unused-variable -lpthread -lm -ldl -lreadline -lncurses")
# include aux cmake scripts
include("cmake/Gengetopt.cmake")
ADD_GENGETOPT_FILES(CMDLINE cmdline.ggo)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
	)

include_directories("${CMAKE_CURRENT_BINARY_DIR}"
	" /usr/local/include/"
	"${PROJECT_BINARY_DIR}"
	"${PROJECT_BINARY_DIR}/include/"
	)

set(SMASTER
main.c
arg.c
bt_model_action.c
epoll_active.c
action/timer.c
action/my_log.c
action/cJSON.c
baicell_net.c
imsi_list_acton.c
tcp_devid.c
)

option(VMWORK "in vmwork test" OFF)
if(VMWORK STREQUAL "ON")
	message("-- defined VMWORK")
endif()

option (BIG_END "work in big-endian device(eg:mips(not mipsel),ARM) " OFF)
if (BIG_END STREQUAL "ON")
	message("-- work in big-Endian device")
endif()
if (BIG_END STREQUAL "OFF")
	message("-- work in Little-Endian device")
endif()

# extra libs
find_package(Threads)
set (EXTRALIB ${EXTRALIB} ${CMAKE_THREAD_LIBS_INIT})

# target
add_executable(base_master ${CMDLINE} ${SMASTER})
target_link_libraries(base_master ${EXTRALIB} m)

# install
install(TARGETS base_master DESTINATION bin)
