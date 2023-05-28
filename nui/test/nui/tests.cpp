#include "test_attributes.hpp"
#include "test_ranges.hpp"
#include "test_render.hpp"
#include "test_switch.hpp"
#include "components/test_table.hpp"
#include "components/test_dialog.hpp"
#include "components/test_select.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}