#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/dom/reference.hpp>

namespace Nui::Tests
{
    class TestRender : public CommonTestFixture
    {};

    TEST_F(TestRender, CanRenderBasicDiv)
    {
        using Nui::Elements::div;

        render(div{}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["tagName"].as<std::string>(), "div");
    }

    TEST_F(TestRender, CanRenderSpan)
    {
        using Nui::Elements::span;

        render(span{}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["tagName"].as<std::string>(), "span");
    }

    TEST_F(TestRender, SingleChildIsRendered)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;

        render(div{}(span{}()));

        ASSERT_EQ(emscripten::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(emscripten::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
    }

    TEST_F(TestRender, MultipleChildrenAreRendered)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        render(div{}(span{}(), div{}()));
        ASSERT_EQ(emscripten::val::global("document")["body"]["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(emscripten::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(emscripten::val::global("document")["body"]["children"][1]["tagName"].as<std::string>(), "div");
    }

    TEST_F(TestRender, CanRenderText)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;
        render(div{}(span{}("Hello World")));
        ASSERT_EQ(emscripten::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "Hello World");
    }

    TEST_F(TestRender, CanGetReferenceToElement)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using Nui::Attributes::id;
        using Nui::Attributes::reference;

        std::weak_ptr<Dom::BasicElement> ref;

        render(body{id = "A", reference = ref}("Hello World"));

        ASSERT_FALSE(ref.expired());
        EXPECT_EQ(ref.lock()->val()["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestRender, CanGetReferenceToElementUsingFunction)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using Nui::Attributes::id;
        using Nui::Attributes::reference;

        std::weak_ptr<Dom::BasicElement> ref;

        render(body{id = "A", reference = [&](auto&& weak) {
                        ref = std::move(weak);
                    }}("Hello World"));

        ASSERT_FALSE(ref.expired());
        EXPECT_EQ(ref.lock()->val()["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestRender, CanGetReferenceToElementWithChildren)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using Nui::Attributes::id;
        using Nui::Attributes::reference;

        std::weak_ptr<Dom::BasicElement> ref;

        render(body{id = "A", reference = ref}(div{id = "B"}()));

        ASSERT_FALSE(ref.expired());
        EXPECT_EQ(ref.lock()->val()["attributes"]["id"].as<std::string>(), "A");
    }
}