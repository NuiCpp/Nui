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

    class TestObserved : public CommonTestFixture
    {};

    TEST_F(TestObserved, ReferenceWrapperCanBeCopied)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed[0];
        auto copy = wrapper;

        EXPECT_EQ(*copy, 1);
    }

    TEST_F(TestObserved, ReferenceWrapperCanBeMoved)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed[0];
        auto moved = std::move(wrapper);

        EXPECT_EQ(*moved, 1);
    }

    TEST_F(TestObserved, ReferenceWrapperCanBeCopyAssigned)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed[0];
        auto copy = observed[1];
        copy = wrapper;

        EXPECT_EQ(*copy, 1);
    }

    TEST_F(TestObserved, ReferenceWrapperCanBeMoveAssigned)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed[0];
        auto copy = observed[1];
        copy = std::move(wrapper);

        EXPECT_EQ(*copy, 1);
    }

    TEST_F(TestObserved, CopiedReferenceWrapperMakesUpdates)
    {
        Nui::val parent;
        Observed<std::string> container{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        auto wrapper = container[12];
        auto copy = wrapper;
        *copy = 'M';

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestObserved, MovedReferenceWrapperMakesUpdates)
    {
        Nui::val parent;
        Observed<std::string> container{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        auto wrapper = container[12];
        auto moved = std::move(wrapper);
        *moved = 'M';

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestObserved, IteratorWrapperCanBeCopied)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed.begin();
        auto copy = wrapper;

        EXPECT_EQ(**copy, 1);
    }

    TEST_F(TestObserved, IteratorWrapperCanBeMoved)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed.begin();
        auto moved = std::move(wrapper);

        EXPECT_EQ(**moved, 1);
    }

    TEST_F(TestObserved, IteratorWrapperCanBeCopyAssigned)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed.begin();
        auto copy = observed.begin();
        copy = wrapper;

        EXPECT_EQ(**copy, 1);
    }

    TEST_F(TestObserved, IteratorWrapperCanBeMoveAssigned)
    {
        Observed<std::vector<int>> observed{{1, 2, 3}};
        auto wrapper = observed.begin();
        auto copy = observed.begin();
        copy = std::move(wrapper);

        EXPECT_EQ(**copy, 1);
    }
}