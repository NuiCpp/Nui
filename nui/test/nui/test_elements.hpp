#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"
#include "engine/object.hpp"

#include <nui/frontend/dom/element.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

namespace Nui::Tests
{
    using namespace Engine;
    using namespace std::string_literals;

    class TestElements : public CommonTestFixture
    {};

    TEST_F(TestElements, CanMakeStandaloneElement)
    {
        using Nui::Elements::div;

        EXPECT_NO_THROW(std::shared_ptr<Nui::Dom::Element> element = Nui::Dom::makeStandaloneElement(div{}("Test")));
    }

    TEST_F(TestElements, StandaloneElementIsOfCorrectType)
    {
        using Nui::Elements::button;

        std::shared_ptr<Nui::Dom::Element> element = Nui::Dom::makeStandaloneElement(button{}("Test"));

        EXPECT_EQ(element->tagName(), "button"s);
    }

    TEST_F(TestElements, StandaloneElementTestContentIsSet)
    {
        using Nui::Elements::div;

        std::shared_ptr<Nui::Dom::Element> element = Nui::Dom::makeStandaloneElement(div{}("Test"));

        EXPECT_EQ(element->val()["textContent"].as<std::string>(), "Test"s);
    }

    TEST_F(TestElements, AttributesOnStandaloneElementAreSet)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        std::shared_ptr<Nui::Dom::Element> element = Nui::Dom::makeStandaloneElement(div{class_ = "hi"}("Test"));

        EXPECT_EQ(element->val()["attributes"]["class"].as<std::string>(), "hi"s);
    }

    TEST_F(TestElements, CanSetDeferredAttributesOnStandaloneElement)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        std::function<void()> deferred;
        std::shared_ptr<Nui::Dom::Element> element = Nui::Dom::makeStandaloneElement(div{!class_ = "hi"}("Test"));

        EXPECT_EQ(element->val()["attributes"]["class"].as<std::string>(), "hi"s);
    }
}