#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/utility/delocalized.hpp>

#include <vector>
#include <string>

namespace Nui::Tests
{
    using namespace Engine;
    using namespace std::string_literals;

    class TestDelocalized : public CommonTestFixture
    {
      protected:
        auto bodyChildren()
        {
            return Nui::val::global("document")["body"]["children"];
        }

      public:
        Delocalized<std::string> delocalizedElement_;
    };

    TEST_F(TestDelocalized, SingleSlotDelocalizedRender)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        delocalizedElement_.initializeIfEmpty(span{}("Hello"));
        delocalizedElement_.slot("slot1");

        render(body{}(delocalizedSlot("slot1", delocalizedElement_)));

        ASSERT_EQ(bodyChildren()["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["tagName"].as<std::string>(), "div");
        ASSERT_EQ(bodyChildren()[0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["children"][0]["tagName"].as<std::string>(), "span");
        EXPECT_EQ(bodyChildren()[0]["children"][0]["textContent"].as<std::string>(), "Hello");
    }

    TEST_F(TestDelocalized, SlotShowsAlternativeWhenNotActive)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        delocalizedElement_.initializeIfEmpty(span{}("Hello"));
        delocalizedElement_.slot("slot2");

        render(body{}(delocalizedSlot("slot1", delocalizedElement_)));

        ASSERT_EQ(bodyChildren()["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["tagName"].as<std::string>(), "div");
        ASSERT_EQ(bodyChildren()[0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["children"][0]["tagName"].as<std::string>(), "div");
        EXPECT_EQ(bodyChildren()[0]["children"][0]["attributes"]["style"].as<std::string>(), "display: none");
    }

    TEST_F(TestDelocalized, WrapperGetsAttributesAssignedWhenSet)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        delocalizedElement_.initializeIfEmpty(span{}("Hello"));
        delocalizedElement_.slot("slot1");

        render(body{}(delocalizedSlot("slot1", delocalizedElement_, std::vector<Attribute>{class_ = "wrapper"})));

        ASSERT_EQ(bodyChildren()["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["tagName"].as<std::string>(), "div");
        EXPECT_EQ(bodyChildren()[0]["attributes"]["class"].as<std::string>(), "wrapper");
        ASSERT_EQ(bodyChildren()[0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["children"][0]["tagName"].as<std::string>(), "span");
    }

    TEST_F(TestDelocalized, CanUseDifferentReplacementElement)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        delocalizedElement_.initializeIfEmpty(span{}("Hello"));
        delocalizedElement_.slot("slotX");

        render(body{}(delocalizedSlot("slot1", delocalizedElement_, {}, div{}("Hello"))));

        ASSERT_EQ(bodyChildren()["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["tagName"].as<std::string>(), "div");
        ASSERT_EQ(bodyChildren()[0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(bodyChildren()[0]["children"][0]["tagName"].as<std::string>(), "div");
        EXPECT_EQ(bodyChildren()[0]["children"][0]["textContent"].as<std::string>(), "Hello");
    }
}
