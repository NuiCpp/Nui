#pragma once

#include "value.hpp"
#include "print.hpp"

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

        std::shared_ptr<Value> reference(std::string_view key)
        {
            if (!members_.contains(key.data()))
                members_.emplace(key.data(), std::make_shared<Value>());
            return members_.at(key.data());
        }

        std::shared_ptr<Value> emplace_back(std::string_view key, Value const& value)
        {
            if (!members_.contains(key.data()))
                members_.emplace(key.data(), std::make_shared<Value>(value));
            return members_.at(key.data());
        }

        void set(std::string_view key, std::shared_ptr<Value> value)
        {
            members_[key.data()] = std::move(value);
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

        void print(int indent = 0) const
        {
            if (members_.empty())
            {
                std::cout << "{}";
                return;
            }

            std::cout << "{\n";
            auto begin = members_.begin();
            printIndent(indent + 1);
            std::cout << "\"" << begin->first << "\": ";
            begin->second->print(indent + 1);
            for (auto it = ++begin; it != members_.end(); ++it)
            {
                std::cout << ",\n";
                printIndent(indent + 1);
                std::cout << "\"" << it->first << "\": ";
                it->second->print(indent + 1);
            }
            std::cout << "\n";
            printIndent(indent);
            std::cout << "}";
        }

      private:
        std::unordered_map<std::string, std::shared_ptr<Value>> members_;
    };
}