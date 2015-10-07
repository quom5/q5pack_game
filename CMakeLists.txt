cmake_minimum_required(VERSION 2.4)

PROJECT(q5pack_game)

ADD_DEFINITIONS(-ggdb)

find_package(Entc)
find_package(Quom5)

SUBDIRS(modules/q5game_server)
SUBDIRS(modules/q5game_engine)
