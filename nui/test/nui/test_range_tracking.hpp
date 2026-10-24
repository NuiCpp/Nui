#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

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
}
