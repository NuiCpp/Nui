#pragma once

#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/dom/reference.hpp>

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
        template <template <typename...> typename ContainerT, typename RangeElementType>
        void rangeTextBodyRender(Observed<ContainerT<RangeElementType>> const& observedRange, Nui::val& parent)
        {
            using Nui::Elements::div;
            using Nui::Elements::body;
            using namespace Nui::Attributes;

            render(body{reference = parent}(range(observedRange), [&observedRange](long long i, auto const& element) {
                return div{}(std::string{element} + ":" + std::to_string(i));
            }));
        }

        template <template <typename...> typename ContainerT, typename RangeElementType>
        void textBodyParityTest(Observed<ContainerT<RangeElementType>> const& observedRange, Nui::val const& parent)
        {
            EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(observedRange.size()));
            for (int i = 0; i != observedRange.size(); ++i)
            {
                EXPECT_EQ(
                    parent["children"][i]["textContent"].as<std::string>(),
                    std::string{observedRange[i]} + ":" + std::to_string(i));
            }
        }
    };

    TEST_F(TestRanges, SubscriptOperatorAssignmentUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec[2] = 'X';
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, FullAssignmentUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec = {'X', 'Y', 'Z'};
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, PushBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

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
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.pop_back();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, InsertUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};
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
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.clear();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, SwapUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec1 = {{'A', 'B', 'C', 'D'}};
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
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.resize(2);
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ResizeWithFillValueUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.resize(6, 'X');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, AssignWithFillValueUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>> vec1 = {{'A', 'B', 'C', 'D'}};
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
        Observed<std::vector<char>> vec1 = {{'A', 'B', 'C', 'D'}};
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
        Observed<std::vector<char>> vec1 = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec1, parent);
        textBodyParityTest(vec1, parent);

        vec1.assign({'X', 'Y', 'Z'});
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec1, parent);
    }

    TEST_F(TestRanges, EmplaceBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.emplace_back("X");
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, EmplaceUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.emplace(vec.cbegin() + 2, 3, 'c');
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ModificationThroughPointerUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        *vec.data() = "Y";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ModificationThroughReferenceUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.front() = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ModificationThroughIteratorUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        (*vec.begin()) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfReferenceFromBackUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.back() = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfReferenceFromAtUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec.at(2) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfIteratorFromEndUpdateView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        *(vec.end() - 1) = "X";
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestRanges, ChangeOfIteratorFromRbeginUpdateView)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};
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
        Observed<std::vector<std::string>> vec = {{"A", "B", "C", "D"}};
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
        Observed<std::deque<char>> container = {{'A', 'B', 'C', 'D'}};

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
        Observed<std::deque<char>> container = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.pop_front();
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(container, parent);
    }

    TEST_F(TestRanges, AggregatedInsertsUpdateCorrectly)
    {
        Nui::val parent;
        Observed<std::deque<char>> container = {{'A', 'B', 'C', 'D'}};

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
        Observed<std::deque<char>> container = {{'A', 'B', 'C', 'D'}};

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
        Observed<std::deque<char>> container = {{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(container, parent);
        textBodyParityTest(container, parent);

        container.insert(container.begin() + 2, 3, 'c');
        container.erase(container.begin() + 2, container.begin() + 5);
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
                    container.insert(container.begin() + 2, 3, static_cast<char>(dis(g)));
                else
                    container.insert(container.begin(), 3, static_cast<char>(dis(g)));
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
}