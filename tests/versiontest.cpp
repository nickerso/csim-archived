#include <iostream>
#include <CellmlSimulator.hpp>

#include "gtest/gtest.h"

TEST(Version, Version) {
    std::string ver = CellmlSimulator::getVersionString();
    EXPECT_EQ("0.5.0-alpha", ver);
}


