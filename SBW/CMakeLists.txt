cmake_minimum_required(VERSION 2.8)
project(cellmlsbw)

find_path(SBW_INCLUDE_DIR SBW/SBW.h )

find_library(SBW_LIBRARY NAMES SBW sbw-static sbw libSBW libsbw)

if(UNIX)
    if(APPLE)
        add_definitions(-DDARWIN)
    endif()
    add_definitions(-DLINUX)
else()
    add_definitions(-DWIN32)
endif()

include_directories(${SBW_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(/tmp/bobby/include)
link_directories(/tmp/bobby/lib)

add_executable(cellmlsbw cellmlsbw.cpp)
target_link_libraries(cellmlsbw ${SBW_LIBRARY} csim)