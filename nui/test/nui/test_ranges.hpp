#pragma once

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <array>

namespace Nui::Tests
{
    using namespace Engine;

    class TestRanges : public CommonTestFixture
    {
      protected:
    };

    TEST_F(TestRanges, SubscriptOperatorAssignmentUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec[2] = 'X';
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, FullAssignmentUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec = {'X', 'Y', 'Z'};
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    // range() over a shared_ptr<Observed<...>> must be reactive (stored as a weak_ptr
    // internally, locked per update). Reassigning the underlying Observed re-renders.
    TEST_F(TestRanges, SharedPtrObservedReassignmentUpdatesView)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Nui::val parent;
        auto vec = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});

        render(body{reference = parent}(range(vec), [](long long, auto const& element) {
            return div{}(std::string{element});
        }));
        textBodyParityTest(*vec, parent);

        *vec = std::vector<char>{'X', 'Y', 'Z'};
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(*vec, parent);
    }

    // Element-level mutation (push_back) through the shared_ptr also re-renders.
    TEST_F(TestRanges, SharedPtrObservedPushBackUpdatesView)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Nui::val parent;
        auto vec = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B'});

        render(body{reference = parent}(range(vec), [](long long, auto const& element) {
            return div{}(std::string{element});
        }));
        textBodyParityTest(*vec, parent);

        vec->push_back('C');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(*vec, parent);
    }

    // weak_ptr<Observed<...>> is accepted too; it reacts while the owner keeps it alive.
    TEST_F(TestRanges, WeakPtrObservedUpdatesView)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Nui::val parent;
        auto vec = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});
        std::weak_ptr<Observed<std::vector<char>>> weak = vec;

        render(body{reference = parent}(range(weak), [](long long, auto const& element) {
            return div{}(std::string{element});
        }));
        textBodyParityTest(*vec, parent);

        vec->push_back('E');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(*vec, parent);
    }

    TEST_F(TestRanges, PushBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.push_back('X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        char X = 'X';
        vec.push_back(X);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, PopBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.pop_back();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, InsertUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        std::vector vec2 = {'1', '2', '3'};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.insert(vec.begin() + 2, 'X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.cbegin() + 2, 'Y');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        char X = 'X';
        vec.insert(vec.begin() + 2, X);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.cbegin() + 2, X);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.begin() + 2, 2, 'Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.cbegin() + 2, 2, 'Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.begin() + 2, vec2.begin(), vec2.end());
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.cbegin() + 2, vec2.begin(), vec2.end());
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.begin() + 2, {'1', '2', '3'});
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.insert(vec.cbegin() + 2, {'1', '2', '3'});
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, MultiplePushBackWork)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.push_back('X');
        vec.push_back('Y');
        vec.push_back('Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, MultiplePushFrontWork)
    {
        Nui::val parent;
        Observed<std::deque<char>> container{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.push_front('X');
        container.push_front('Y');
        container.push_front('Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec;

        vec.reserve(26);
        for (char c = 'A'; c != 'Z'; ++c)
            vec.push_back(c);

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.erase(vec.begin() + 2);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.erase(vec.cbegin() + 2);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.erase(vec.begin() + 2, vec.begin() + 4);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        vec.erase(vec.cbegin() + 2, vec.cbegin() + 4);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ClearUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.clear();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, SwapUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec1{{'A', 'B', 'C', 'D'}};
        std::vector<char> vec2 = {'X', 'Y', 'Z'};

        rangeTextBodyRender(vec1, parent);
        textBodyParityTest(vec1, parent);

        vec1.swap(vec2);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);
    }

    TEST_F(TestRanges, ResizeUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.resize(2);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, LargerThanPreviouslyResizeUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.resize(6);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ResizeWithFillValueUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.resize(6, 'X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, AssignWithFillValueUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec1{{'A', 'B', 'C', 'D'}};
        std::vector<char> vec2 = {'X', 'Y', 'Z'};

        rangeTextBodyRender(vec1, parent);
        textBodyParityTest(vec1, parent);

        vec1.assign(6, 'X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);

        vec1.assign(vec2.begin(), vec2.end());
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);

        vec1.assign({'1', '2', '3'});
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);
    }

    TEST_F(TestRanges, AssignWithRangeUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec1{{'A', 'B', 'C', 'D'}};
        std::vector<char> vec2 = {'X', 'Y', 'Z'};

        rangeTextBodyRender(vec1, parent);
        textBodyParityTest(vec1, parent);

        vec1.assign(vec2.begin(), vec2.end());
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);
    }

    TEST_F(TestRanges, AssignWithInitializerListUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec1{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec1, parent);
        textBodyParityTest(vec1, parent);

        vec1.assign({'X', 'Y', 'Z'});
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);
    }

    TEST_F(TestRanges, EmplaceBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.emplace_back("X");
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EmplaceUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.emplace(vec.cbegin() + 2, 3, 'c');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ModificationThroughPointerUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        *vec.data() = "Y";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ModificationThroughReferenceUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.front() = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ModificationThroughIteratorUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        (*vec.begin()) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfReferenceFromBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.back() = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfReferenceFromAtUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.at(2) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfIteratorFromEndUpdateView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        *(vec.end() - 1) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfIteratorFromRbeginUpdateView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};
        auto rbegin = vec.rbegin();

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        *rbegin = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfIteratorFromRendUpdateView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec{{"A", "B", "C", "D"}};
        auto rend = vec.rend();

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        *std::prev(rend) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, PushFrontUpdatesView)
    {
        Nui::val parent;
        Observed<std::deque<char>> container{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.push_front('X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);

        char c = 'Y';
        container.push_front(c);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, PopFrontUpdatesView)
    {
        Nui::val parent;
        Observed<std::deque<char>> container{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.pop_front();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, AggregatedInsertsUpdateCorrectly)
    {
        Nui::val parent;
        Observed<std::deque<char>> container{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, 'c');
        container.insert(container.begin() + 2, 1, 'y');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, AggregatedInsertsUpdateCorrectly2)
    {
        Nui::val parent;
        Observed<std::deque<char>> container{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, 'c');
        container.push_front('X');
        container.push_back('Y');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, MixOfInsertionAndErasureUpdateCorrectly)
    {
        Nui::val parent;
        Observed<std::deque<char>> container{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, 'c');
        container.erase(container.begin() + 2, container.begin() + 5);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseOverlappingFullyContainedModifyWorks)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);
        rangeTextBodyRender(container, parent);

        for (int i = 5; i != 13; ++i)
            container[i] = '0' + (i - 5);
        container.erase(container.begin() + 6, container.begin() + 12);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseBeforeModifyRequiresInstantUpdate1)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);
        rangeTextBodyRender(container, parent);

        for (int i = 5; i != 13; ++i)
            container[i] = '0' + (i - 5);
        container.erase(container.begin());

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    // An erase before the LAST modify suffices to require an instant update
    TEST_F(TestRanges, EraseBeforeModifyRequiresInstantUpdate2)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);
        rangeTextBodyRender(container, parent);

        container[5] = 'X';
        container[12] = 'X';
        container.erase(container.begin() + 8);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, StdStringEraseNotify)
    {
        Nui::val parent;
        Observed<std::string> container{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.erase(2);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, ResizeFillEraseNotify)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.resize(6, 'X');

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, ResizeEraseNotify)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.resize(6);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, PopFrontNotify)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.pop_front();

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseConstIteratorsNotify)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.erase(container.cbegin() + 2, container.cbegin() + 5);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseIteratorsNotify)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.erase(container.begin() + 2, container.begin() + 5);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseConstIteratorNotify)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.erase(container.cbegin() + 2);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseIteratorNotify)
    {
        Nui::val parent;
        Observed<std::vector<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[12] = 'X';
        container.erase(container.begin() + 2);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, PopBackNotify)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[container.size() - 2] = 'X';
        container.pop_back();
        container.pop_back();
        container.pop_back();
        container.pop_back();
        container.pop_back();

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseOverlappingPreviousInsertionUpdatesCorrectly)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');

        if (container.size() > 2)
            container.erase(container.begin() + 2, container.begin() + 3);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseOverlappingOverEndOfInsertionWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');

        if (container.size() > 2)
            container.erase(container.begin() + 2, container.end());

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseRemovingPreviousInsertionEntirelyWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');

        if (container.size() > 2)
            container.erase(container.begin() + 2, container.begin() + 5);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseOverlappingMultiplePreviousInsertsEntirelyWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');
        container.insert(container.begin() + 5, 3, '_');

        if (container.size() > 2)
            container.erase(container.begin() + 2, container.begin() + 8);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseOverlappingMultiplePreviousInsertsInPartWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');
        container.insert(container.begin() + 5, 3, '_');

        if (container.size() > 2)
            container.erase(container.begin() + 2, container.begin() + 6);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, EraseRightOfInsertWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');

        if (container.size() > 2)
            container.erase(container.begin() + 5, container.begin() + 6);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, CanClearAfterAnyInsertions)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, '_');
        container.push_front('_');
        container.push_back('_');
        container.clear();

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, CanClearAfterModifications)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container[2] = '_';
        container[5] = '_';
        container[12] = '_';
        container.clear();

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, RandomOrderOfModificationsProcessesCorrectly)
    {
        Nui::val parent;
        Observed<std::deque<char>> container;
        for (char c = 'A'; c <= 'Z'; ++c)
            container.push_back(c);

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        // use mersenne twister seeded with 0 to get deterministic results
        std::mt19937 g(0);
        std::uniform_int_distribution<int> dis('A', 'Z');

        std::array<std::function<void()>, 6> actions = {
            [&]() {
                if (container.size() > 2)
                {
                    container.insert(container.begin() + 2, 3, static_cast<char>(dis(g)));
                }
                else
                {
                    container.insert(container.begin(), 3, static_cast<char>(dis(g)));
                }
            },
            [&]() {
                container.push_front(static_cast<char>(dis(g)));
            },
            [&]() {
                container.push_back(static_cast<char>(dis(g)));
            },
            [&]() {
                container.pop_front();
            },
            [&]() {
                container.pop_back();
            },
            [&]() {
                if (container.size() > 2)
                    container.erase(container.begin() + 2, container.begin() + 3);
            }};

        std::uniform_int_distribution<> actionDistribution(0, actions.size() - 1);
        for (int j = 0; j < 5; ++j)
        {
            // scrambly modify the container
            for (int i = 0; i < 20; ++i)
                actions[actionDistribution(g)]();

            globalEventContext.executeActiveEventsImmediately();
            textBodyParityTest(container, parent);
        }
    }

    TEST_F(TestRanges, CanUseNonObservedContainerForRange)
    {
        std::vector<char> characters{'A', 'B', 'C', 'D'};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        for (int i = 0; i != characters.size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{characters[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseSetAsNonObservedContainerForRange)
    {
        std::set<char> characters{'A', 'B', 'C', 'D'};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        int i = 0;
        for (auto const& elem : characters)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(), std::string{elem} + ":" + std::to_string(i));
            ++i;
        }
    }

    TEST_F(TestRanges, CustomRangeCanBeUpdatedViaOtherObserved)
    {
        std::vector<char> characters{'A', 'B', 'C', 'D'};
        Nui::val parent;
        Nui::Observed<bool> other{true};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters, other), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        for (int i = 0; i != characters.size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{characters[i]} + ":" + std::to_string(i));
        }

        characters.push_back('E');
        other = false;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        for (int i = 0; i != characters.size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{characters[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, StaticRangeRendererCanTakeNonConstElement)
    {
        std::vector<char> characters{'A', 'B', 'C', 'D'};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto& element) {
            element = 'X';
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        for (int i = 0; i != characters.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), "X:" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, ObservedRangeRendererCanTakeNonConstElement)
    {
        Nui::Observed<std::vector<char>> characters{{'A', 'B', 'C', 'D'}};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto& element) {
            element = 'X';
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        for (int i = 0; i != characters.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), "X:" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, StaticRangeRendererCanTakeConstObserved)
    {
        std::vector<char> characters{'A', 'B', 'C', 'D'};
        Nui::val parent;
        Nui::Observed<bool> other{true};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto doRender = [&](Nui::Observed<bool> const& constObserved) {
            render(
                body{reference = parent}(
                    range(characters, constObserved), [&characters](long long i, auto const& element) {
                        return div{}(std::string{element} + ":" + std::to_string(i));
                    }));
        };
        doRender(other);

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        for (int i = 0; i != characters.size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{characters[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseObservedNonRandomAccessRange)
    {
        Nui::Observed<std::set<char>> characters{{'A', 'B', 'C', 'D'}};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        int i = 0;
        for (auto const& elem : characters.value())
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(), std::string{elem} + ":" + std::to_string(i));
            ++i;
        }
    }

    TEST_F(TestRanges, CanUseStaticNonRandomAccessRange)
    {
        std::set<char> characters{'A', 'B', 'C', 'D'};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters.size()));
        int i = 0;
        for (auto const& elem : characters)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(), std::string{elem} + ":" + std::to_string(i));
            ++i;
        }
    }

    TEST_F(TestRanges, CanUseStaticRangeSharedPointer)
    {
        std::shared_ptr<std::vector<char>> characters =
            std::make_shared<std::vector<char>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->size()));
        for (int i = 0; i != characters->size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{(*characters)[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseStaticRangeSharedPointerToConst)
    {
        std::shared_ptr<const std::vector<char>> characters =
            std::make_shared<const std::vector<char>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->size()));
        for (int i = 0; i != characters->size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{(*characters)[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseStaticRangeWeakPointer)
    {
        std::shared_ptr<std::vector<char>> characters =
            std::make_shared<std::vector<char>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;
        std::weak_ptr<std::vector<char>> weakCharacters{characters};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(weakCharacters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->size()));
        for (int i = 0; i != characters->size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{(*characters)[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseStaticRangeWeakPointerToConst)
    {
        std::shared_ptr<const std::vector<char>> characters =
            std::make_shared<const std::vector<char>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;
        std::weak_ptr<const std::vector<char>> weakCharacters{characters};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(weakCharacters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->size()));
        for (int i = 0; i != characters->size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{(*characters)[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseSharedStaticRendererTakingNonConst)
    {
        std::shared_ptr<std::vector<char>> characters =
            std::make_shared<std::vector<char>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto& element) {
            element = 'X';
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->size()));
        for (int i = 0; i != characters->size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), "X:" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseWeakStaticRendererTakingNonConst)
    {
        std::shared_ptr<std::vector<char>> characters =
            std::make_shared<std::vector<char>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;
        std::weak_ptr<std::vector<char>> weakCharacters{characters};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(weakCharacters), [&characters](long long i, auto& element) {
            element = 'X';
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->size()));
        for (int i = 0; i != characters->size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), "X:" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseObservedRangeSharedPointer)
    {
        auto characters = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->value().size()));
        for (int i = 0; i != characters->value().size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{characters->value()[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseObservedRangeWeakPointer)
    {
        auto characters = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;
        std::weak_ptr<Observed<std::vector<char>>> weakCharacters{characters};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(weakCharacters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->value().size()));
        for (int i = 0; i != characters->value().size(); ++i)
        {
            EXPECT_EQ(
                parent["children"][i]["textContent"].as<std::string>(),
                std::string{characters->value()[i]} + ":" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseObservedRangeSharedPointerRendererTakingNonConst)
    {
        auto characters = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(characters), [&characters](long long i, auto& element) {
            element = 'X';
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->value().size()));
        for (int i = 0; i != characters->value().size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), "X:" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, CanUseObservedRangeWeakPointerRendererTakingNonConst)
    {
        auto characters = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;
        std::weak_ptr<Observed<std::vector<char>>> weakCharacters{characters};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(weakCharacters), [&characters](long long i, auto& element) {
            element = 'X';
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(characters->value().size()));
        for (int i = 0; i != characters->value().size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), "X:" + std::to_string(i));
        }
    }

    TEST_F(TestRanges, WeakObservedMayExpireErrorlessly)
    {
        auto characters = std::make_shared<Observed<std::vector<char>>>(std::vector<char>{'A', 'B', 'C', 'D'});
        Nui::val parent;
        std::weak_ptr<Observed<std::vector<char>>> weakCharacters{characters};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(weakCharacters), [&characters](long long i, auto const& element) {
            return div{}(std::string{element} + ":" + std::to_string(i));
        }));

        // activates an event
        characters->push_back('E');
        characters.reset();
        EXPECT_NO_FATAL_FAILURE(globalEventContext.executeActiveEventsImmediately());
    }

    TEST_F(TestRanges, PushBackOnlyDoesNecessaryUpdates)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        std::function<Nui::ElementRenderer(char element)> renderer{[](char element) -> Nui::ElementRenderer {
            return Nui::Elements::div{}(std::string{element});
        }};

        render(body{reference = parent}(range(vec), [&vec, &renderer](long long i, auto const& element) {
            return renderer(element);
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size()));

        std::vector<char> renderedElements{};
        renderer = [&](char element) -> Nui::ElementRenderer {
            renderedElements.push_back(element);
            return Nui::Elements::div{}(std::string{element});
        };

        vec.push_back('E');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size()));
        EXPECT_EQ(renderedElements.size(), 1);
        EXPECT_EQ(renderedElements[0], 'E');

        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, EmplaceBackOnlyDoesNecessaryUpdates)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        std::function<Nui::ElementRenderer(char element)> renderer{[](char element) -> Nui::ElementRenderer {
            return Nui::Elements::div{}(std::string{element});
        }};

        render(body{reference = parent}(range(vec), [&vec, &renderer](long long i, auto const& element) {
            return renderer(element);
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size()));

        std::vector<char> renderedElements{};
        renderer = [&](char element) -> Nui::ElementRenderer {
            renderedElements.push_back(element);
            return Nui::Elements::div{}(std::string{element});
        };

        vec.emplace_back('E');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size()));
        EXPECT_EQ(renderedElements.size(), 1);
        EXPECT_EQ(renderedElements[0], 'E');

        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, InsertOnlyDoesNecessaryUpdates)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        std::function<Nui::ElementRenderer(char element)> renderer{[](char element) -> Nui::ElementRenderer {
            return Nui::Elements::div{}(std::string{element});
        }};

        render(body{reference = parent}(range(vec), [&vec, &renderer](long long i, auto const& element) {
            return renderer(element);
        }));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"]["length"].as<long long>(),
            static_cast<long long>(vec.size()));

        std::vector<char> renderedElements{};
        renderer = [&](char element) -> Nui::ElementRenderer {
            renderedElements.push_back(element);
            return Nui::Elements::div{}(std::string{element});
        };

        vec.insert(vec.begin() + 2, 'X');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"]["length"].as<long long>(),
            static_cast<long long>(vec.size()));
        EXPECT_EQ(renderedElements.size(), 1);
        EXPECT_EQ(renderedElements[0], 'X');

        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(
                Nui::val::global("document")["body"]["children"][i]["textContent"].as<std::string>(),
                std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, ErasingTheLastElementWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.erase(vec.begin() + 3);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingTheFirstElementWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.erase(vec.begin());
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingAllWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.clear();
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingFrontAndEndWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.erase(vec.begin());
        vec.erase(vec.begin() + 2);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingOverlappingRangesWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.erase(vec.begin() + 2, vec.begin() + 3);
        vec.erase(vec.begin() + 1, vec.begin() + 3);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingOverlappingRangesWorksAsExpectedWhenAlsoDeletingFromTheEnd)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.erase(vec.begin() + vec.size() - 1);
        vec.erase(vec.begin() + 2, vec.begin() + 3);
        vec.erase(vec.begin() + 1, vec.begin() + 3);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingOverlappingRangesWorksAsExpectedWhenAlsoDeletingFromTheEndAndTheBeginning)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.erase(vec.begin());
        vec.erase(vec.begin() + vec.size() - 1);
        vec.erase(vec.begin() + 3, vec.begin() + 4);
        vec.erase(vec.begin() + 2, vec.begin() + 5);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingEvenElementsFromTheFrontWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        const auto initialSize = vec.size();
        for (int i = 0; vec.size() > initialSize / 2; ++i)
        {
            vec.erase(vec.begin() + i);
        }
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingEvenElementsFromTheBackWorksAsExpected)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        const auto initialSize = vec.size();
        for (int i = 0; vec.size() > initialSize / 2; ++i)
        {
            vec.erase(vec.begin() + vec.size() - i - 1);
        }
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingUsingPopBackWorks)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.pop_back();
        vec.pop_back();
        vec.pop_back();
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ErasingUsingPopFrontWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.pop_front();
        vec.pop_front();
        vec.pop_front();
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EraseUsingEraseWithSingleIteratorWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        auto it = vec.begin();
        std::advance(it, 3);
        vec.erase(it);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EraseUsingEraseWithTwoIteratorsWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        auto it1 = vec.begin();
        std::advance(it1, 3);
        auto it2 = vec.begin();
        std::advance(it2, 5);
        vec.erase(it1, it2);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EraseUsingResizeToSmallerWorks)
    {
        Nui::val parent;
        Observed<std::deque<char>> vec{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::string{element});
        }));

        vec.resize(5);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EraseMultiOverlapTest)
    {
        constexpr static auto totalElements = 30;

        Nui::val parent;
        Observed<std::vector<int>> vec;
        for (int i = 0; i != totalElements; ++i)
        {
            vec.value().push_back(i);
        }

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::to_string(element));
        }));

        const auto initialSize = vec.size();
        for (int i = 0; vec.size() > initialSize / 2; ++i)
        {
            vec.erase(vec.begin() + vec.size() - i - 1);
        }
        vec.erase(vec.begin(), vec.begin() + 7);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, MassiveEraseTest)
    {
        constexpr static auto totalRanges = 20;
        constexpr static auto totalElements = 1000;

        Nui::val parent;
        Observed<std::vector<int>> vec;
        for (int i = 0; i != totalElements; ++i)
        {
            vec.value().push_back(i);
        }

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        render(body{reference = parent}(range(vec), [&vec](long long i, auto const& element) {
            return Nui::Elements::div{}(std::to_string(element));
        }));

        std::mt19937 g(0);
        for (int i = 0; i != totalRanges; ++i)
        {
            std::uniform_int_distribution<int> dis(0, vec.size() - 1);
            std::uniform_int_distribution<int> width(0, 10);

            auto low = dis(g);
            auto high = std::min(static_cast<int>(vec.size()) - 1, low + width(g));
            if (low > high)
            {
                std::swap(low, high);
            }

            vec.erase(vec.begin() + low, vec.begin() + high + 1);
        }

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EraseRemoveIdiomTest)
    {
        Nui::val parent;
        Observed<std::vector<int>> vec{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        int renders = 0;
        auto renderElement = [&vec, &renders](long long i, auto const& element) {
            ++renders;
            return Nui::Elements::div{}(std::to_string(element));
        };

        render(body{reference = parent}(range(vec), renderElement));
        renders = 0;

        vec.erase(
            std::remove_if(
                vec.begin(),
                vec.end(),
                [](int val) {
                    return val & 1;
                }),
            vec.end());

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);

        EXPECT_EQ(renders, 0);
    }

    TEST_F(TestRanges, ParentRerenderRemakesAllChildren)
    {
        Observed<bool> outer{false};
        Observed<std::vector<char>> inner{{'A', 'B', 'C', 'D'}};
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{}(observe(outer), [&inner, &parent]() -> Nui::ElementRenderer {
            return div{reference.onMaterialize([&parent](Nui::val va) {
                parent = va;
            })}(range(inner), [&inner](long long i, auto const& element) {
                return div{}(std::string{element});
            });
        }));

        textBodyParityTest(inner, parent);

        outer = true;
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(inner, parent);
    }

    TEST_F(TestRanges, AllRangeObserversAreRerenderedOnChange)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [&vec](long long i, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);

        vec.push_back('E');
        vec[0] = 'X';
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(this->aggregateObservedCharList(vec), this->getChildrenBodyTextConcat(parent1));
        EXPECT_EQ(this->aggregateObservedCharList(vec), this->getChildrenBodyTextConcat(parent2));
    }

    // ---------------------------------------------------------------------
    // Multi-subscriber tests for the fullRangeUpdate path
    // ---------------------------------------------------------------------
    // These exercise mutations that set rangeContext.fullRangeUpdate_=true
    // (clear, operator=, swap, assign). Currently buggy: the first range
    // subscriber consumes the shared rangeContext via reset(), leaving the
    // second subscriber with nothing to render. After the per-subscriber
    // context refactor these will pass.

    TEST_F(TestRanges, MultiSubscriber_ClearThenPushBack)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.clear();
        vec.push_back('A');
        vec.push_back('B');
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_AssignNewVector)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec = std::vector<char>{'X', 'Y', 'Z'};
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Swap)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        std::vector<char> other{'P', 'Q'};
        vec.swap(other);
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_AssignViaInitializerList)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.assign({'M', 'N', 'O', 'P'});
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    // ---------------------------------------------------------------------
    // Multi-subscriber tests for the per-operation paths (Insert/Modify/
    // Erase). These exercise the path where rangeContext stays through the
    // event cycle and is reset by the registered afterEffect rather than
    // eagerly by the renderer. Most should already pass today; they are
    // here to lock the contract in once per-subscriber contexts land.

    TEST_F(TestRanges, MultiSubscriber_PushBack)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.push_back('D');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_PopBack)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.pop_back();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_PushFront_Deque)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::deque<char>> deq{{'A', 'B', 'C'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(deq), renderer), div{reference = parent2}(range(deq), renderer)));

        deq.push_front('Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(deq, parent1);
        textBodyParityTest(deq, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_PopFront_Deque)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::deque<char>> deq{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(deq), renderer), div{reference = parent2}(range(deq), renderer)));

        deq.pop_front();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(deq, parent1);
        textBodyParityTest(deq, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_EmplaceBack)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.emplace_back('D');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_EmplaceFront_Deque)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::deque<char>> deq{{'A', 'B', 'C'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(deq), renderer), div{reference = parent2}(range(deq), renderer)));

        deq.emplace_front('Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(deq, parent1);
        textBodyParityTest(deq, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Insert_PosValue)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.insert(vec.begin() + 2, 'X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Insert_PosCountValue)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.insert(vec.begin() + 2, 3, 'Z');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Insert_PosFirstLast)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        std::vector<char> source{'X', 'Y'};
        vec.insert(vec.begin() + 2, source.begin(), source.end());
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Insert_PosInitList)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.insert(vec.begin() + 2, {'1', '2', '3'});
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Erase_Pos)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.erase(vec.begin() + 1);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Erase_FirstLast)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.erase(vec.begin() + 1, vec.begin() + 4);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Resize_Grow)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.resize(6);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Resize_Shrink)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D', 'E'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.resize(2);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_Resize_GrowWithFill)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.resize(6, 'F');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_IndexedAssign)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec[2] = 'X';
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_IteratorDerefAssign)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        *(vec.begin() + 1) = 'Y';
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, MultiSubscriber_MixedSequence)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(div{reference = parent1}(range(vec), renderer), div{reference = parent2}(range(vec), renderer)));

        vec.push_back('E');
        vec[0] = 'X';
        vec.erase(vec.begin() + 2);
        vec.insert(vec.begin(), '_');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);
    }

    TEST_F(TestRanges, SingleSubscriber_MixedSequence_DiagnosticControl)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};
        rangeTextBodyRender(vec, parent);

        vec.push_back('E');
        vec[0] = 'X';
        vec.erase(vec.begin() + 2);
        vec.insert(vec.begin(), '_');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, SetInsertReplacesNotAppends)
    {
        Nui::val parent;
        Observed<std::set<char>> s{{'A', 'B', 'C'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(s), [](long long, auto const& element) {
            return div{}(std::string{element});
        }));
        ASSERT_EQ(parent["children"]["length"].as<long long>(), 3);

        s.insert('D');
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(parent["children"]["length"].as<long long>(), 4);
    }

    TEST_F(TestRanges, MultiSubscriber_SetInsertReplacesNotAppends)
    {
        Nui::val parent1;
        Nui::val parent2;
        Observed<std::set<char>> s{{'A', 'B', 'C'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(
            div{reference = parent1}(range(s), renderer),
            div{reference = parent2}(range(s), renderer)));

        ASSERT_EQ(parent1["children"]["length"].as<long long>(), 3);
        ASSERT_EQ(parent2["children"]["length"].as<long long>(), 3);

        s.insert('D');
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(parent1["children"]["length"].as<long long>(), 4);
        EXPECT_EQ(parent2["children"]["length"].as<long long>(), 4);
    }

    TEST_F(TestRanges, MultiSubscriber_AfterPriorBodyReplace_SetInsert)
    {
        // Reproduces the failure-in-context: a prior render+mutation cycle
        // before the set scope causes the set's insert to double-render its
        // parents. Standalone the test passes; this minimizes the
        // surrounding state to identify what leaks.
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        {
            Nui::val deqp1;
            Nui::val deqp2;
            Observed<std::deque<char>> deq{{'A', 'B'}};
            auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
                return div{}(std::string{element});
            };
            render(body{}(
                div{reference = deqp1}(range(deq), renderer),
                div{reference = deqp2}(range(deq), renderer)));
            deq.push_back('C');
            globalEventContext.executeActiveEventsImmediately();
        }

        Nui::val parent1;
        Nui::val parent2;
        Observed<std::set<char>> s{{'A', 'B', 'C'}};
        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };
        render(body{}(
            div{reference = parent1}(range(s), renderer),
            div{reference = parent2}(range(s), renderer)));

        ASSERT_EQ(parent1["children"]["length"].as<long long>(), 3);
        ASSERT_EQ(parent2["children"]["length"].as<long long>(), 3);

        s.insert('D');
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(parent1["children"]["length"].as<long long>(), 4);
        EXPECT_EQ(parent2["children"]["length"].as<long long>(), 4);
    }

    TEST_F(TestRanges, RepeatedIndexedAssignAtSamePosition)
    {
        // Regression: repeated Replace ops at the same position used to leave
        // the parent's mock children array referencing the very first
        // replacement, because replaceElementImpl reassigns element_ to the
        // new replacement after replaceWith and the mock's replaceWith only
        // redirected the original handle.
        Nui::val parent;
        Observed<std::vector<int>> vec{{0, 0, 0}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(vec), [](long long, auto const& element) {
            return div{}(std::to_string(element));
        }));
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), std::string{"0"});

        vec[1] = 100;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), std::string{"100"});

        vec[1] = 200;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), std::string{"200"});

        vec[1] = 300;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), std::string{"300"});
    }

    // ---------------------------------------------------------------------
    // Subscriber lifecycle and stress tests
    // ---------------------------------------------------------------------

    TEST_F(TestRanges, SubscriberLifetime_Detach)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<bool> showSecond{false};
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        // clang-format off
        render(
            body{}(
                div{reference = parent1}(range(vec), renderer),
                div{}(
                    observe(showSecond),
                    [&parent2, &vec, &renderer, &showSecond]() -> Nui::ElementRenderer {
                        if (!showSecond.value())
                            return Nui::nil();
                        return div{reference = parent2}(range(vec), renderer);
                    }
                )
            )
        );
        // clang-format on

        EXPECT_EQ(vec.readerContextCount(), 1u);

        showSecond = true;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(vec.readerContextCount(), 2u);
        textBodyParityTest(vec, parent1);
        textBodyParityTest(vec, parent2);

        showSecond = false;
        globalEventContext.executeActiveEventsImmediately();

        // First mutation tears down the dead subscriber's renderer (its event
        // returns false from its action), but at this point its own
        // forEachReaderContext iteration already saw the weak_ptr alive.
        vec.push_back('E');
        globalEventContext.executeActiveEventsImmediately();
        // Second mutation prunes the now-expired weak_ptr.
        vec.push_back('F');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(vec.readerContextCount(), 1u);
        textBodyParityTest(vec, parent1);
    }

    TEST_F(TestRanges, SubscriberLifetime_DiesMidBroadcast)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<bool> showSecond{true};
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        // clang-format off
        render(
            body{}(
                div{reference = parent1}(range(vec), renderer),
                div{}(
                    observe(showSecond),
                    [&parent2, &vec, &renderer, &showSecond]() -> Nui::ElementRenderer {
                        if (!showSecond.value())
                            return Nui::nil();
                        return div{reference = parent2}(range(vec), renderer);
                    }
                )
            )
        );
        // clang-format on

        EXPECT_EQ(vec.readerContextCount(), 2u);

        // Kill the second subscriber and immediately mutate. The broadcast must
        // tolerate an expired weak_ptr without crashing and the survivor must
        // stay in sync.
        showSecond = false;
        vec.push_back('E');
        vec.push_back('F');
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent1);

        vec.erase(vec.begin());
        vec.insert(vec.begin() + 1, 'Z');
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parent1);
        EXPECT_EQ(vec.readerContextCount(), 1u);
    }

    TEST_F(TestRanges, FiveSubscribers_RandomMutations)
    {
        std::array<Nui::val, 5> parents;
        Observed<std::vector<int>> vec{{0, 1, 2, 3, 4, 5, 6, 7}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::to_string(element));
        };

        // clang-format off
        render(body{}(
            div{reference = parents[0]}(range(vec), renderer),
            div{reference = parents[1]}(range(vec), renderer),
            div{reference = parents[2]}(range(vec), renderer),
            div{reference = parents[3]}(range(vec), renderer),
            div{reference = parents[4]}(range(vec), renderer)
        ));
        // clang-format on

        EXPECT_EQ(vec.readerContextCount(), 5u);

        // Cycle through the supported single-type ops in batches, flushing
        // between each. This stresses the broadcast loop with five readers
        // without crossing into the parked PerformAndRetry-with-mixed-types
        // territory (DISABLED_*).
        std::mt19937 rng{0xC0FFEEu};
        const auto runBatch = [&](int op, int count) {
            for (int n = 0; n != count; ++n)
            {
                const auto size = vec.size();
                switch (op)
                {
                    case 0: // push_back
                        vec.push_back(n);
                        break;
                    case 1: // pop_back
                        if (size > 0u)
                            vec.pop_back();
                        break;
                    case 2: // insert
                    {
                        std::uniform_int_distribution<std::size_t> posDist{0u, size};
                        const auto pos = posDist(rng);
                        vec.insert(vec.begin() + static_cast<std::ptrdiff_t>(pos), -n);
                        break;
                    }
                    case 3: // erase
                        if (size > 0u)
                        {
                            std::uniform_int_distribution<std::size_t> posDist{0u, size - 1u};
                            const auto pos = posDist(rng);
                            vec.erase(vec.begin() + static_cast<std::ptrdiff_t>(pos));
                        }
                        break;
                    case 4: // indexed assign
                        if (size > 0u)
                        {
                            std::uniform_int_distribution<std::size_t> posDist{0u, size - 1u};
                            const auto pos = posDist(rng);
                            vec[pos] = 1000 + n;
                        }
                        break;
                }
                globalEventContext.executeActiveEventsImmediately();
            }
            std::string source;
            for (auto v : vec.value())
                source += std::to_string(v) + ",";
            for (std::size_t pIdx = 0; pIdx != parents.size(); ++pIdx)
            {
                auto const& parent = parents[pIdx];
                ASSERT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size()))
                    << "parent[" << pIdx << "] op " << op;
                std::string viewReality;
                for (long long i = 0, end = parent["children"]["length"].as<long long>(); i != end; ++i)
                    viewReality += parent["children"][i]["textContent"].as<std::string>() + ",";
                ASSERT_EQ(source, viewReality) << "parent[" << pIdx << "] op " << op;
            }
        };

        runBatch(0, 20); // push_back x 20
        runBatch(4, 20); // indexed assign x 20
        runBatch(2, 20); // insert x 20
        runBatch(3, 20); // erase x 20
        runBatch(1, 5); // pop_back x 5
        runBatch(4, 20); // indexed assign x 20 (post-mutation)

        EXPECT_EQ(vec.readerContextCount(), 5u);
    }

    TEST_F(TestRanges, InteractionAcrossSubscribers)
    {
        // Subscribers attached at different points in time must all converge
        // to the same view of the container after later mutations.
        Nui::val parentEarly;
        Nui::val parentLate;

        Observed<bool> mountLate{false};
        Observed<std::vector<char>> vec{{'A', 'B', 'C'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        // clang-format off
        render(
            body{}(
                div{reference = parentEarly}(range(vec), renderer),
                div{}(
                    observe(mountLate),
                    [&parentLate, &vec, &renderer, &mountLate]() -> Nui::ElementRenderer {
                        if (!mountLate.value())
                            return Nui::nil();
                        return div{reference = parentLate}(range(vec), renderer);
                    }
                )
            )
        );
        // clang-format on

        // First mutate while only the early subscriber is mounted.
        vec.push_back('D');
        vec.push_back('E');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parentEarly);

        // Mount the late subscriber. It should render the current state.
        mountLate = true;
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parentEarly);
        textBodyParityTest(vec, parentLate);
        EXPECT_EQ(vec.readerContextCount(), 2u);

        // Now mutate again. Both subscribers must agree.
        vec.erase(vec.begin() + 1);
        globalEventContext.executeActiveEventsImmediately();
        vec.insert(vec.begin(), 'Z');
        globalEventContext.executeActiveEventsImmediately();
        vec[0] = 'Q';
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(vec, parentEarly);
        textBodyParityTest(vec, parentLate);
    }

    TEST_F(TestRanges, OperatorAssignBetweenContainers)
    {
        Nui::val parentA1;
        Nui::val parentA2;

        Observed<std::vector<char>> a{{'A', 'B', 'C'}};
        Observed<std::vector<char>> b{{'X', 'Y', 'Z', 'W'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        render(body{}(
            div{reference = parentA1}(range(a), renderer), div{reference = parentA2}(range(a), renderer)));

        EXPECT_EQ(a.readerContextCount(), 2u);
        EXPECT_EQ(b.readerContextCount(), 0u);

        a = std::move(b.value());
        globalEventContext.executeActiveEventsImmediately();

        textBodyParityTest(a, parentA1);
        textBodyParityTest(a, parentA2);

        // b is unrendered; mutating it must not affect a's subscribers.
        b = std::vector<char>{'1', '2'};
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(a, parentA1);
        textBodyParityTest(a, parentA2);
    }

    TEST_F(TestRanges, Map_Set_Deque_MultiSubscriber)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // deque
        {
            Nui::val parent1;
            Nui::val parent2;
            Observed<std::deque<char>> deq{{'A', 'B', 'C', 'D'}};
            auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
                return div{}(std::string{element});
            };
            render(body{}(
                div{reference = parent1}(range(deq), renderer), div{reference = parent2}(range(deq), renderer)));

            deq.push_front('Z');
            globalEventContext.executeActiveEventsImmediately();
            textBodyParityTest(deq, parent1);
            textBodyParityTest(deq, parent2);

            deq.push_back('E');
            globalEventContext.executeActiveEventsImmediately();
            textBodyParityTest(deq, parent1);
            textBodyParityTest(deq, parent2);

            EXPECT_EQ(deq.readerContextCount(), 2u);
        }

        // set
        {
            Nui::val parent1;
            Nui::val parent2;
            Observed<std::set<char>> s{{'A', 'B', 'C'}};
            auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
                return div{}(std::string{element});
            };
            render(body{}(
                div{reference = parent1}(range(s), renderer), div{reference = parent2}(range(s), renderer)));

            EXPECT_EQ(parent1["children"]["length"].as<long long>(), static_cast<long long>(s.size()));
            EXPECT_EQ(parent2["children"]["length"].as<long long>(), static_cast<long long>(s.size()));

            s.insert('D');
            globalEventContext.executeActiveEventsImmediately();
            EXPECT_EQ(parent1["children"]["length"].as<long long>(), static_cast<long long>(s.size()));
            EXPECT_EQ(parent2["children"]["length"].as<long long>(), static_cast<long long>(s.size()));

            s.insert('Z');
            globalEventContext.executeActiveEventsImmediately();
            EXPECT_EQ(parent1["children"]["length"].as<long long>(), static_cast<long long>(s.size()));
            EXPECT_EQ(parent2["children"]["length"].as<long long>(), static_cast<long long>(s.size()));

            EXPECT_EQ(s.readerContextCount(), 2u);
        }

        // map: routed through UnoptimizedRangeRenderer rather than the
        // per-subscriber-context path. Verify both subscribers render the
        // initial state.
        {
            Nui::val parent1;
            Nui::val parent2;
            Observed<std::map<int, char>> m{{{1, 'A'}, {2, 'B'}, {3, 'C'}}};
            auto renderer = [](long long, auto const& element) -> Nui::ElementRenderer {
                return div{}(std::to_string(element.first) + ":" + std::string{element.second});
            };
            render(body{}(
                div{reference = parent1}(range(m), renderer), div{reference = parent2}(range(m), renderer)));

            EXPECT_EQ(parent1["children"]["length"].as<long long>(), static_cast<long long>(m->size()));
            EXPECT_EQ(parent2["children"]["length"].as<long long>(), static_cast<long long>(m->size()));
        }
    }

    TEST_F(TestRanges, MoveAssignedObservedHasNoSubscribers)
    {
        // A freshly-constructed Observed has no subscribers until rendered.
        Observed<std::vector<char>> fresh{{'A', 'B'}};
        EXPECT_EQ(fresh.readerContextCount(), 0u);

        // After replacing the value via operator=(T&&) but without any range()
        // having been bound, there must still be no subscribers.
        fresh = std::vector<char>{'X', 'Y', 'Z'};
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(fresh.readerContextCount(), 0u);

        // .assign() likewise must not create subscribers.
        fresh.assign({'P', 'Q', 'R', 'S'});
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(fresh.readerContextCount(), 0u);
    }

    TEST_F(TestRanges, CanRenderRangeEvenDuringPendingModification)
    {
        Nui::val parent1;
        Nui::val parent2;

        Observed<bool> delayed{false};
        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        auto renderer = [&vec](long long i, auto const& element) -> Nui::ElementRenderer {
            return div{}(std::string{element});
        };

        // clang-format off
        render(
            body{}(
                div{
                    reference = parent1,
                }(range(vec), renderer),
                div{}(
                    observe(delayed),
                    [&parent2, &vec, &renderer, &delayed]() -> Nui::ElementRenderer {
                        if (!delayed.value())
                            return Nui::nil();
                        return div{
                            reference = parent2,
                        }(range(vec), renderer);
                    }
                )
            )
        );
        // clang-format on

        delayed = true;
        vec.push_back('E');
        vec[0] = 'X';

        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(this->aggregateObservedCharList(vec), this->getChildrenBodyTextConcat(parent1));
        EXPECT_EQ(this->aggregateObservedCharList(vec), this->getChildrenBodyTextConcat(parent2));
    }

    TEST_F(TestRanges, CanHavePrefixElementsBeforeRange)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    div{}("Prefix1"),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 2));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 2]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, CanHavePostifxElementsAfterRange)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).after(
                    div{}("Postfix1"),
                    div{}("Postfix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 2));
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i]["textContent"].as<std::string>(), std::string{vec[i]});
        }
        EXPECT_EQ(parent["children"][vec.size()]["textContent"].as<std::string>(), "Postfix1");
        EXPECT_EQ(parent["children"][vec.size() + 1]["textContent"].as<std::string>(), "Postfix2");
    }

    TEST_F(TestRanges, CanHavePostfixAndPrefixElements)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec)
                    .before(
                        div{}("Prefix1"),
                        div{}("Prefix2")
                    )
                    .after(
                        div{}("Postfix1"),
                        div{}("Postfix2")
                    ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 4));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 2]["textContent"].as<std::string>(), std::string{vec[i]});
        }
        EXPECT_EQ(parent["children"][vec.size() + 2]["textContent"].as<std::string>(), "Postfix1");
        EXPECT_EQ(parent["children"][vec.size() + 3]["textContent"].as<std::string>(), "Postfix2");
    }

    TEST_F(TestRanges, PrefixElementCanBeNil)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    Nui::nil(),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 1));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 1]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, InsertInsertsAtCorrectPositionWithPrefix)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    div{}("Prefix1"),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec.insert(vec.begin() + 2, 'X');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 2));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 2]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, EraseErasesAtCorrectPositionWithPrefix)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    div{}("Prefix1"),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec.erase(vec.begin() + 1);
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 2));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 2]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, ClearingRangeStillRendersPrefixes)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    div{}("Prefix1"),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec.clear();
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
    }

    TEST_F(TestRanges, ModificationIsPerformedAtRightLocationWithPrefixes)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    div{}("Prefix1"),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec[1] = 'X';
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 2));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 2]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, ModificationIsPerformedAtRightLocationWithNilPrefix)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    Nui::nil(),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec[1] = 'X';
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 1));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 1]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, EraseErasesAtCorrectPositionWithNilPrefix)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    Nui::nil(),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec.erase(vec.begin() + 1);
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 1));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 1]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, InsertInsertsAtCorrectPositionWithNilPrefix)
    {
        Nui::val parent;

        Observed<std::vector<char>> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    Nui::nil(),
                    div{}("Prefix2")
                ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec.insert(vec.begin() + 2, 'X');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 1));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 1]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestRanges, PrefixAndPostfixWorkWithUnoptimizedRange)
    {
        Nui::val parent;

        std::vector<int> vec{1, 2, 3, 4, 5};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec)
                    .before(
                        div{}("Prefix1"),
                        div{}("Prefix2")
                    )
                    .after(
                        div{}("Postfix1"),
                        div{}("Postfix2")
                    ),
                [&vec](long long i, auto const& element) {
                    return div{}(std::to_string(element));
                }
            )
        );
        // clang-format on

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 4));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix1");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 2]["textContent"].as<std::string>(), std::to_string(vec[i]));
        }
    }

    TEST_F(TestRanges, CanUseUnorderedMapForRange)
    {
        Nui::val parent;

        Observed<std::unordered_map<int, char>> map{{{1, 'A'}, {2, 'B'}, {3, 'C'}}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(map), [&map](long index, auto const& element) {
            return div{}(std::to_string(element.first) + ":" + std::string{element.second});
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(map->size()));
        for (const auto& [key, value] : map.value())
        {
            bool found = false;
            for (int i = 0; i != parent["children"]["length"].as<long long>(); ++i)
            {
                if (parent["children"][i]["textContent"].as<std::string>() ==
                    std::to_string(key) + ":" + std::string{value})
                {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found);
        }
    }

    TEST_F(TestRanges, CanUseMapForRange)
    {
        Nui::val parent;

        Observed<std::map<int, char>> map{{{1, 'A'}, {2, 'B'}, {3, 'C'}}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        render(body{reference = parent}(range(map), [&map](long index, auto const& element) {
            return div{}(std::to_string(element.first) + ":" + std::string{element.second});
        }));

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(map->size()));
        for (const auto& [key, value] : map.value())
        {
            bool found = false;
            for (int i = 0; i != parent["children"]["length"].as<long long>(); ++i)
            {
                if (parent["children"][i]["textContent"].as<std::string>() ==
                    std::to_string(key) + ":" + std::string{value})
                {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found);
        }
    }

    TEST_F(TestRanges, OwningRangeKeepsRvalueContainerAlive)
    {
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // Build a local vector and std::move it into range() — the resulting
        // UnoptimizedRange must own the vector so iteration stays safe after
        // the caller's scope exits.
        auto makeVec = []() {
            std::vector<int> local{10, 20, 30};
            return local;
        };

        // clang-format off
        render(
            body{reference = parent}(
                range(makeVec()),
                [](long long, auto const& element) {
                    return div{}(std::to_string(element));
                }
            )
        );
        // clang-format on

        ASSERT_EQ(parent["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "10");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "20");
        EXPECT_EQ(parent["children"][2]["textContent"].as<std::string>(), "30");
    }

    TEST_F(TestRanges, OwningRangeWorksWithMovedLocal)
    {
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        std::vector<std::string> children;
        children.emplace_back("alpha");
        children.emplace_back("beta");
        children.emplace_back("gamma");

        // clang-format off
        render(
            body{reference = parent}(
                range(std::move(children)),
                [](long long, auto const& element) {
                    return div{}(element);
                }
            )
        );
        // clang-format on

        ASSERT_EQ(parent["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "alpha");
        EXPECT_EQ(parent["children"][1]["textContent"].as<std::string>(), "beta");
        EXPECT_EQ(parent["children"][2]["textContent"].as<std::string>(), "gamma");
    }

    TEST_F(TestRanges, LvalueRangeStillWorks)
    {
        // Regression guard: the rvalue overload must not shadow the
        // long-standing lvalue overload when callers pass a named container.
        Nui::val parent;

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        std::vector<int> vec{7, 8, 9};

        render(body{reference = parent}(range(vec), [](long long, auto const& element) {
            return div{}(std::to_string(element));
        }));

        ASSERT_EQ(parent["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "7");
        EXPECT_EQ(parent["children"][2]["textContent"].as<std::string>(), "9");
    }
}