#pragma once

#include <gtest/gtest.h>

#include "../common_test_fixture.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/components/dialog.hpp>
#include <nui/frontend/dom/element.hpp>

#include <vector>
#include <string>

namespace Nui::Tests
{
    using namespace Engine;

    class TestDialog : public CommonTestFixture
    {
      protected:
    };

    TEST_F(TestDialog, RenderedDialogHasDialogTagName)
    {
        Nui::Components::DialogController controller{{}};

        render(Dialog(controller));

        EXPECT_EQ(Nui::val::global("document")["body"]["tagName"].as<std::string>(), "dialog");
    }

    TEST_F(TestDialog, RenderedDialogHasCorrectClassName)
    {
        Nui::Components::DialogController controller{{.className = "test-class"}};

        render(Dialog(controller));

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "test-class");
    }

    TEST_F(TestDialog, DialogClassNameIsReactive)
    {
        Nui::Components::DialogController controller{{.className = "test-class"}};
        render(Dialog(controller));

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "test-class");

        controller.setClassName("test-class-2");
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "test-class-2");
    }

    TEST_F(TestDialog, DialogTitleIsRendered)
    {
        Nui::Components::DialogController controller{{.title = "test-title"}};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["textContent"].as<std::string>(),
            "test-title");

        controller.setTitle("test-title-2");
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][0]["textContent"].as<std::string>(),
            "test-title-2");
    }

    TEST_F(TestDialog, DialogBodyIsRendered)
    {
        Nui::Components::DialogController controller{{.body = "test-body"}};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][1]["textContent"].as<std::string>(),
            "test-body");

        controller.setBody("test-body-2");
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][1]["textContent"].as<std::string>(),
            "test-body-2");
    }

    TEST_F(TestDialog, DialogOkButtonIsRendered)
    {
        Nui::Components::DialogController controller{{
            .buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::Ok,
        }};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][0]["textContent"]
                .as<std::string>(),
            "Ok");
    }

    TEST_F(TestDialog, DialogOkButtonIsRenderedWithCorrectClassName)
    {
        Nui::Components::DialogController controller{{
            .buttonClassName = "test-class",
            .buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::Ok,
        }};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][0]["attributes"]["class"]
                .as<std::string>(),
            "test-class");
    }

    TEST_F(TestDialog, DialogYesNoButtonsAreRendered)
    {
        Nui::Components::DialogController controller{{
            .buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::YesNo,
        }};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][0]["textContent"]
                .as<std::string>(),
            "Yes");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][1]["textContent"]
                .as<std::string>(),
            "No");
    }

    TEST_F(TestDialog, DialogYesNoButtonsAreRenderedWithCorrectClassName)
    {
        Nui::Components::DialogController controller{{
            .buttonClassName = "test-class",
            .buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::YesNo,
        }};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][0]["attributes"]["class"]
                .as<std::string>(),
            "test-class");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][1]["attributes"]["class"]
                .as<std::string>(),
            "test-class");
    }

    TEST_F(TestDialog, DialogOkCancelButtonsAreRendered)
    {
        Nui::Components::DialogController controller{
            {.buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::OkCancel}};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][0]["textContent"]
                .as<std::string>(),
            "Ok");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][1]["textContent"]
                .as<std::string>(),
            "Cancel");
    }

    TEST_F(TestDialog, DialogOkCancelButtonsAreRenderedWithCorrectClassName)
    {
        Nui::Components::DialogController controller{{
            .buttonClassName = "test-class",
            .buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::OkCancel,
        }};

        render(Dialog(controller));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][0]["attributes"]["class"]
                .as<std::string>(),
            "test-class");
        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"][1]["attributes"]["class"]
                .as<std::string>(),
            "test-class");
    }

    TEST_F(TestDialog, DialogCanHaveNoButtons)
    {
        Nui::Components::DialogController controller{
            {.buttonConfiguration = Nui::Components::DialogController::ButtonConfiguration::None}};

        render(Dialog(controller));

        EXPECT_TRUE(
            Nui::val::global("document")["body"]["children"][0]["children"][2]["children"].as<Engine::Array>().empty());
    }
}