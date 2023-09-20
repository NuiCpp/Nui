#pragma once

#pragma once

#include <nui/data_structures/selectables_registry.hpp>

#include <gtest/gtest.h>

#include <random>
#include <string>
#include <unordered_map>
#include <set>

namespace Nui::Tests
{
    struct BasicItem
    {
        int timesMoved{0};
        int timesCopied{0};
        std::string data = "testData";

        BasicItem(std::string data)
            : data(std::move(data))
        {}
        BasicItem() = default;
        BasicItem(const BasicItem& other)
            : timesMoved(other.timesMoved)
            , timesCopied(other.timesCopied + 1)
            , data(other.data)
        {}
        BasicItem(BasicItem&& other)
            : timesMoved(other.timesMoved + 1)
            , timesCopied(other.timesCopied)
            , data(std::move(other.data))
        {}
        BasicItem& operator=(const BasicItem& other)
        {
            timesMoved = other.timesMoved;
            timesCopied = other.timesCopied + 1;
            data = other.data;
            return *this;
        }
        BasicItem& operator=(BasicItem&& other)
        {
            timesMoved = other.timesMoved + 1;
            timesCopied = other.timesCopied;
            data = std::move(other.data);
            return *this;
        }
        ~BasicItem() = default;
    };

    class TestSelectablesRegistry : public ::testing::Test
    {
      public:
        TestSelectablesRegistry()
            : registry{}
            , engine{std::random_device{}()}
        {}

      protected:
        SelectablesRegistry<BasicItem> registry;
        std::mt19937 engine;
    };

    TEST_F(TestSelectablesRegistry, RegistryIsEmptyInitially)
    {
        EXPECT_TRUE(registry.empty());
        EXPECT_EQ(registry.size(), 0);
    }

    TEST_F(TestSelectablesRegistry, CanAppendLValue)
    {
        BasicItem item;
        registry.append(item);
        EXPECT_EQ(registry.size(), 1);
    }

    TEST_F(TestSelectablesRegistry, CanAppendRValue)
    {
        registry.append(BasicItem{});
        EXPECT_EQ(registry.size(), 1);
    }

    TEST_F(TestSelectablesRegistry, CanAppendMultipleItems)
    {
        registry.append(BasicItem{});
        registry.append(BasicItem{});
        registry.append(BasicItem{});
        EXPECT_EQ(registry.size(), 3);
    }

    TEST_F(TestSelectablesRegistry, LValueAppendReturnsUniqueId)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            BasicItem item;
            ids.insert(registry.append(item));
        }
        EXPECT_EQ(ids.size(), idCount);
    }

    TEST_F(TestSelectablesRegistry, RValueAppendReturnsUniqueId)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.append(BasicItem{}));
        }
        EXPECT_EQ(ids.size(), idCount);
    }

    TEST_F(TestSelectablesRegistry, CanEmplaceItem)
    {
        registry.emplace("testData");
        EXPECT_EQ(registry.size(), 1);
    }

    TEST_F(TestSelectablesRegistry, EmplaceReturnsUniqueId)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace("testData"));
        }
        EXPECT_EQ(ids.size(), idCount);
    }

    TEST_F(TestSelectablesRegistry, CanRemoveItem)
    {
        auto id = registry.emplace("testData");
        registry.erase(id);
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanRemoveMultipleItems)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace("testData"));
        }
        for (auto id : ids)
        {
            registry.erase(id);
        }
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, IdsAreStillUniqueWhenRemovingItems)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace("testData"));
        }
        // delete every second item
        for (auto iter = ids.begin(); iter != ids.end(); ++iter)
        {
            registry.erase(*iter);
            ++iter;
            if (iter == ids.end())
                break;
        }

        std::set<decltype(registry)::IdType> newIds;
        for (int i = 0; i != idCount; ++i)
        {
            newIds.insert(registry.emplace("testData"));
        }
        EXPECT_EQ(newIds.size(), idCount);
    }

    TEST_F(TestSelectablesRegistry, IdsAreStillUniqueWhenRemovingItemsRandomly)
    {
        constexpr auto idCount = 1000;

        std::vector<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.push_back(registry.emplace("testData"));
        }

        ASSERT_EQ(ids.size(), registry.size());

        std::shuffle(ids.begin(), ids.end(), engine);
        for (std::size_t i = 0; i < static_cast<std::size_t>(ids.size() / 1.3); ++i)
        {
            registry.erase(ids[i]);
        }

        std::set<decltype(registry)::IdType> newIds;
        for (int i = 0; i != idCount; ++i)
        {
            newIds.insert(registry.emplace("testData"));
        }
        EXPECT_EQ(newIds.size(), idCount);
    }

    TEST_F(TestSelectablesRegistry, CanPopItem)
    {
        auto id = registry.emplace("XXX");
        auto item = registry.pop(id);

        EXPECT_EQ(item->data, "XXX");
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanPopMultipleItems)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace("testData"));
        }
        for (auto id : ids)
        {
            registry.pop(id);
        }
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanGetItemById)
    {
        auto id = registry.emplace("XXX");
        auto item = registry.get(id);

        EXPECT_EQ(item->data, "XXX");
        EXPECT_EQ(registry.size(), 1);
        EXPECT_FALSE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanGetMultipleItemsById)
    {
        constexpr auto idCount = 1000;

        std::unordered_map<decltype(registry)::IdType, std::string> assoc{};
        for (int i = 0; i != idCount; ++i)
        {
            assoc[registry.emplace(std::to_string(i))] = std::to_string(i);
        }
        for (auto const& [id, data] : assoc)
        {
            auto item = registry.get(id);
            ASSERT_EQ(item->data, data);
        }
        EXPECT_EQ(registry.size(), idCount);
        EXPECT_FALSE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanGetConstItem)
    {
        auto id = registry.emplace("XXX");
        [id](auto const& registry) {
            auto const& item = registry.get(id);

            EXPECT_EQ(item->data, "XXX");
            EXPECT_EQ(registry.size(), 1);
            EXPECT_FALSE(registry.empty());
        }(registry);
    }

    TEST_F(TestSelectablesRegistry, CanGetItemBySubscriptOperator)
    {
        registry.emplace("XXX");
        auto item = registry[0];

        EXPECT_EQ(item.data, "XXX");
    }

    TEST_F(TestSelectablesRegistry, CanGetMultipleItemsBySubscriptOperator)
    {
        constexpr auto idCount = 1000;

        std::unordered_map<decltype(registry)::IdType, std::string> assoc{};
        for (int i = 0; i != idCount; ++i)
        {
            assoc[registry.emplace(std::to_string(i))] = std::to_string(i);
        }
        for (auto const& [id, data] : assoc)
        {
            auto item = registry[id];
            ASSERT_EQ(item.data, data);
        }
        EXPECT_EQ(registry.size(), idCount);
        EXPECT_FALSE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanGetConstItemBySubscriptOperator)
    {
        registry.emplace("XXX");
        [](auto const& registry) {
            auto const& item = registry[0];

            EXPECT_EQ(item.data, "XXX");
        }(registry);
    }

    TEST_F(TestSelectablesRegistry, CanGetItemByIterator)
    {
        registry.emplace("XXX");
        auto item = *registry.begin();

        EXPECT_EQ(item.data, "XXX");
    }

    TEST_F(TestSelectablesRegistry, CanIterateRegistry)
    {
        constexpr auto idCount = 1000;

        for (int i = 0; i != idCount; ++i)
        {
            registry.emplace(std::to_string(i));
        }

        for (std::size_t i = 0; auto const& item : registry)
        {
            ASSERT_EQ(item.data, std::to_string(i));
            ++i;
        }
    }

    TEST_F(TestSelectablesRegistry, CanIterateConstRegistry)
    {
        constexpr auto idCount = 1000;

        for (int i = 0; i != idCount; ++i)
        {
            registry.emplace(std::to_string(i));
        }

        [](auto const& registry) {
            for (std::size_t i = 0; auto const& item : registry)
            {
                ASSERT_EQ(item.data, std::to_string(i));
                ++i;
            }
        }(registry);
    }

    TEST_F(TestSelectablesRegistry, CanSelectItem)
    {
        auto id = registry.emplace("XXX");
        auto result = registry.select(id);

        EXPECT_TRUE(result.found);
        EXPECT_FALSE(result.alreadySelected);
        ASSERT_NE(result.item, nullptr);
        // pointer to optional
        EXPECT_EQ((*result.item)->data, "XXX");

        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, SelectedItemIsRemovedFromIteration)
    {
        auto id = registry.emplace("XXX");
        registry.select(id);

        EXPECT_EQ(registry.begin(), registry.end());
    }

    TEST_F(TestSelectablesRegistry, CanSelectMultipleItems)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace(std::to_string(i)));
        }
        for (auto id : ids)
        {
            auto result = registry.select(id);
            ASSERT_TRUE(result.found);
            ASSERT_FALSE(result.alreadySelected);
            ASSERT_NE(result.item, nullptr);
            ASSERT_EQ((*result.item)->data, std::to_string(id));
        }
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, AllSelectedItemsAreRemovedFromIteration)
    {
        constexpr auto idCount = 100;

        std::vector<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            auto id = registry.emplace("?");
            registry[id].data = std::to_string(id);
            ids.push_back(id);
        }

        std::shuffle(ids.begin(), ids.end(), engine);
        std::size_t i = 0;
        for (; i != ids.size() / 2; ++i)
        {
            registry.select(ids[i]);
        }

        std::set<decltype(registry)::IdType> remainingIds;
        for (; i != ids.size(); ++i)
        {
            const auto id = ids[i];
            remainingIds.insert(id);
            auto item = registry.get(id);
            ASSERT_EQ(item->data, std::to_string(id));
        }

        for (auto const& item : registry)
        {
            ASSERT_NE(remainingIds.find(std::stoull(item.data)), remainingIds.end());
        }

        EXPECT_EQ(registry.size(), remainingIds.size());
        EXPECT_FALSE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanDeselectItem)
    {
        auto id = registry.emplace("XXX");
        registry.select(id);
        bool wasReinserted = registry.deselect(id, [](auto const&) {
            return true;
        });

        EXPECT_TRUE(wasReinserted);
    }

    TEST_F(TestSelectablesRegistry, CanDeselectMultipleItems)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace(std::to_string(i)));
        }
        for (auto id : ids)
        {
            registry.select(id);
        }
        for (auto id : ids)
        {
            bool wasReinserted = registry.deselect(id, [](auto const&) {
                return true;
            });
            ASSERT_TRUE(wasReinserted);
        }
    }

    TEST_F(TestSelectablesRegistry, DeselectedItemsReappearInIteration)
    {
        auto id = registry.emplace("XXX");
        registry.select(id);
        registry.deselect(id, [](auto const&) {
            return true;
        });

        ASSERT_NE(registry.begin(), registry.end());
        EXPECT_EQ(registry.begin()->data, "XXX");
    }

    TEST_F(TestSelectablesRegistry, DeselectCallbackIsCalledWithItem)
    {
        auto id = registry.emplace("XXX");
        registry.select(id);
        registry.deselect(id, [&](auto const& item) {
            EXPECT_EQ(item.item->data, "XXX");
            return true;
        });
    }

    TEST_F(TestSelectablesRegistry, CorrectItemIsDeselected)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace(std::to_string(i)));
        }
        for (auto id : ids)
        {
            registry.select(id);
        }
        for (auto id : ids)
        {
            bool wasReinserted = registry.deselect(id, [&](auto const& item) {
                EXPECT_EQ(item.item->data, std::to_string(id));
                if (::testing::Test::HasFailure())
                    throw std::runtime_error{"Test failed"};
                return true;
            });
            ASSERT_TRUE(wasReinserted);
        }
    }

    TEST_F(TestSelectablesRegistry, DeselctedItemIsNotReinsertedWhenCallbackReturnsFalse)
    {
        auto id = registry.emplace("XXX");
        registry.select(id);
        registry.deselect(id, [](auto const&) {
            return false;
        });

        EXPECT_EQ(registry.begin(), registry.end());
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanDeselectMultipleItemsWithCallback)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace(std::to_string(i)));
        }
        for (auto id : ids)
        {
            registry.select(id);
        }
        for (auto id : ids)
        {
            bool wasReinserted = registry.deselect(id, [](auto const&) {
                return false;
            });
            ASSERT_FALSE(wasReinserted);
        }
        EXPECT_EQ(registry.size(), 0);
        EXPECT_TRUE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, CanDeselectAllItems)
    {
        constexpr auto idCount = 1000;

        std::set<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            ids.insert(registry.emplace(std::to_string(i)));
        }
        for (auto id : ids)
        {
            registry.select(id);
        }
        registry.deselectAll([](auto const&) {
            return true;
        });
        EXPECT_EQ(registry.size(), idCount);
        EXPECT_FALSE(registry.empty());
    }

    TEST_F(TestSelectablesRegistry, RawIteratorsIterateEvenSelectedItems)
    {
        constexpr auto idCount = 100;

        std::vector<decltype(registry)::IdType> ids;
        for (int i = 0; i != idCount; ++i)
        {
            auto id = registry.emplace("?");
            registry[id].data = std::to_string(id);
            ids.push_back(id);
        }

        std::shuffle(ids.begin(), ids.end(), engine);
        for (std::size_t i = 0; i != ids.size() / 2; ++i)
        {
            registry.select(ids[i]);
        }

        EXPECT_EQ(std::distance(registry.rawBegin(), registry.rawEnd()), idCount);
    }
}