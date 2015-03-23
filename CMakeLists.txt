
PROJECT(CSim)
cmake_minimum_required(VERSION 2.8)

set(CSIM_DEPENDENCY_DIR "${CMAKE_SOURCE_DIR}/deps" CACHE path "Dependency directory")

# By default, do not warn when built on machines using only VS Express.
# and need to let people know to install Microsoft Visual C++ 2010 Redistributable Package
# before running, if required...(http://www.microsoft.com/en-us/download/details.aspx?id=5555)
IF(NOT DEFINED CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)
    SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
ENDIF()
include(InstallRequiredSystemLibraries)

# Use the following variables when configuring the build from the command line to
# set the corresponding cache variables.
# TESTS ==> CSIM_TESTS
set( CSIM_TESTS OFF CACHE BOOL "Enable build of automated CSim tests." )
if( TESTS )
  set( CSIM_TESTS "${TESTS}" CACHE BOOL "Enable build of automated CSim tests." FORCE )
endif()

# FIXME: should do the above for all options.

# Options
#OPTION(DEBUG
#  "Build this project with debugging turned on (default)"
#  ON)

STRING( TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM )

# Set version information in a config.h file
set(CSim_VERSION_MAJOR "0")
set(CSim_VERSION_MINOR "5")
set(CSim_VERSION_PATCH "0-alpha")
configure_file(
  "${PROJECT_SOURCE_DIR}/src/csim-config.h.in"
  "${PROJECT_BINARY_DIR}/csim-config.h"
  ESCAPE_QUOTES
  )
include_directories("${PROJECT_BINARY_DIR}")


# Add in the directory with the FindCellML module
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CSim_SOURCE_DIR} ${LLVM_DIR})

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    #SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
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

set(PLATFORM_LIBS "")

##
## Set up LLVM/Clang
##

# http://lists.cs.uiuc.edu/pipermail/llvmdev/2011-December/046505.html
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

# Since LLVM 3.5 the cmake files are available in the releases, so consolidate on the "cmake" method now.
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES
    instrumentation
    mcjit
    interpreter
    x86codegen
    asmparser
    bitwriter
    ipo
    option
    irreader
    linker
    runtimedyld
    executionengine
    asmprinter
    selectiondag
    objcarcopts
    x86desc
    mcparser
    codegen
    x86info
    bitreader
    scalaropts
    x86utils
    instcombine
    transformutils
    ipa
    analysis
    target
    vectorize
    core
    mc
    object
    support
)

#    if( ${OPERATING_SYSTEM} STREQUAL "linux" )
#        SET(PLATFORM_LIBS ${PLATFORM_LIBS} "dl" "pthread" "tinfo")
#    endif()

add_definitions(${LLVM_DEFINITIONS})
include_directories( ${LLVM_INCLUDE_DIRS} )

#message("LLVM defs: ${LLVM_DEFINITIONS}")
#message("LLVM includes: ${LLVM_INCLUDE_DIRS}")
#message("LLVM lib dir: ${LLVM_LIBRARY_DIRS}")
#message("LLVM libs: ${REQ_LLVM_LIBRARIES}")

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
    clangEdit
    clangLex
)

link_directories(${LLVM_LIBRARY_DIRS})

# Set compiler flags
if (WIN32)
    # need to treat all C files as C++ to avoid problems (http://stackoverflow.com/questions/2706453/what-is-the-cause-of-these-visual-studio-2010-errors-warnings)
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4290 /wd4091")
    add_definitions(-DWIN32 -D_CRT_SECURE_NO_WARNINGS)
else(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    add_definitions("-Wall" "-Werror")
    # TODO: need a way to determine when lapack and blas are required.
    #set(PLATFORM_LIBS ${PLATFORM_LIBS} "lapack" "blas" "m")
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "m")
endif(WIN32)

if( ${OPERATING_SYSTEM} STREQUAL "darwin" )
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "termcap" "pcrecpp" "pcre")
endif( ${OPERATING_SYSTEM} STREQUAL "darwin" )

if( ${OPERATING_SYSTEM} STREQUAL "linux" )
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "pcrecpp" "pcre")
endif( ${OPERATING_SYSTEM} STREQUAL "linux" )

if( ${OPERATING_SYSTEM} STREQUAL "windows" )
    FIND_PATH(PCRECPP_INCLUDE_DIR pcrecpp.h
        ${CSIM_DEPENDENCY_DIR}/include
    )
    FIND_LIBRARY(PCRE_LIBRARY pcre
        ${CSIM_DEPENDENCY_DIR}/lib
    )
    FIND_LIBRARY(PCRECPP_LIBRARY pcrecpp
        ${CSIM_DEPENDENCY_DIR}/lib
    )
    INCLUDE_DIRECTORIES(${PCRECPP_INCLUDE_DIR})
    set(PLATFORM_LIBS ${PLATFORM_LIBS} ${PCRECPP_LIBRARY} ${PCRE_LIBRARY})
endif( ${OPERATING_SYSTEM} STREQUAL "windows" )

ADD_DEFINITIONS(
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
  src/timer.c
  src/cellml.cpp
  src/flatten-model.cpp
  src/cellml-utils.cpp
  src/CellmlCode.cpp
  src/integrator.cpp
  src/ccgs_required_functions.cpp
  src/simulation.c
  src/xpath.cpp
  src/xmldoc.cpp
  src/outputVariables.cpp
  src/ModelCompiler.cpp
  src/ExecutableModel.cpp
  src/csim.cpp
)

SET(libCSim_SRCS
  src/utils.c
  src/timer.c
  src/cellml.cpp
  src/flatten-model.cpp
  src/cellml-utils.cpp
  src/CellmlCode.cpp
  src/integrator.cpp
  src/ccgs_required_functions.cpp
  src/simulation.c
  src/xpath.cpp
  src/xmldoc.cpp
  src/outputVariables.cpp
  src/ModelCompiler.cpp
  src/ExecutableModel.cpp
  src/CellmlSimulator.cpp
)

SET(libCSim_PUBLIC_API
  src/CellmlSimulator.hpp
  ${PROJECT_BINARY_DIR}/csim-config.h
)

# Special treatment for generating and compiling version.c
## assume no perl on windows and simply copy the file over.
## FIXME: need to migrate away from this perl script (and subversion)
if(WIN32)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/version.c
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/version.c.in ${CMAKE_BINARY_DIR}/version.c
      DEPENDS ${CMAKE_SOURCE_DIR}/version.c.in ${CSim_SRCS}
    )
else(WIN32)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_BINARY_DIR}/version.c
      COMMAND ${CMAKE_SOURCE_DIR}/create-version.pl ${CMAKE_SOURCE_DIR}/version.c.in ${CMAKE_BINARY_DIR}/version.c ${CMAKE_SOURCE_DIR} ${CVODES_INCLUDE_DIR}
      DEPENDS ${CMAKE_SOURCE_DIR}/create-version.pl ${CMAKE_SOURCE_DIR}/version.c.in ${CSim_SRCS}
    )
endif(WIN32)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/version.c
  PROPERTIES COMPILE_FLAGS -I${CMAKE_SOURCE_DIR}/src
)

# setting up the rpath options (http://www.cmake.org/Wiki/CMake_RPATH_handling)
### Must come before setting up the executable??!!

# use, i.e. don't skip the full RPATH for the build tree
#SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

#SET(CMAKE_INSTALL_RPATH "\$ORIGIN/")


# http://stackoverflow.com/questions/17164731/installing-only-one-target-and-its-dependencies-out-of-a-complex-project-with
# Remove dependency of "install" target to "all" target (once, in the main CMakeLists.txt):
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
# Set to OPTIONAL installation of all targets you do not want to always build:
# install(TARGETS <<targetname>> DESTINATION . OPTIONAL)


# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH false)

# For Apple set the icns file containing icons
if(APPLE)
  # set how it shows up in the Info.plist file
  set(MACOSX_BUNDLE_ICON_FILE QtTest.icns) 
  # set where in the bundle to put the icns file
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/QtTest.icns
      PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns file in the target
  set(CSim_SRCS ${CSim_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/QtTest.icns)
ENDIF(APPLE)

set(EXECUTABLE_NAME "csim")
ADD_EXECUTABLE(${EXECUTABLE_NAME} MACOSX_BUNDLE ${CSim_SRCS} ${CMAKE_BINARY_DIR}/version.c)
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME}
  ${REQ_CLANG_LIBRARIES}
  ${REQ_LLVM_LIBRARIES}
  ${CELLML_LIBRARIES}
  ${CCGS_LIBRARIES}
  ${CVODES_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${PLATFORM_LIBS}
)
# need this to get things working on Linux?
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    INSTALL_RPATH "\$ORIGIN"
)

# the library version of CSim
set(CSIM_LIBRARY_NAME "CSimLibrary")
add_library(${CSIM_LIBRARY_NAME} SHARED ${libCSim_SRCS})
TARGET_LINK_LIBRARIES(${CSIM_LIBRARY_NAME}
  ${CELLML_LIBRARIES}
  ${CCGS_LIBRARIES}
  ${REQ_CLANG_LIBRARIES}
  ${REQ_LLVM_LIBRARIES}
  ${CVODES_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${PLATFORM_LIBS}
)
set_target_properties(${CSIM_LIBRARY_NAME} PROPERTIES
    OUTPUT_NAME csim
    VERSION "${CSim_VERSION_MAJOR}.${CSim_VERSION_MINOR}.${CSim_VERSION_PATCH}"
)

OPTION(BUILD_STATIC_LIB "Build Static CSIM library" ON)
if (BUILD_STATIC_LIB)
	add_library(csim-static STATIC ${libCSim_SRCS})
	set_target_properties(csim-static PROPERTIES
    OUTPUT_NAME csim-static
    VERSION "${CSim_VERSION_MAJOR}.${CSim_VERSION_MINOR}.${CSim_VERSION_PATCH}"
)
endif()

# Install the built application and required libraries on the various platforms

set(library_dest_dir bin)
## Need to escape the CMAKE_INSTALL_PREFIX so that it is resolved appropriately when packaging
set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/${EXECUTABLE_NAME}")
IF(APPLE)
  SET(library_dest_dir "\${EXECUTABLE_NAME}.app/Contents/MacOS")
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/${EXECUTABLE_NAME}.app")
ENDIF(APPLE)
IF(WIN32)
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/${EXECUTABLE_NAME}.exe")
ENDIF(WIN32)

# Install the application, on Apple, the bundle is at the root of the
# install tree, and on other platforms it'll go into the bin directory.
install(TARGETS ${EXECUTABLE_NAME}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)

# Install the library and public headers
install(TARGETS ${CSIM_LIBRARY_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION lib
)
INSTALL( FILES ${libCSim_PUBLIC_API}
    DESTINATION include
)

# Use BundleUtilities to get all other dependencies for the application to work.
# It takes a bundle or executable along with possible plugins and inspects it
# for dependencies.  If they are not system dependencies, they are copied.

# directories to look for dependencies
function(getSearchPath outDir lib)
    get_filename_component(dir ${lib} PATH)
    message(STATUS "${outDir} dir = ${dir}")
    set(${outDir} ${dir})
    message(STATUS "${outDir} dir = ${dir}")
endfunction(getSearchPath)
list(GET CELLML_LIBRARIES 0 cellmlLib)
get_filename_component(CELLML_SEARCH_DIR ${cellmlLib} PATH)
list(GET CCGS_LIBRARIES 0 ccgsLib)
get_filename_component(CCGS_SEARCH_DIR ${ccgsLib} PATH)
list(GET LIBXML2_LIBRARIES 0 xml2Lib)
get_filename_component(LIBXML2_SEARCH_DIR ${xml2Lib} PATH)
set(LIBXML2_BIN_DIR ${LIBXML2_SEARCH_DIR}/../bin)
set(DIRS 
    ${CELLML_SEARCH_DIR}
    ${CCGS_SEARCH_DIR}
    ${LIBXML2_SEARCH_DIR}
    ${LIBXML2_BIN_DIR}
)
IF(WIN32)
  get_filename_component(PCRE_SEARCH_DIR ${PCRECPP_LIBRARY} PATH)
  set(DIRS ${DIRS} "${PCRE_SEARCH_DIR}/../bin")
ENDIF(WIN32)
message(STATUS "DIRS = ${DIRS}")

OPTION(BUILD_SBW_MODULE "Build SBW Module" OFF)
if (${BUILD_SBW_MODULE})
	add_subdirectory(SBW)
endif()




# Now the work of copying dependencies into the bundle/package
# The quotes are escaped and variables to use at install time have their $ escaped
# An alternative is the do a configure_file() on a script and use install(SCRIPT  ...).
INSTALL(CODE "
    include(BundleUtilities)
    SET(BU_CHMOD_BUNDLE_ITEMS ON)
    fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")
    " COMPONENT Runtime)

# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well

# License file for the project, used by the STGZ, NSIS, and PackageMaker generators.
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
install(FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION .)
install(FILES ${CPACK_PACKAGE_DESCRIPTION_FILE} DESTINATION .)

set(CPACK_PACKAGE_VERSION_MAJOR "${CSim_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${CSim_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${CSim_VERSION_PATCH}")

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_BINARY_DRAGNDROP ON)
endif()

include(CPack)

if (CSIM_TESTS)
  # enable testing here so that we can make use of the 'test' target
  enable_testing()
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()
