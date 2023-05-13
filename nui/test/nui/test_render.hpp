#include <gtest/gtest.h>

#include <nui/frontend/elements/div.hpp>

namespace Nui::Tests
{
    class TestRender : public ::testing::Test
    {
      private:
    };

    TEST_F(TestRender, CanRenderBasicDiv)
    {
        using Nui::Elements::div;

        div{}();
    }
}