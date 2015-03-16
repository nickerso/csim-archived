# This adds another subdirectory, which has 'project(gtest)'.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gtest-1.7.0)

# Include the gtest library. gtest_SOURCE_DIR is available due to
# 'project(gtest)' above.
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/src"  "${PROJECT_BINARY_DIR}")

add_executable (versionTest
  ${CMAKE_CURRENT_SOURCE_DIR}/versiontest.cpp
)

# Standard linking to gtest stuff.
target_link_libraries(versionTest gtest_main ${CSIM_LIBRARY_NAME})

# Extra linking for the project.
#target_link_libraries(libcellmlTest cellml)

# test executable.
add_test(version-test versionTest)
# need to enable the test executable to find all the required libraries?
# FIXME: maybe it is best to "install" the library first and get the rpath all fixed up before trying to run the tests?
# FIXME: obviously this will only work on OS X
set_property(TEST version-test PROPERTY ENVIRONMENT DYLD_LIBRARY_PATH=${DIRS})

# To work around a bug conditionally set the CXX_STANDARD property
#if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
#  set_target_properties(versionTest PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
#endif()

