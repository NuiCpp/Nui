#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"
#include "engine/object.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

namespace Nui::Tests
{
    using namespace Engine;
    using namespace std::string_literals;

    class TestSwitch : public CommonTestFixture
    {
      protected:
        Nui::Observed<std::string> urlFragment_;
    };

    TEST_F(TestSwitch, SwitchSwitchesContent)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div; // because of the global div.
        using Nui::Elements::default_;
        using Nui::Elements::span;

        // Nui::listenToFragmentChanges(impl_->fragment);

        // clang-format off
        render(div{}(
            switch_(urlFragment_)(
                default_()(
                    span{}("Default")
                ),
                case_("")(
                    span{}("Empty")
                ),
                case_("about")(
                    span{}("About")
                )
            )
        ));
        // clang-format on

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "Empty");

        urlFragment_ = "about"s;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "About");

        urlFragment_ = "something-else"s;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "Default");
    }
}