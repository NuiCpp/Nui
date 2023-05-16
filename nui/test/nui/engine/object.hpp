#pragma once

#include "value.hpp"

#include <unordered_map>
#include <string>
#include <memory>

namespace Nui::Tests::Engine
{
    class Object
    {
      public:
        Object() = default;
        Object(const Object&) = default;
        Object(Object&&) = default;
        Object& operator=(const Object&) = default;
        Object& operator=(Object&&) = default;

        Value& operator[](std::string_view key)
        {
            if (!members_.contains(key.data()))
                members_.emplace(key.data(), std::make_shared<Value>());
            return *members_.at(key.data());
        }

        const Value& operator[](std::string_view key) const
        {
            return *members_.at(key.data());
        }

        std::weak_ptr<Value> reference(std::string_view key)
        {
            if (!members_.contains(key.data()))
                members_.emplace(key.data(), std::make_shared<Value>());
            return members_.at(key.data());
        }

        std::weak_ptr<Value> emplace_back(std::string_view key, Value const& value)
        {
            if (!members_.contains(key.data()))
                members_.emplace(key.data(), std::make_shared<Value>(value));
            return members_.at(key.data());
        }

        bool has(std::string_view key) const
        {
            return members_.contains(key.data());
        }

        auto begin() const
        {
            return members_.begin();
        }

        auto end() const
        {
            return members_.end();
        }

        auto size() const
        {
            return members_.size();
        }

        bool empty() const
        {
            return members_.empty();
        }

        void erase(std::string_view key)
        {
            members_.erase(key.data());
        }

      private:
        std::unordered_map<std::string, std::shared_ptr<Value>> members_;
    };
}