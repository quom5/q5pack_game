cmake_minimum_required(VERSION 2.4)

INCLUDE_DIRECTORIES( ${ENTC_INCLUDE_DIRS} ${Q5_INCLUDE_DIRS} )

SET (GAMESERVER_SOURCES
  q5game_server.c
  prot_context.h
  prot_context.c  
  realm.h
  realm.c
  player.h
  player.c
)

ADD_LIBRARY( q5game_server SHARED ${GAMESERVER_SOURCES} )
TARGET_LINK_LIBRARIES( q5game_server ${ENTC_LIBRARIES} ${Q5_LIBRARY_CORE} )

SET (CMAKE_DISABLE_FIND_PACKAGE_CURSES TRUE)

find_package(Curses)

IF( CURSES_LIBRARY )

  include_directories(${CURSES_INCLUDE_DIR})

  ADD_EXECUTABLE( q5game_client q5game_client.cpp )
  TARGET_LINK_LIBRARIES( q5game_client ${ENTC_LIBRARIES} ${CURSES_LIBRARIES}) 

ENDIF( CURSES_LIBRARY )