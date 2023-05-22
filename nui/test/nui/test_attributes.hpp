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

    class TestAttributes : public CommonTestFixture
    {};

    TEST_F(TestAttributes, AttributeIsSetOnElement)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        render(div{class_ = "asdf"}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, MultipleAttributesAreSet)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        render(div{class_ = "asdf", id = "qwer"}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"].template as<Object const&>().size(), 2);
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, AttributeIsSetOnElementWithChildren)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        render(div{class_ = "asdf"}(div{}()));

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"].template as<Object const&>().size(), 1);
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, AttributeIsSetOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        render(div{}(div{class_ = "asdf"}()));

        EXPECT_FALSE(emscripten::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]
                .template as<Object const&>()
                .size(),
            1);
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(),
            "asdf");
    }

    TEST_F(TestAttributes, MultipleAttributesAreSetOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        render(div{}(div{class_ = "asdf", id = "qwer"}()));

        EXPECT_FALSE(emscripten::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]
                .template as<Object const&>()
                .size(),
            2);
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(),
            "asdf");
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, CanSetObservedAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, CanSetObservedAttributeOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{}(div{class_ = observedClass}()));

        EXPECT_FALSE(emscripten::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]
                .template as<Object const&>()
                .size(),
            1);
    }

    TEST_F(TestAttributes, ChangingObservedValueChangesTheDomWhenEventsAreProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");

        observedClass = "qwer";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, ChangingObservedValuesDoesNotChangeTheDomWhenEventsAreNotProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        observedClass = "qwer";
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, CanSetMixedAttributes)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass, id = "qwer"}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, ChangeInObservedValueOnlyChangesRelatedAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        Observed<std::string> observedClass{"asdf"};
        Observed<std::string> observedId{"qwer"};

        render(div{class_ = observedClass, id = observedId}());

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
        observedClass = "zxcv";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "zxcv");
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, ChangeInObservedValueOnlyChangesRelatedAttributeInHierarchy)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};
        Observed<std::string> observedClassNested{"qwer"};

        render(div{class_ = observedClass}(div{class_ = observedClassNested}()));

        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(
            emscripten::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(),
            "qwer");
        observedClass = "zxcv";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(emscripten::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "zxcv");
    }
}