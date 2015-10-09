cmake_minimum_required(VERSION 2.4)

INCLUDE_DIRECTORIES( ${ENTC_INCLUDE_DIRS} ${Q5_INCLUDE_DIRS} )

SET (GAMESERVER_SOURCES
  q5game_server.c
  entities.h
  entities.c
  realm.h
  realm.c
  player.h
  player.c
)

SET (CMAKE_DISABLE_FIND_PACKAGE_ENET TRUE)
find_package(enet)

IF( ENET_LIBRARY )


ELSE(ENET_LIBRARY)

SUBDIRS(thirdparty/enet)
INCLUDE_DIRECTORIES(thirdparty/enet/include)

ENDIF(ENET_LIBRARY)

ADD_LIBRARY( q5game_server SHARED ${GAMESERVER_SOURCES} )
TARGET_LINK_LIBRARIES( q5game_server ${ENTC_LIBRARIES} ${Q5_LIBRARY_CORE} enet)

SET (CMAKE_DISABLE_FIND_PACKAGE_CURSES TRUE)

find_package(Curses)

IF( CURSES_LIBRARY )

  include_directories(${CURSES_INCLUDE_DIR})

  ADD_EXECUTABLE( q5game_client q5game_client.cpp )
  TARGET_LINK_LIBRARIES( q5game_client ${ENTC_LIBRARIES} ${CURSES_LIBRARIES} enet) 

ENDIF( CURSES_LIBRARY )
