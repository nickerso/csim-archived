# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is CellMLSimulator.
#
# The Initial Developer of the Original Code is
# David Nickerson <nickerso@users.sourceforge.net>.
# Portions created by the Initial Developer are Copyright (C) 2007-2008
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
cmake_minimum_required(VERSION 2.6)

PROJECT(CSim)

# Options
#OPTION(DEBUG
#  "Build this project with debugging turned on (default)"
#  ON)

# Add in the directory with the FindCellML module
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CSim_SOURCE_DIR})

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

# Set compiler flags
SET(CMAKE_C_FLAGS -std=gnu99)
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
  src/cellml_code_manager.c
  src/cellml_methods.c
  src/integrator.c
  src/ccgs_required_functions.cpp
  src/integrator_user_data.c
  src/simulation.c
  src/csim.c
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
  ${CELLML_LIBRARIES}
  ${CCGS_LIBRARIES}
  ${CVODES_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${ZLIB_LIBRARIES}
  -lcurl -ldl -llapack -lblas -lm)

