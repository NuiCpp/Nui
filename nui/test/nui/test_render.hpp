#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements.hpp>

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
}