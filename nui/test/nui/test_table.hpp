#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"

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
    {};

    TEST_F(TestTable, StaticCaptionIsRendererd)
    {
        using namespace Nui::Components;
        using Nui::Elements::div;

        Nui::Observed<std::vector<TableEntry>> tableModel;

        auto const table = Table<TableEntry>{{
            .tableModel = tableModel,
            .caption = "Caption",
        }}();

        render(div{}(table));

        globalObject.print();
    }
}