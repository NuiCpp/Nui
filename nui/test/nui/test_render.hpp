#include <gtest/gtest.h>

#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/dom/dom.hpp>

namespace Nui::Tests
{
    class TestRender : public ::testing::Test
    {
      protected:
        template <typename T>
        void render(T&& factory)
        {
            thread_local Dom::Dom dom;
            dom.setBody(std::forward<T>(factory));
        }

        Nui::Tests::Engine::Document document;
    };

    TEST_F(TestRender, CanRenderBasicDiv)
    {
        using Nui::Elements::div;

        render(div{}());
    }
}