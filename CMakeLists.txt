cmake_minimum_required(VERSION 4.0)
project(NanoDB)

set(CMAKE_CXX_STANDARD 20)

add_executable(NanoDB main.cpp
        DBEngine.h
        InMemoryEngine.h
        InMemoryEngine.cpp
        FileEngine.h
        BinaryEngine.h
        ShellInterface.cpp
        ShellInterface.h
        linenoise.c
        linenoise.h)
