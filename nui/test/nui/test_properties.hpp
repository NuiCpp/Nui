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

    class TestProperties : public CommonTestFixture
    {};

    TEST_F(TestProperties, CanSetPropertyOnRender)
    {
        using Nui::Elements::div;
        using Nui::Attributes::checked;
        using Nui::property;

        render(div{checked = property("asdf")}());

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<std::string>(), "asdf");
    }

    TEST_F(TestProperties, PropertiesCanBeObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::checked;
        using Nui::property;

        Observed<bool> isChecked{false};

        render(div{checked = property(isChecked)}());

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), false);

        isChecked = true;
        Nui::globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), true);
    }

    TEST_F(TestProperties, PropertiesDoNotChangeIfEventsAreNotProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::checked;
        using Nui::property;

        Observed<bool> isChecked{false};

        render(div{checked = property(isChecked)}());

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), false);

        isChecked = true;

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), false);
    }

    TEST_F(TestProperties, PropertyCanBeOptional)
    {
        using Nui::Elements::div;
        using Nui::Attributes::checked;
        using Nui::property;

        Observed<std::optional<bool>> isChecked{std::nullopt};

        render(div{checked = property(isChecked)}());

        EXPECT_FALSE(Nui::val::global("document")["body"].hasOwnProperty("checked"));

        isChecked = true;
        Nui::globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), true);

        isChecked = std::nullopt;
        Nui::globalEventContext.executeActiveEventsImmediately();

        EXPECT_FALSE(Nui::val::global("document")["body"].hasOwnProperty("checked"));
    }

    TEST_F(TestProperties, PropertiesCanBeGenerated)
    {
        using Nui::Elements::input;
        using Nui::Attributes::value;
        using Nui::property;

        Observed<std::string> observed{"Hello"};

        render(input{value = property(observe(observed).generate([&observed]() {
                         return observed.value() + " World";
                     }))}());

        EXPECT_EQ(Nui::val::global("document")["body"]["value"].as<std::string>(), "Hello World");

        observed = "Goodbye";
        Nui::globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["value"].as<std::string>(), "Goodbye World");
    }

    TEST_F(TestProperties, PropertiesCanBeGeneratedAlternate)
    {
        using Nui::Elements::input;
        using Nui::Attributes::value;
        using Nui::property;

        Observed<std::string> observed{"Hello"};

        render(input{value = observe(observed).generateProperty([&observed]() {
            return observed.value() + " World";
        })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["value"].as<std::string>(), "Hello World");

        observed = "Goodbye";
        Nui::globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["value"].as<std::string>(), "Goodbye World");
    }

    TEST_F(TestProperties, CanUseAttributeLiteralForProperty)
    {
        using Nui::Elements::input;
        using Nui::property;
        using namespace Nui::Attributes::Literals;

        render(input{"value"_attr = property("Hello World")}());

        EXPECT_EQ(Nui::val::global("document")["body"]["value"].as<std::string>(), "Hello World");
    }

    TEST_F(TestProperties, CanUsePropertyLiteral)
    {
        using Nui::Elements::input;
        using Nui::property;
        using namespace Nui::Attributes::Literals;

        render(input{"value"_prop = "Hello World"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["value"].as<std::string>(), "Hello World");
    }

    TEST_F(TestProperties, PropertyCanBeVariant)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        std::variant<std::string, int> idValue{"A"};

        render(div{id = property(idValue)}());
        EXPECT_EQ(Nui::val::global("document")["body"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestProperties, PropertyCanBeObservedVariant)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::variant<std::string, int>> idValue{"A"};

        render(div{id = property(idValue)}());
        EXPECT_EQ(Nui::val::global("document")["body"]["id"].as<std::string>(), "A");

        idValue = 1;
        Nui::globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["id"].as<long long>(), 1);
    }

    TEST_F(TestProperties, PropertyCanBeStringView)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        std::string_view idValue{"A"};

        render(div{id = property(idValue)}());
        EXPECT_EQ(Nui::val::global("document")["body"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestProperties, PropertyCanBeCString)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        char const* idValue{"A"};

        render(div{id = property(idValue)}());
        EXPECT_EQ(Nui::val::global("document")["body"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestProperties, PropertyCanBeFunction)
    {
        using Nui::Elements::div;
        using Nui::Attributes::onClick;

        bool called = false;
        render(div{onClick = property([&called]() {
                       called = true;
                   })}());
        Nui::val::global("document")["body"]["onclick"](Nui::val{});
        EXPECT_TRUE(called);
    }

    TEST_F(TestProperties, PropertyCanBeFunctionTakingVal)
    {
        using Nui::Elements::div;
        using Nui::Attributes::onClick;

        bool called = false;
        render(div{onClick = property([&called](Nui::val) {
                       called = true;
                   })}());
        Nui::val::global("document")["body"]["onclick"](Nui::val{});
        EXPECT_TRUE(called);
    }

    TEST_F(TestProperties, PropertyCanBeBoolean)
    {
        using Nui::Elements::div;
        using Nui::Attributes::hidden;

        bool hiddenValue = true;

        render(div{hidden = property(hiddenValue)}());
        EXPECT_TRUE(Nui::val::global("document")["body"]["hidden"].as<bool>());
    }

    TEST_F(TestProperties, PropertyCanBeIntegral)
    {
        using Nui::Elements::div;
        using Nui::Attributes::tabIndex;

        int tabIndexValue = 1;

        render(div{tabIndex = property(tabIndexValue)}());
        EXPECT_EQ(Nui::val::global("document")["body"]["tabIndex"].as<long long>(), 1);
    }

    TEST_F(TestProperties, PropertyCanBeFloating)
    {
        using Nui::Elements::div;
        using Nui::Attributes::tabIndex;

        double tabIndexValue = 1.5;

        render(div{tabIndex = property(tabIndexValue)}());
        EXPECT_EQ(Nui::val::global("document")["body"]["tabIndex"].as<long double>(), 1.5);
    }
}