#pragma once

#include <gtest/gtest.h>

#include "../common_test_fixture.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/components/table.hpp>
#include <nui/frontend/dom/element.hpp>

#include <vector>
#include <string>

namespace Nui::Tests
{
    using namespace Engine;

    struct TableEntry
    {
        std::string firstName;
        std::string lastName;
        int age;
    };

    class TestTable : public CommonTestFixture
    {
      protected:
        Nui::Observed<std::vector<TableEntry>> tableModel_;
    };

    TEST_F(TestTable, TableRendersWithTableElement)
    {
        using namespace Nui::Components;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
        }}();

        render(table);

        EXPECT_EQ(Nui::val::global("document")["body"]["tagName"].as<std::string>(), "table");
    }

    TEST_F(TestTable, StaticCaptionIsRendererd)
    {
        using namespace Nui::Components;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .caption = "Caption",
        }}();

        render(table);

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "caption");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "Caption");
    }

    TEST_F(TestTable, ObservedCaptionIsRendererdAndUpdatesTheCaption)
    {
        using namespace Nui::Components;

        Nui::Observed<std::string> caption{"Caption"};

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .caption = &caption,
        }}();

        render(table);

        ASSERT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["tagName"].as<std::string>(), "caption");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "Caption");

        caption = "New Caption";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "New Caption");
    }

    TEST_F(TestTable, HeaderIsRenderedWhenAHeaderRendererIsPassed)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .headerRenderer =
                []() {
                    return Nui::Elements::tr{}(
                        Nui::Elements::th{}("First Name"),
                        Nui::Elements::th{}("Last Name"),
                        Nui::Elements::th{}("Age"));
                },
            .tableAttributes = {reference = ref},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "thead");
        ASSERT_EQ(ref["children"][0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][0]["children"][0]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][0]["children"][0]["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][0]["tagName"].as<std::string>(), "th");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][0]["textContent"].as<std::string>(), "First Name");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][1]["tagName"].as<std::string>(), "th");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][1]["textContent"].as<std::string>(), "Last Name");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][2]["tagName"].as<std::string>(), "th");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][2]["textContent"].as<std::string>(), "Age");
    }

    TEST_F(TestTable, BodyIsRendered)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        tableModel_->push_back(TableEntry{"John", "Doe", 27});
        tableModel_->push_back(TableEntry{"Jane", "Foster", 36});

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .rowRenderer =
                [](long long i, TableEntry const& entry) {
                    return Nui::Elements::tr{}(
                        Nui::Elements::td{}(entry.firstName),
                        Nui::Elements::td{}(entry.lastName),
                        Nui::Elements::td{}(entry.age));
                },
            .tableAttributes = {reference = ref},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "tbody");
        ASSERT_EQ(ref["children"][0]["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(ref["children"][0]["children"][0]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][0]["children"][0]["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][0]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][0]["textContent"].as<std::string>(), "John");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][1]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][1]["textContent"].as<std::string>(), "Doe");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][2]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][0]["children"][0]["children"][2]["textContent"].as<std::string>(), "27");
        EXPECT_EQ(ref["children"][0]["children"][1]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][0]["children"][1]["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(ref["children"][0]["children"][1]["children"][0]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][0]["children"][1]["children"][0]["textContent"].as<std::string>(), "Jane");
        EXPECT_EQ(ref["children"][0]["children"][1]["children"][1]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][0]["children"][1]["children"][1]["textContent"].as<std::string>(), "Foster");
        EXPECT_EQ(ref["children"][0]["children"][1]["children"][2]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][0]["children"][1]["children"][2]["textContent"].as<std::string>(), "36");
    }

    TEST_F(TestTable, FooterIsRendered)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .footerRenderer =
                []() {
                    return Nui::Elements::tr{}(Nui::Elements::td{}("Footer"));
                },
            .tableAttributes = {reference = ref},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(ref["children"][1]["tagName"].as<std::string>(), "tfoot");
        ASSERT_EQ(ref["children"][1]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][1]["children"][0]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][1]["children"][0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][0]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][0]["textContent"].as<std::string>(), "Footer");
    }

    TEST_F(TestTable, HeaderIsNotPresentWhenNoRenderer)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .tableAttributes = {reference = ref},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "tbody");
    }

    TEST_F(TestTable, FooterIsNotPresentWhenNoRenderer)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .tableAttributes = {reference = ref},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "tbody");
    }

    TEST_F(TestTable, HeaderAttributesAreForwarded)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .headerRenderer =
                []() {
                    return Nui::Elements::tr{}(Nui::Elements::td{}("Header"));
                },
            .tableAttributes = {reference = ref},
            .headerAttributes = {id = "header"},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "thead");
        EXPECT_EQ(ref["children"][0]["attributes"]["id"].as<std::string>(), "header");
    }

    TEST_F(TestTable, BodyAttributesAreForwarded)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .rowRenderer =
                [](long long, auto const& entry) {
                    return Nui::Elements::tr{}(
                        Nui::Elements::td{}(entry.firstName),
                        Nui::Elements::td{}(entry.lastName),
                        Nui::Elements::td{}(entry.age));
                },
            .tableAttributes = {reference = ref},
            .bodyAttributes = {id = "body"},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "tbody");
        EXPECT_EQ(ref["children"][0]["attributes"]["id"].as<std::string>(), "body");
    }

    TEST_F(TestTable, FooterAttributesAreForwarded)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .footerRenderer =
                []() {
                    return Nui::Elements::tr{}(Nui::Elements::td{}("Footer"));
                },
            .tableAttributes = {reference = ref},
            .footerAttributes = {id = "footer"},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 2);
        EXPECT_EQ(ref["children"][1]["tagName"].as<std::string>(), "tfoot");
        EXPECT_EQ(ref["children"][1]["attributes"]["id"].as<std::string>(), "footer");
    }

    TEST_F(TestTable, FullFeatureTableTest)
    {
        using namespace Nui::Components;
        using namespace Nui::Attributes;

        Nui::val ref;
        tableModel_->push_back(TableEntry{"John", "Doe", 27});

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel_,
            .caption = "Table",
            .headerRenderer =
                []() {
                    return Nui::Elements::tr{}(
                        Nui::Elements::th{}("First Name"),
                        Nui::Elements::th{}("Last Name"),
                        Nui::Elements::th{}("Age"));
                },
            .rowRenderer =
                [](long long, auto const& entry) {
                    return Nui::Elements::tr{}(
                        Nui::Elements::td{}(entry.firstName),
                        Nui::Elements::td{}(entry.lastName),
                        Nui::Elements::td{}(entry.age));
                },
            .footerRenderer =
                []() {
                    return Nui::Elements::tr{}(Nui::Elements::td{}("Footer"));
                },
            .tableAttributes = {reference = ref},
            .captionAttributes = {id = "caption"},
            .headerAttributes = {id = "header"},
            .bodyAttributes = {id = "body"},
            .footerAttributes = {id = "footer"},
        }}();

        render(table);

        ASSERT_EQ(ref["children"]["length"].as<long long>(), 4);
        EXPECT_EQ(ref["children"][0]["tagName"].as<std::string>(), "caption");
        EXPECT_EQ(ref["children"][0]["attributes"]["id"].as<std::string>(), "caption");
        EXPECT_EQ(ref["children"][0]["textContent"].as<std::string>(), "Table");
        EXPECT_EQ(ref["children"][1]["tagName"].as<std::string>(), "thead");
        EXPECT_EQ(ref["children"][1]["attributes"]["id"].as<std::string>(), "header");
        ASSERT_EQ(ref["children"][1]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][1]["children"][0]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][1]["children"][0]["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][0]["tagName"].as<std::string>(), "th");
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][0]["textContent"].as<std::string>(), "First Name");
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][1]["tagName"].as<std::string>(), "th");
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][1]["textContent"].as<std::string>(), "Last Name");
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][2]["tagName"].as<std::string>(), "th");
        EXPECT_EQ(ref["children"][1]["children"][0]["children"][2]["textContent"].as<std::string>(), "Age");
        EXPECT_EQ(ref["children"][2]["tagName"].as<std::string>(), "tbody");
        EXPECT_EQ(ref["children"][2]["attributes"]["id"].as<std::string>(), "body");
        ASSERT_EQ(ref["children"][2]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][2]["children"][0]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][2]["children"][0]["children"]["length"].as<long long>(), 3);
        EXPECT_EQ(ref["children"][2]["children"][0]["children"][0]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][2]["children"][0]["children"][0]["textContent"].as<std::string>(), "John");
        EXPECT_EQ(ref["children"][2]["children"][0]["children"][1]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][2]["children"][0]["children"][1]["textContent"].as<std::string>(), "Doe");
        EXPECT_EQ(ref["children"][2]["children"][0]["children"][2]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][2]["children"][0]["children"][2]["textContent"].as<std::string>(), "27");
        EXPECT_EQ(ref["children"][3]["tagName"].as<std::string>(), "tfoot");
        EXPECT_EQ(ref["children"][3]["attributes"]["id"].as<std::string>(), "footer");
        ASSERT_EQ(ref["children"][3]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][3]["children"][0]["tagName"].as<std::string>(), "tr");
        ASSERT_EQ(ref["children"][3]["children"][0]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(ref["children"][3]["children"][0]["children"][0]["tagName"].as<std::string>(), "td");
        EXPECT_EQ(ref["children"][3]["children"][0]["children"][0]["textContent"].as<std::string>(), "Footer");
    }
}