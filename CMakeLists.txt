CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(CongestionProxy)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_MAINTAINER "Kennny")
SET(CPACK_PACKAGE_NAME "congestionproxy")
SET(CPACK_PACKAGE_VERSION "1.0-dev")
INCLUDE(CPack)

IF(UNIX)
	ADD_DEFINITIONS(-std=c++11 -g)
ENDIF()

FILE(GLOB_RECURSE srcfiles src/*.h src/*.cpp src/*.c)

ADD_EXECUTABLE(congestion-proxy ${srcfiles})

INSTALL(TARGETS congestion-proxy DESTINATION /usr/sbin)
INSTALL(FILES etc/congestion-proxy.conf.dist.in DESTINATION /etc/congestion-proxy.conf)
INSTALL(FILES etc/congestion-proxy.service DESTINATION /etc/systemd/system/)
INSTALL(DIRECTORY DESTINATION /var/lib/congestion-proxy)
INSTALL(CODE "execute_process(COMMAND systemctl enable /etc/systemd/system/congestion-proxy.service)")

IF(UNIX)
	TARGET_LINK_LIBRARIES(congestion-proxy pthread)
ENDIF()

IF(WIN32)
	TARGET_LINK_LIBRARIES(congestion-proxy Ws2_32)
ENDIF()
