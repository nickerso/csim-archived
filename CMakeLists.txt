cmake_minimum_required(VERSION 2.6)

PROJECT(CSim)

# Options
#OPTION(DEBUG
#  "Build this project with debugging turned on (default)"
#  ON)

STRING( TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM )

# Add in the directory with the LLVM cmake modules (for versions of LLVM built with cmake, which is not the 3.1 release)
if( ${OPERATING_SYSTEM} STREQUAL "linux" )
    SET(LLVM_DIR /home/andre/LLVM-build/local/linux-x86_64/share/llvm/cmake)
elseif( ${OPERATING_SYSTEM} STREQUAL "darwin" )
    SET(LLVM_DIR /Users/dnic019/shared-folders/projects/cellml-simulator/LLVM/local/macosx-debug/share/llvm/cmake)
endif()

# Add in the directory with the FindCellML module
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CSim_SOURCE_DIR} ${LLVM_DIR})

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)

# Can use -DCMAKE_INCLUDE_PATH=$HOME/std-libs/physiome/include -DCMAKE_LIBRARY_PATH=$HOME/std-libs/physiome/lib
# with cmake command to add extra default search locations or set equivalent
# ENV variables so the system path specifications get properly converted to cmake
# path lists.
FIND_PACKAGE(CellML REQUIRED QUIET)
FIND_PACKAGE(CCGS REQUIRED QUIET)
FIND_PACKAGE(CVODES REQUIRED QUIET)
FIND_PACKAGE(LibXml2 REQUIRED QUIET)
FIND_PACKAGE(ZLIB REQUIRED QUIET)

##
## Set up LLVM/Clang
##

# http://lists.cs.uiuc.edu/pipermail/llvmdev/2011-December/046505.html
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")

# the released versions of LLVM are not built using CMake so need to manually define stuff
set(USE_LLVM_RELEASE TRUE)

set(PLATFORM_LIBS "")

if(USE_LLVM_RELEASE)
    set(LLVM_INSTALL_DIR "/usr/local" CACHE path "The path to the local installed version of Clang+LLVM 3.0")
    message("Using LLVM release installed at: ${LLVM_INSTALL_DIR}")
    SET(LLVM_DEFINITIONS "-D__STDC_LIMIT_MACROS;-D__STDC_CONSTANT_MACROS")
    SET(LLVM_INCLUDE_DIRS "${LLVM_INSTALL_DIR}/include")
    SET(LLVM_LIBRARY_DIRS "${LLVM_INSTALL_DIR}/lib")
    SET(REQ_LLVM_LIBRARIES "LLVMInstrumentation;LLVMJIT;LLVMInterpreter;LLVMX86CodeGen;LLVMAsmParser;LLVMBitWriter;LLVMipo;LLVMLinker;LLVMRuntimeDyld;LLVMExecutionEngine;LLVMAsmPrinter;LLVMSelectionDAG;LLVMX86Desc;LLVMArchive;LLVMMCParser;LLVMCodeGen;LLVMX86AsmPrinter;LLVMX86Info;LLVMBitReader;LLVMScalarOpts;LLVMX86Utils;LLVMInstCombine;LLVMTransformUtils;LLVMipa;LLVMAnalysis;LLVMTarget;LLVMCore;LLVMMC;LLVMObject;LLVMSupport")

    if( ${OPERATING_SYSTEM} STREQUAL "linux" )
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} "dl" "pthread")
    endif()

else()
    # using newer versions of LLVM built with CMake we can use the cmake modules
    find_package(LLVM)

    if( NOT LLVM_FOUND )
        message(FATAL_ERROR "LLVM package can't be found. Set CMAKE_PREFIX_PATH variable to LLVM's installation prefix.")
    endif()

    llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES
        instrumentation
        jit
        interpreter
        nativecodegen
        asmparser
        bitreader
        bitwriter
        codegen
        ipo
        linker
        selectiondag
    )
endif()

add_definitions(${LLVM_DEFINITIONS})
include_directories( ${LLVM_INCLUDE_DIRS} )
link_directories( ${LLVM_LIBRARY_DIRS} )

message("LLVM defs: ${LLVM_DEFINITIONS}")
message("LLVM includes: ${LLVM_INCLUDE_DIRS}")
message("LLVM lib dir: ${LLVM_LIBRARY_DIRS}")
message("LLVM libs: ${REQ_LLVM_LIBRARIES}")

set(REQ_CLANG_LIBRARIES
    clangFrontend
    clangSerialization
    clangDriver
    clangCodeGen
    clangParse
    clangSema
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangAnalysis
    clangRewrite
    clangAST
    clangLex
    clangBasic
    #clangEdit
)

# Set compiler flags
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
ADD_DEFINITIONS(-Wall -Werror
  ${LIBXML2_DEFINITIONS}
)
# Default to debug build type
#SET(CMAKE_BUILD_TYPE Debug)
# Make a new build type
#SET(CMAKE_BUILD_TYPE distribution)
#SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O3")
#SET(CMAKE_C_FLAGS_DISTRIBUTION "-O3")

# Global includes?
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src
  ${CELLML_INCLUDE_DIR}
  ${CCGS_INCLUDE_DIR}
  ${CVODES_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)

# Sources
SET(CSim_SRCS
  src/utils.c
  src/xml-utils.c
  src/curl_file.c
  src/timer.c
  src/cellml.cpp
  src/cellml-utils.cpp
  src/CellmlCode.cpp
  src/cellml_methods.c
  src/integrator.c
  src/ccgs_required_functions.cpp
  #src/integrator_user_data.c
  src/simulation.c
  src/xpath.c
  src/outputVariables.cpp
  src/csim.cpp
)

# Special treatment for generating and compiling version.c
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_BINARY_DIR}/version.c
  COMMAND ${CMAKE_SOURCE_DIR}/create-version.pl ${CMAKE_SOURCE_DIR}/version.c.in ${CMAKE_BINARY_DIR}/version.c ${CMAKE_SOURCE_DIR} ${CVODES_INCLUDE_DIR}
  DEPENDS ${CMAKE_SOURCE_DIR}/create-version.pl ${CMAKE_SOURCE_DIR}/version.c.in ${CSim_SRCS}
)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/version.c
  PROPERTIES COMPILE_FLAGS -I${CMAKE_SOURCE_DIR}/src
)

ADD_EXECUTABLE(csim ${CSim_SRCS} ${CMAKE_BINARY_DIR}/version.c)
TARGET_LINK_LIBRARIES(csim
  ${REQ_CLANG_LIBRARIES}
  ${REQ_LLVM_LIBRARIES}
  ${CELLML_LIBRARIES}
  ${CCGS_LIBRARIES}
  ${CVODES_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${PLATFORM_LIBS}
  -lcurl -ldl -llapack -lblas -lm)

