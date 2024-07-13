#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"
#include "engine/object.hpp"

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/listen.hpp>

namespace Nui::Tests
{
    using namespace Engine;
    using namespace std::string_literals;

    class TestEvents : public CommonTestFixture
    {};

    TEST_F(TestEvents, ListenedEventIsExecuted)
    {
        Observed<int> obs;

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        obs = 42;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 42);
    }

    TEST_F(TestEvents, ListenedEventIsNotExecutedWhenValueDoesNotChangeAndEventsAreNotProcessed)
    {
        Observed<int> obs;

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        EXPECT_EQ(calledWith, 77);
    }

    TEST_F(TestEvents, ListenedEventIsNotExecutedWhenEventsAreNotProcessed)
    {
        Observed<int> obs;

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        obs = 42;

        EXPECT_EQ(calledWith, 77);
    }

    TEST_F(TestEvents, ListenedEventIsNotExecutedWhenValueDoesNotChange)
    {
        Observed<int> obs{42};

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 77);
    }

    TEST_F(TestEvents, ListenedSharedObservedEventIsNotExecutedWhenEventsAreNotProcessed)
    {
        auto obs = std::make_shared<Observed<int>>(40);

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        *obs = 42;

        EXPECT_EQ(calledWith, 77);
    }

    TEST_F(TestEvents, ListenedSharedObservedEventIsNotExecutedWhenValueDoesNotChange)
    {
        auto obs = std::make_shared<Observed<int>>(40);

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 77);
    }

    TEST_F(TestEvents, CanUseSharedObservedWithListen)
    {
        auto obs = std::make_shared<Observed<int>>();

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        *obs = 42;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 42);
    }

    TEST_F(TestEvents, SharedObservedMayBeDestroyed)
    {
        auto obs = std::make_shared<Observed<int>>();

        int calledWith = 77;
        listen(globalEventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        obs.reset();
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 77);
    }

    TEST_F(TestEvents, CanUseCustomEventContext)
    {
        EventContext eventContext;

        Observed<int> obs{&eventContext};

        int calledWith = 77;
        listen(eventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        obs = 42;
        eventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 42);
    }

    TEST_F(TestEvents, CanUseListenerReturningVoid)
    {
        EventContext eventContext;

        Observed<int> obs{&eventContext};

        int calledWith = 77;
        listen(eventContext, obs, [&calledWith](int const& value) -> void {
            calledWith = value;
        });

        obs = 42;
        eventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 42);
    }

    TEST_F(TestEvents, CanUseListenerReturningVoidWithSharedObserved)
    {
        EventContext eventContext;

        std::shared_ptr<Observed<int>> obs = std::make_shared<Observed<int>>(&eventContext);

        int calledWith = 77;
        listen(eventContext, obs, [&calledWith](int const& value) -> void {
            calledWith = value;
        });

        *obs = 42;
        eventContext.executeActiveEventsImmediately();

        EXPECT_EQ(calledWith, 42);
    }

    TEST_F(TestEvents, ListenEventIsNotRemovedWhenReturningTrue)
    {
        EventContext eventContext;

        std::shared_ptr<Observed<int>> obs = std::make_shared<Observed<int>>(&eventContext);

        int calledWith = 77;
        listen(eventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return true;
        });

        *obs = 42;
        eventContext.executeActiveEventsImmediately();
        EXPECT_EQ(calledWith, 42);

        *obs = 45;
        eventContext.executeActiveEventsImmediately();
        EXPECT_EQ(calledWith, 45);
    }

    TEST_F(TestEvents, ListenEventIsRemovedWhenReturningFalse)
    {
        EventContext eventContext;

        std::shared_ptr<Observed<int>> obs = std::make_shared<Observed<int>>(&eventContext);

        int calledWith = 77;
        listen(eventContext, obs, [&calledWith](int const& value) -> bool {
            calledWith = value;
            return false;
        });

        *obs = 42;
        eventContext.executeActiveEventsImmediately();
        EXPECT_EQ(calledWith, 42);

        *obs = 45;
        eventContext.executeActiveEventsImmediately();
        EXPECT_EQ(calledWith, 42);
    }
}