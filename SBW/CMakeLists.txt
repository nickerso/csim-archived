cmake_minimum_required(VERSION 2.8)
project(cellmlsbw)

find_path(SBW_INCLUDE_DIR SBW/SBW.h 
	${CSIM_DEPENDENCY_DIR}/include
)

find_library(SBW_LIBRARY NAMES SBW sbw-static sbw libSBW libsbw
	${CSIM_DEPENDENCY_DIR}/lib
)

if(UNIX)
    if(APPLE)
        add_definitions(-DDARWIN)
    endif()
    add_definitions(-DLINUX)
else()
    add_definitions(-DWIN32  -DSBW_EXPORTS)
endif()

include_directories(${SBW_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(/tmp/bobby/include)
link_directories(/tmp/bobby/lib)

add_executable(cellmlsbw cellmlsbw.cpp)

if (BUILD_STATIC_LIB)
  add_dependencies(cellmlsbw csim-static)
  target_link_libraries(cellmlsbw ${SBW_LIBRARY}  csim-static
    ${REQ_CLANG_LIBRARIES}
    ${REQ_LLVM_LIBRARIES}
    ${CELLML_LIBRARIES}
    ${CCGS_LIBRARIES}
    ${CVODES_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PLATFORM_LIBS})
else()
  add_dependencies(cellmlsbw ${CSIM_LIBRARY_NAME})
  target_link_libraries(cellmlsbw ${SBW_LIBRARY}  ${CSIM_LIBRARY_NAME}
    ${REQ_CLANG_LIBRARIES}
    ${REQ_LLVM_LIBRARIES}
    ${CELLML_LIBRARIES}
    ${CCGS_LIBRARIES}
    ${CVODES_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PLATFORM_LIBS})
endif()
install(TARGETS cellmlsbw DESTINATION bin)