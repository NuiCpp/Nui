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
            render(body{reference = parent}(
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

    TEST_F(TestRanges, RangeContextIsResetAfterAllChildrenAreRendered)
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

        vec.push_back('E');
        vec[0] = 'X';
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_TRUE(vec.rangeContext().isInDefaultState());
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
}