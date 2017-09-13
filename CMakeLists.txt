CMAKE_MINIMUM_REQUIRED(VERSION 2.0)

PROJECT(CongestionProxy)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_MAINTAINER "Kennny")
SET(CPACK_PACKAGE_NAME "congestionproxy")
SET(CPACK_PACKAGE_VERSION "1.0-dev")
INCLUDE(CPack)

IF(UNIX)
	ADD_DEFINITIONS(--std=c++14 -g)
ENDIF()

## Prepare congestion-proxy project

FILE(GLOB_RECURSE srcfiles src/*.h src/*.cpp src/*.c)

ADD_EXECUTABLE(congestion-proxy ${srcfiles})

INSTALL(TARGETS congestion-proxy DESTINATION /usr/sbin)
INSTALL(FILES etc/congestion-proxy.conf.dist.in DESTINATION /etc/)
INSTALL(FILES etc/congestion-proxy.service DESTINATION /etc/systemd/system/)
INSTALL(CODE "execute_process(COMMAND systemctl enable /etc/systemd/system/congestion-proxy.service)")

IF(UNIX)
	TARGET_LINK_LIBRARIES(congestion-proxy pthread)
ENDIF()

IF(WIN32)
	TARGET_LINK_LIBRARIES(congestion-proxy Ws2_32)
ENDIF()

## Prepare congestion-proxy-ctl project

FILE(GLOB_RECURSE srcfiles_ctl src-ctl/*.h src-ctl/*.cpp src-ctl/*.c)

ADD_EXECUTABLE(congestion-proxy-ctl ${srcfiles_ctl})

INSTALL(TARGETS congestion-proxy-ctl DESTINATION /usr/sbin)

IF(WIN32)
	TARGET_LINK_LIBRARIES(congestion-proxy-ctl Ws2_32)
ENDIF()
