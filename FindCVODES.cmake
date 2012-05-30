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
