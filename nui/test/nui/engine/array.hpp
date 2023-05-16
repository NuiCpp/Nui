#pragma once

#include "value.hpp"
#include "print.hpp"

#include <boost/container/stable_vector.hpp>

#include <list>
#include <memory>
#include <iostream>

namespace Nui::Tests::Engine
{
    class Array
    {
      public:
        Array() = default;
        Array(const Array&) = default;
        Array(Array&&) = default;
        Array& operator=(const Array&) = default;
        Array& operator=(Array&&) = default;

        Value& operator[](std::size_t index)
        {
            return *values_[index];
        }

        const Value& operator[](std::size_t index) const
        {
            return *values_[index];
        }

        std::shared_ptr<Value> reference(std::size_t index)
        {
            return values_[index];
        }

        auto begin() const
        {
            return values_.begin();
        }

        auto end() const
        {
            return values_.end();
        }

        auto size() const
        {
            return values_.size();
        }

        bool empty() const
        {
            return values_.empty();
        }

        std::shared_ptr<Value> push_back(Value const& value)
        {
            values_.emplace_back(std::make_shared<Value>(value));
            return values_.back();
        }

        std::shared_ptr<Value> push_back(std::shared_ptr<Value> const& value)
        {
            values_.push_back(value);
            return values_.back();
        }

        void clearUndefined()
        {
            values_.erase(
                std::remove_if(
                    values_.begin(),
                    values_.end(),
                    [](auto const& value) {
                        return value->type() == Value::Type::Undefined;
                    }),
                values_.end());
        }

        void print(int indent = 0)
        {
            if (values_.empty())
            {
                std::cout << "[]";
                return;
            }
            std::cout << "[\n";
            auto begin = values_.begin();
            printIndent(indent + 1);
            (*begin)->print(indent + 1);
            for (auto it = ++begin; it != values_.end(); ++it)
            {
                std::cout << ",\n";
                printIndent(indent + 1);
                (*it)->print(indent + 1);
            }
            printIndent(indent);
            std::cout << "\n]";
        }

      private:
        boost::container::stable_vector<std::shared_ptr<Value>> values_;
    };
}