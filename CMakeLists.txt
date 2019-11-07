cmake_minimum_required(VERSION 3.15)
project(shellLab)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(shellLab
        globals.h
        helper-routines.cc
        helper-routines.h
        jobs.cc
        jobs.h
        myint.cc
        myspin.cc
        mysplit.cc
        mystop.cc
        tsh.cc)
