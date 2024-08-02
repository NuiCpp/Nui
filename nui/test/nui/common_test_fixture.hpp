#pragma once

#include <gtest/gtest.h>

#include "engine/global_object.hpp"
#include "engine/document.hpp"

#include <nui/frontend/dom/dom.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/dom/reference.hpp>

namespace Nui::Tests
{
    class CommonTestFixture : public ::testing::Test
    {
      protected:
        CommonTestFixture()
            : preConstructionHelper_{[]() {
                Engine::resetGlobals();
                return 0;
            }()}
            , document_{}
            , dom_{}
        {}

        void TearDown() override
        {
            Nui::globalEventContext.reset();
        }

        template <typename T>
        void render(T&& factory)
        {
            dom_.setBody(std::forward<T>(factory));
        }

        static auto accumulateReferences(std::vector<Nui::val>& references)
        {
            return [&references](std::weak_ptr<Dom::BasicElement>&& weak) {
                references.push_back(weak.lock()->val());
            };
        };

        template <template <typename...> typename ContainerT, typename RangeElementType>
        void rangeTextBodyRender(Observed<ContainerT<RangeElementType>>& observedRange, Nui::val& parent)
        {
            using Nui::Elements::div;
            using Nui::Elements::body;
            using namespace Nui::Attributes;

            render(body{reference = parent}(range(observedRange), [&observedRange](long long i, auto const& element) {
                return div{}(std::string{element});
            }));
        }

        template <template <typename...> typename ContainerT, typename RangeElementType>
        std::string aggregateObservedCharList(Observed<ContainerT<RangeElementType>>& observedRange)
        {
            auto toString = [](RangeElementType elem) {
                if constexpr (std::is_same_v<RangeElementType, char>)
                    return std::string{elem};
                else if constexpr (std::is_fundamental_v<RangeElementType>)
                    return std::to_string(elem);
                else
                    return elem;
            };

            std::string sourceData;
            sourceData.reserve(observedRange.size());
            for (auto const& elem : observedRange.value())
                sourceData += toString(elem);
            return sourceData;
        }

        std::string getChildrenBodyTextConcat(Nui::val const& parent)
        {
            std::string viewReality;
            for (long long i = 0, end = parent["children"]["length"].as<long long>(); i != end; ++i)
            {
                viewReality += parent["children"][i]["textContent"].as<std::string>();
            }
            return viewReality;
        }

        template <template <typename...> typename ContainerT, typename RangeElementType>
        void textBodyParityTest(Observed<ContainerT<RangeElementType>>& observedRange, Nui::val const& parent)
        {
            ASSERT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(observedRange.size()));
            EXPECT_EQ(this->aggregateObservedCharList(observedRange), this->getChildrenBodyTextConcat(parent));
        }

        int preConstructionHelper_;
        Engine::Document document_;
        Dom::Dom dom_;
    };
}