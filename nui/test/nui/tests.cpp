#include "test_attributes.hpp"
#include "test_ranges.hpp"
#include "test_render.hpp"
#include "test_table.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}