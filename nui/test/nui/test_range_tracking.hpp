#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/event_system/range_event_context.hpp>

#include <algorithm>
#include <deque>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace Nui::Tests
{
    using namespace Engine;

    // Several changes to one observed container between two syncs, checked row by row against the rendered children.
    class TestRangeTracking : public CommonTestFixture
    {
      protected:
        template <template <typename...> typename ContainerT>
        static Observed<ContainerT<std::string>> makeRows(int count)
        {
            Observed<ContainerT<std::string>> rows;
            for (int row = 1; row <= count; ++row)
                rows.value().push_back(std::to_string(row));
            return rows;
        }

        /// The newest row goes to the front of a page of at most pageSize rows, the oldest drops off its back.
        static void addNewestRow(Observed<std::vector<std::string>>& rows, std::size_t pageSize, int row)
        {
            const std::vector<std::string> incoming{std::to_string(row)};
            if (rows.size() == pageSize)
            {
                rows.erase(
                    rows.cbegin() + static_cast<std::ptrdiff_t>(pageSize - 1),
                    rows.cbegin() + static_cast<std::ptrdiff_t>(pageSize));
            }
            rows.insert(rows.cbegin(), incoming.begin(), incoming.end());
        }

        template <template <typename...> typename ContainerT>
        static void rowParityTest(Observed<ContainerT<std::string>> const& rows, Nui::val const& parent)
        {
            ASSERT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(rows.size()));
            for (std::size_t index = 0; index < rows.size(); ++index)
                EXPECT_EQ(parent["children"][index]["textContent"].as<std::string>(), rows.value()[index]) << index;
        }

        template <typename NodeT>
        static long subtreeHigh(NodeT const* node)
        {
            if (node == nullptr)
                return std::numeric_limits<long>::min();
            return std::max({node->interval()->high(), subtreeHigh(node->left()), subtreeHigh(node->right())});
        }
    };

    TEST_F(TestRangeTracking, FillingAnEmptyPageWithoutSyncThenEraseAtBackAndInsertAtFrontWorks)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> rows;
        rangeTextBodyRender(rows, parent);

        for (int row = 1; row <= 6; ++row)
            addNewestRow(rows, 4, row);

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, BurstOfTwoThousandRowsOnPageOfHundredWithoutSyncWorks)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> rows;
        rangeTextBodyRender(rows, parent);

        for (int row = 1; row <= 2000; ++row)
            addNewestRow(rows, 100, row);

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, BurstOfTwoThousandRowsWithSyncAfterEveryRowWorks)
    {
        Nui::val parent;
        Observed<std::vector<std::string>> rows;
        rangeTextBodyRender(rows, parent);

        for (int row = 1; row <= 2000; ++row)
        {
            addNewestRow(rows, 100, row);
            globalEventContext.executeActiveEventsImmediately();
        }
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, EraseOverTheEndOfAPendingInsertThatStartsLeftOfItWorks)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(26);
        rangeTextBodyRender(rows, parent);
        rowParityTest(rows, parent);

        const std::vector<std::string> incoming{"a", "b", "c", "d"};
        rows.insert(rows.cbegin() + 3, incoming.begin(), incoming.end());
        rows.erase(rows.cbegin() + 5, rows.cbegin() + 10);

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, EraseOfOnlyPendingRowsThenInsertWorks)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        rangeTextBodyRender(rows, parent);

        rows.push_back("a");
        rows.push_back("b");
        rows.erase(rows.cbegin() + 4, rows.cend());
        rows.insert(rows.cbegin(), "c");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, InsertAfterPendingModificationRendersTheModifiedRow)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(10);
        rangeTextBodyRender(rows, parent);

        rows[5] = "modified";
        rows.insert(rows.cbegin(), "new");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, PushFrontAfterPendingModificationRendersTheModifiedRow)
    {
        Nui::val parent;
        auto rows = makeRows<std::deque>(10);
        rangeTextBodyRender(rows, parent);

        rows[2] = "modified";
        rows[7] = "modified";
        rows.push_front("new");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, ShiftedPendingInsertsKeepTheMaxOfEverySubtree)
    {
        RangeEventContext context;
        for (long position : {10L, 20L, 30L, 40L, 50L, 60L, 70L})
            context.insertModificationRange(position, position, RangeOperationType::Insert);
        context.insertModificationRange(35L, 35L, RangeOperationType::Insert);

        for (auto it = context.begin(); it != context.end(); ++it)
            EXPECT_GE(it.node()->max(), subtreeHigh(it.node())) << *it;
    }

    TEST_F(TestRangeTracking, RandomPendingInsertsKeepTheMaxOfEverySubtree)
    {
        std::mt19937 generator{7};
        for (int round = 0; round != 100; ++round)
        {
            RangeEventContext context;
            long size = 20;
            for (int insert = 0; insert != 30; ++insert)
            {
                const auto low = std::uniform_int_distribution<long>{0, size}(generator);
                const auto count = std::uniform_int_distribution<long>{1, 3}(generator);
                context.insertModificationRange(low, low + count - 1, RangeOperationType::Insert);
                size += count;
                for (auto it = context.begin(); it != context.end(); ++it)
                    ASSERT_GE(it.node()->max(), subtreeHigh(it.node())) << *it << " in round " << round;
            }
        }
    }

    TEST_F(TestRangeTracking, InsertInsideAPendingInsertRendersEachRowOnce)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        rangeTextBodyRender(rows, parent);

        const std::vector<std::string> first{"a", "b", "c"};
        rows.insert(rows.cbegin() + 1, first.begin(), first.end());
        rows.insert(rows.cbegin() + 2, "d");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, InsertRightAfterAPendingInsertRendersEachRowOnce)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        rangeTextBodyRender(rows, parent);

        const std::vector<std::string> first{"a", "b"};
        rows.insert(rows.cbegin() + 1, first.begin(), first.end());
        rows.insert(rows.cbegin() + 3, "c");
        rows.insert(rows.cbegin() + 6, "d");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, LongerInsertAtThePositionOfAPendingInsertRendersEachRowOnce)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(10);
        rangeTextBodyRender(rows, parent);

        rows.insert(rows.cbegin() + 5, "x");
        rows.insert(rows.cbegin(), "a");
        const std::vector<std::string> second{"b", "c"};
        rows.insert(rows.cbegin(), second.begin(), second.end());

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, RandomPendingInsertsTrackEveryNewRowOnce)
    {
        std::mt19937 generator{11};
        for (int round = 0; round != 100; ++round)
        {
            RangeEventContext context;
            // true for the rows that are pending inserts.
            std::vector<bool> pending(20, false);
            for (int insert = 0; insert != 30; ++insert)
            {
                const auto low = std::uniform_int_distribution<long>{0, static_cast<long>(pending.size())}(generator);
                const auto count = std::uniform_int_distribution<long>{1, 3}(generator);
                context.insertModificationRange(low, low + count - 1, RangeOperationType::Insert);
                pending.insert(pending.begin() + low, static_cast<std::size_t>(count), true);

                std::vector<int> tracked(pending.size(), 0);
                for (auto const& range : context)
                {
                    ASSERT_LE(0, range.low()) << range << " in round " << round;
                    ASSERT_LT(range.high(), static_cast<long>(pending.size())) << range << " in round " << round;
                    for (auto row = range.low(); row <= range.high(); ++row)
                        ++tracked[static_cast<std::size_t>(row)];
                }
                for (std::size_t row = 0; row != pending.size(); ++row)
                    ASSERT_EQ(tracked[row], pending[row] ? 1 : 0) << "row " << row << " in round " << round;
            }
        }
    }

    TEST_F(TestRangeTracking, ShrinkingResizeAfterPendingPushBacksStaysInBounds)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(2);
        rangeTextBodyRender(rows, parent);

        for (auto const* text : {"a", "b", "c"})
            rows.push_back(text);
        rows.resize(2);

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, ShrinkingResizeWithFillValueAfterPendingPushBacksStaysInBounds)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(2);
        rangeTextBodyRender(rows, parent);

        for (auto const* text : {"a", "b", "c"})
            rows.push_back(text);
        rows.resize(3, "fill");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

#ifndef NDEBUG
    TEST_F(TestRangeTracking, ChangeThatTheTrackingCannotTakeIsReported)
    {
        auto rows = makeRows<std::vector>(4);
        auto context = std::make_shared<RangeEventContext>();
        rows.attachReaderContext(context);

        // Without the registered after effect, a flush no longer resets the tracked ranges.
        globalEventContext.reset();
        rows.push_back("a");

        EXPECT_THROW(rows[0] = "b", std::runtime_error);
    }
#endif

    // Changes through value() are not observed: the tracked change that follows does not fit the rendered rows.
    TEST_F(TestRangeTracking, ModificationOfARowThatIsNotRenderedRendersTheRangeAnew)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        rangeTextBodyRender(rows, parent);

        rows.value().push_back("untracked");
        rows[4] = "modified";

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, EraseOfARowThatIsNotRenderedRendersTheRangeAnew)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        rangeTextBodyRender(rows, parent);

        rows.value().push_back("untracked");
        rows.erase(rows.cbegin() + 4);

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, InsertOfARowThatIsGoneRendersTheRangeAnew)
    {
        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        rangeTextBodyRender(rows, parent);

        rows.push_back("a");
        rows.value().pop_back();

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent);
    }

    TEST_F(TestRangeTracking, InsertAtTheEndOfARangeWithPrefixAndPostfixKeepsTheRenderedRows)
    {
        using Nui::Elements::body;
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val parent;
        auto rows = makeRows<std::vector>(4);
        render(body{reference = parent}(
            range(rows).before(div{}("prefix")).after(div{}("postfix1"), div{}("postfix2")),
            [](long long, auto const& row) {
                return div{}(row);
            }));
        // Rendering the range anew would replace the marked children.
        for (int child = 1; child <= 4; ++child)
            parent["children"][child].set("marker", Nui::val{true});

        rows.push_back("a");
        globalEventContext.executeActiveEventsImmediately();

        ASSERT_EQ(parent["children"]["length"].as<long long>(), 8);
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "prefix");
        for (int row = 0; row != 5; ++row)
            EXPECT_EQ(parent["children"][row + 1]["textContent"].as<std::string>(), rows.value()[row]) << row;
        EXPECT_EQ(parent["children"][6]["textContent"].as<std::string>(), "postfix1");
        EXPECT_EQ(parent["children"][7]["textContent"].as<std::string>(), "postfix2");
        for (int child = 1; child <= 4; ++child)
            EXPECT_TRUE(parent["children"][child].hasOwnProperty("marker")) << child;
    }

    TEST_F(TestRangeTracking, StringEraseOfACountInTheMiddleUpdatesView)
    {
        Nui::val parent;
        Observed<std::string> text{"ABCDEFGH"};
        rangeTextBodyRender(text, parent);

        text.erase(2, 3);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(text, parent);
    }

    TEST_F(TestRangeTracking, StringEraseOfACountPastTheEndUpdatesView)
    {
        Nui::val parent;
        Observed<std::string> text{"ABCDEFGH"};
        rangeTextBodyRender(text, parent);

        text.erase(5, 10);
        text.erase(5, 2);

        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(text, parent);
    }

    TEST_F(TestRangeTracking, InsertAfterPendingEraseWorksInARangeAttachedInBetween)
    {
        using Nui::Elements::body;
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val parent1;
        Nui::val parent2;
        std::weak_ptr<Dom::BasicElement> host;
        auto rows = makeRows<std::vector>(4);
        auto renderer = [](long long, auto const& row) {
            return div{}(row);
        };
        render(body{}(div{reference = parent1}(range(rows), renderer), div{reference = host}()));

        rows.erase(rows.cbegin());
        std::static_pointer_cast<Dom::Element>(host.lock())->appendElement(
            div{reference = parent2}(range(rows), renderer));
        rows.push_back("new");

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent1);
        rowParityTest(rows, parent2);
    }

    TEST_F(TestRangeTracking, EraseAfterPendingModificationWorksInARangeAttachedInBetween)
    {
        using Nui::Elements::body;
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val parent1;
        Nui::val parent2;
        std::weak_ptr<Dom::BasicElement> host;
        auto rows = makeRows<std::vector>(6);
        auto renderer = [](long long, auto const& row) {
            return div{}(row);
        };
        render(body{}(div{reference = parent1}(range(rows), renderer), div{reference = host}()));

        rows[1] = "modified";
        std::static_pointer_cast<Dom::Element>(host.lock())->appendElement(
            div{reference = parent2}(range(rows), renderer));
        rows.erase(rows.cbegin() + 3);

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent1);
        rowParityTest(rows, parent2);
    }

    TEST_F(TestRangeTracking, ModificationThroughAReferenceAfterPendingInsertWorksInARangeAttachedInBetween)
    {
        using Nui::Elements::body;
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val parent1;
        Nui::val parent2;
        std::weak_ptr<Dom::BasicElement> host;
        auto rows = makeRows<std::vector>(4);
        auto renderer = [](long long, auto const& row) {
            return div{}(row);
        };
        render(body{}(div{reference = parent1}(range(rows), renderer), div{reference = host}()));

        rows.push_back("new");
        std::static_pointer_cast<Dom::Element>(host.lock())->appendElement(
            div{reference = parent2}(range(rows), renderer));
        // get() tracks the modification before the row changes.
        rows[1].get() = "modified";

        globalEventContext.executeActiveEventsImmediately();
        rowParityTest(rows, parent1);
        rowParityTest(rows, parent2);
    }

    TEST_F(TestRangeTracking, RandomChangesBetweenSyncsRenderEveryRow)
    {
        using Nui::Elements::body;
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val parent;
        std::weak_ptr<Dom::BasicElement> weakHost;
        // Ranges attached during a round, a deque keeps the references stable.
        std::deque<Nui::val> attachedParents;
        // The mocked DOM knows the length of the children only after the first child was added.
        auto rows = makeRows<std::vector>(1);
        auto renderer = [](long long, auto const& row) {
            return div{}(row);
        };
        render(body{}(div{reference = parent}(range(rows), renderer), div{reference = weakHost}()));
        auto host = std::static_pointer_cast<Dom::Element>(weakHost.lock());

        std::mt19937 generator{3};
        const auto pick = [&generator](long low, long high) {
            return std::uniform_int_distribution<long>{low, high}(generator);
        };
        const auto rendered = [](Nui::val const& rangeParent) {
            std::string texts;
            for (long long i = 0, end = rangeParent["children"]["length"].as<long long>(); i != end; ++i)
                texts += rangeParent["children"][i]["textContent"].as<std::string>() + "|";
            return texts;
        };
        const auto syncAndCompare = [&](int round, int change) {
            globalEventContext.executeActiveEventsImmediately();
            std::string expected;
            for (auto const& row : rows.value())
                expected += row + "|";
            EXPECT_EQ(rendered(parent), expected) << "round " << round << ", change " << change;
            for (auto const& attached : attachedParents)
                EXPECT_EQ(rendered(attached), expected) << "attached range, round " << round << ", change " << change;
        };

        int next = 0;
        for (int round = 0; round != 300 && !HasFailure(); ++round)
        {
            std::vector<std::string> initial;
            for (auto count = pick(0, 8); count != 0; --count)
                initial.push_back(std::to_string(next++));
            rows = std::move(initial);
            host->clearChildren();
            attachedParents.clear();
            syncAndCompare(round, -1);

            for (int change = 0; change != 40 && !HasFailure(); ++change)
            {
                const auto size = static_cast<long>(rows.size());
                switch (pick(0, 9))
                {
                    case 0:
                    {
                        std::vector<std::string> incoming;
                        for (auto count = pick(1, 3); count != 0; --count)
                            incoming.push_back(std::to_string(next++));
                        rows.insert(rows.cbegin() + pick(0, size), incoming.begin(), incoming.end());
                        break;
                    }
                    case 1:
                        rows.insert(rows.cbegin(), std::to_string(next++));
                        break;
                    case 2:
                        rows.push_back(std::to_string(next++));
                        break;
                    case 3:
                        if (size != 0)
                        {
                            const auto low = pick(0, size - 1);
                            rows.erase(rows.cbegin() + low, rows.cbegin() + std::min(size, low + pick(1, 3)));
                        }
                        break;
                    case 4:
                        rows.pop_back();
                        break;
                    case 5:
                        if (size != 0)
                            rows[static_cast<std::size_t>(pick(0, size - 1))] = std::to_string(next++);
                        break;
                    case 6:
                        if (size != 0)
                            rows[static_cast<std::size_t>(pick(0, size - 1))].get() = std::to_string(next++);
                        break;
                    case 7:
                        rows.resize(static_cast<std::size_t>(std::max(0L, size + pick(-3, 3))));
                        break;
                    case 8:
                        if (size != 0 && attachedParents.size() < 2)
                        {
                            attachedParents.emplace_back();
                            host->appendElement(div{reference = attachedParents.back()}(range(rows), renderer));
                        }
                        break;
                    default:
                        syncAndCompare(round, change);
                        break;
                }
            }
            syncAndCompare(round, 40);
        }
    }
}
