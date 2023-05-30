#pragma once

#include "value.hpp"
#include "reference_type.hpp"
#include "print.hpp"

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

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

        Value& operator[](std::string_view key);
        const Value& operator[](std::string_view key) const;
        std::shared_ptr<ReferenceType> reference(std::string_view key) const;
        template <typename... ValueCtorArgs>
        std::shared_ptr<ReferenceType> emplace(std::string_view key, ValueCtorArgs&&... value)
        {
            return members_[std::string{key}] =
                       std::make_shared<ReferenceType>(createValue(std::forward<ValueCtorArgs>(value)...));
        }
        void set(std::string_view key, std::shared_ptr<ReferenceType> const& value);
        bool has(std::string_view key) const;
        std::unordered_map<std::string, std::shared_ptr<ReferenceType>>::const_iterator begin() const;
        std::unordered_map<std::string, std::shared_ptr<ReferenceType>>::const_iterator end() const;
        std::size_t size() const;
        bool empty() const;
        void erase(std::string_view key);
        void print(int indent = 0, std::vector<ReferenceType> referenceStack = {}) const;

      private:
        std::unordered_map<std::string, std::shared_ptr<ReferenceType>> members_;
    };
}