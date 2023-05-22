#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

namespace Nui::Tests
{
    class TestAttributes : public CommonTestFixture
    {};

    TEST_F(TestAttributes, AttributeIsSetOnElement)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        // clang-format off
		render(div{
			class_ = "asdf"
		}());
        // clang-format on

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }
}