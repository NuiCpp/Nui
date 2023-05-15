#include <gtest/gtest.h>

#include "engine/global_object.hpp"

#include <nui/frontend/elements/div.hpp>

namespace Nui::Tests
{
    class TestRender : public ::testing::Test
    {
      private:
        void SetUp() override
        {
            Nui::Tests::Engine::installDocument();
        }
    };

    TEST_F(TestRender, CanRenderBasicDiv)
    {
        using Nui::Elements::div;

        div{}();
    }
}