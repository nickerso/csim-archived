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
# - Find CVODES
# Find the CVODES includes and library
# This module defines
#  CVODES_INCLUDE_DIR, where to find CVODES header files.
#  CVODES_LIBRARIES, the libraries needed to use CVODES.
#  CVODES_FOUND, If false, do not try to use CVODES.
# also defined, but not for general use are
#  CVODES_LIBRARY, where to find CVODES.
#  CVODES_NVECTOR_SERIAL_LIBRARY, where to find NVECTOR_SERIAL.

FIND_PATH(CVODES_INCLUDE_DIR cvodes/cvodes.h 
        /usr/include/
        /usr/local/include/
)

FIND_LIBRARY(CVODES_LIBRARY sundials_cvodes
        /usr/lib
        /usr/local/lib
) 

FIND_LIBRARY(CVODES_NVECTOR_SERIAL_LIBRARY sundials_nvecserial
        /usr/lib
        /usr/local/lib
) 

IF (CVODES_INCLUDE_DIR AND CVODES_LIBRARY AND CVODES_NVECTOR_SERIAL_LIBRARY)
   SET(CVODES_LIBRARIES ${CVODES_LIBRARY} ${CVODES_NVECTOR_SERIAL_LIBRARY})
   SET(CVODES_FOUND TRUE)
ENDIF (CVODES_INCLUDE_DIR AND CVODES_LIBRARY AND CVODES_NVECTOR_SERIAL_LIBRARY)


IF (CVODES_FOUND)
   IF (NOT CVODES_FIND_QUIETLY)
      MESSAGE(STATUS "Found CVODES: ${CVODES_LIBRARIES}")
   ENDIF (NOT CVODES_FIND_QUIETLY)
ELSE (CVODES_FOUND)
   IF (CVODES_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find CVODES")
   ELSE (CVODES_FIND_REQUIRED)
      MESSAGE(STATUS "CVODES not found")
   ENDIF (CVODES_FIND_REQUIRED)
ENDIF (CVODES_FOUND)
