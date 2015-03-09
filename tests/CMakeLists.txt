# This adds another subdirectory, which has 'project(gtest)'.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gtest-1.7.0)

# Include the gtest library. gtest_SOURCE_DIR is available due to
# 'project(gtest)' above.
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/src/api"  "${PROJECT_BINARY_DIR}/src/api")

add_executable (versionTest
  ${CMAKE_CURRENT_SOURCE_DIR}/versiontest.cpp
  ${HEADER_FILES}
)

# Standard linking to gtest stuff.
target_link_libraries(versionTest gtest gtest_main)

# Extra linking for the project.
#target_link_libraries(libcellmlTest cellml)

# test executable.
add_test(version-test versionTest)

# To work around a bug conditionally set the CXX_STANDARD property
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set_target_properties(libcellmlTest PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
endif()

