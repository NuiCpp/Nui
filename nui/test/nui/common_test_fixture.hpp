#pragma once

#include <gtest/gtest.h>

#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/dom/dom.hpp>

namespace Nui::Tests
{
    class CommonTestFixture : public ::testing::Test
    {
      protected:
        CommonTestFixture()
            : preConstructionHelper_{[]() {
                Engine::resetGlobals();
                return 0;
            }()}
            , document_{}
            , dom_{}
        {}

        template <typename T>
        void render(T&& factory)
        {
            dom_.setBody(std::forward<T>(factory));
        }

        int preConstructionHelper_;
        Engine::Document document_;
        Dom::Dom dom_;
    };
}