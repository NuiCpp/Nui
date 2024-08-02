#pragma once

#include <gtest/gtest.h>

#include "../common_test_fixture.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/components/select.hpp>
#include <nui/frontend/dom/element.hpp>

#include <vector>
#include <string>

namespace Nui::Tests
{
    using namespace Engine;

    class TestSelect : public CommonTestFixture
    {
      protected:
        Observed<std::vector<Components::SelectOptions<int>>> model_;
    };

    TEST_F(TestSelect, OptionsAreRendered)
    {
        model_.value().emplace_back(Components::SelectOptions<int>{.label = "foo", .value = 7});
        model_.value().emplace_back(Components::SelectOptions<int>{.label = "bar", .value = 8});

        auto select = Components::Select<int>({
            .model = model_,
        });

        render(select);

        EXPECT_EQ(Nui::val::global("document")["body"]["tagName"].as<std::string>(), "select");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "option");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "foo");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["value"].as<long long>(), 7);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][1]["tagName"].as<std::string>(), "option");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][1]["textContent"].as<std::string>(), "bar");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][1]["attributes"]["value"].as<long long>(), 8);
    }

    TEST_F(TestSelect, PreselectionIsAsExpected)
    {
        model_.value().emplace_back(Components::SelectOptions<int>{.label = "foo", .value = 7});
        model_.value().emplace_back(Components::SelectOptions<int>{.label = "bar", .value = 8});

        auto select = Components::Select<int>({.model = model_, .preSelectedIndex = 1});

        render(select);

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 2);
        EXPECT_FALSE(Nui::val::global("document")["body"]["children"][0]["attributes"].hasOwnProperty("selected"));
        EXPECT_TRUE(Nui::val::global("document")["body"]["children"][1]["attributes"].hasOwnProperty("selected"));
    }

    TEST_F(TestSelect, SelectAttributesAreForwarded)
    {
        using namespace Nui::Attributes;

        auto select = Components::Select<int>({
            .model = model_,
            .selectAttributes =
                {
                    class_ = "test-class",
                    id = "test-id",
                },
        });

        render(select);

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "test-class");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "test-id");
    }

    TEST_F(TestSelect, OnSelectIsSet)
    {
        using namespace Nui::Attributes;

        model_.value().emplace_back(Components::SelectOptions<int>{.label = "foo", .value = 7});

        bool called = false;
        auto select = Components::Select<int>({
            .model = model_,
            .onSelect =
                [&called](long long index, Components::SelectOptions<int> const& opt) {
                    called = true;
                    EXPECT_EQ(index, 0);
                    EXPECT_EQ(opt.label, "foo");
                    EXPECT_EQ(opt.value, 7);
                },
        });

        render(select);

        Nui::val event = Nui::val::object();
        event.set("target", Nui::val::global("document")["body"]);
        event["target"].set("selectedIndex", 0);

        Nui::val::global("document")["body"]["onchange"](event);

        EXPECT_TRUE(called);
    }
}