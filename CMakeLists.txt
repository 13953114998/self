cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0048 NEW)
project(base_master VERSION 0.1.0.0)

# options
option(DEBUG "enable debug" OFF)
if (DEBUG STREQUAL "ON")
	message("-- defined debug")
endif()


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -g -Wno-unused-but-set-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-unused-function -Wno-unused-variable -lpthread -lm -lsqlite3 -ldl -lreadline -lncurses -luci -lcurl")
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
save2sqlite.c
epoll_active.c
hash_list_active.c
action/http.c
action/cJSON.c
action/timer.c
action/my_log.c
action/mosquit_sub.c
udp_wifi.c
cli_action.c
baicell_net.c
imsi_list_acton.c
udp_send_imsi.c
gpio_serial.h
udp_gsm.c
tcp_wcdma.c
scan_fre.c
fiberhome_lte.c
)

set (UDPRECV
arg.c
recev_imsi_udp.c
action/cJSON.c
)

option(VMWORK "in vmwork test" OFF)
if(VMWORK STREQUAL "ON")
	message("-- defined VMWORK")
endif()

option(SYS86 "work in x86 device " OFF)
if (SYS86 STREQUAL "ON")
	message("-- work in x86 device")
endif()

option(SCACHE "save imsi info to cache file" OFF)
if (SCACHE STREQUAL "ON")
	message("-- save imsi info to cache file")
endif()

option(SAVESQL "save imsi infor to sqlite db" OFF)
if (SAVESQL STREQUAL "ON")
	message("-- save imsi infor to sqlite db")
endif()

option (BIG_END "work in big-endian device(eg:mips(not mipsel),ARM) " OFF)
if (BIG_END STREQUAL "ON")
	message("-- work in big-Endian device")
endif()
if (BIG_END STREQUAL "OFF")
	message("-- work in Little-Endian device")
endif()

option(AUTH "run with auth" ON)
if (AUTH STREQUAL "ON")
	message("-- deive need auth and set MD5POS (CPUID, MAC or SATASN)")
set(SMASTER ${SMASTER}
auth.c
md5.c
)
endif()
option(MD5POS "md5sum position" OFF)

if ((SYS86 STREQUAL "ON") AND (VMWORK STREQUAL "OFF"))
set(SMASTER ${SMASTER}
action/serialtest_sh.c
action/serialtest_ln.c
action/gpio_ctrl.c
gpio_serial.c
)
endif()

if ((SCACHE STREQUAL "ON"))
set(SMASTER ${SMASTER}
cache_file.c
)
endif()

# extra libs
find_package(Threads)
set (EXTRALIB ${EXTRALIB} ${CMAKE_THREAD_LIBS_INIT})

find_package(CURL)
set (EXTRALIB ${EXTRALIB} ${CURL_LIBRARIES})

option(CLI "use cli manage base band device" ON)
if (CLI STREQUAL "ON")
	message("-- defined CLI")
set(SMASTER ${SMASTER}
cli_action.c
)
set (CLISRC
arg.c
cli.c
action/cJSON.c
lua_action.c
)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -llua")
add_executable(cli ${CMDLINE} ${CLISRC})
target_link_libraries(cli ${EXTRALIB} m sqlite3 readline lua)
install(TARGETS cli DESTINATION bin)
endif()

# target
add_executable(base_master ${CMDLINE} ${SMASTER})
target_link_libraries(base_master ${EXTRALIB} mosquitto m sqlite3 lua uci)
add_executable(recev_imsi_udp ${CMDLINE} ${UDPRECV})
target_link_libraries(recev_imsi_udp ${EXTRALIB} m)

# install
install(TARGETS base_master DESTINATION bin)
install(TARGETS recev_imsi_udp DESTINATION bin)
