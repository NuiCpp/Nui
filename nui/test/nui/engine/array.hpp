#pragma once

#include "value.hpp"
#include "print.hpp"
#include "object.hpp"

#include <boost/container/stable_vector.hpp>

#include <list>
#include <memory>
#include <iostream>

namespace Nui::Tests::Engine
{
    class Array
    {
      public:
        Array();
        Array(const Array&);
        Array(Array&&);
        Array& operator=(const Array&);
        Array& operator=(Array&&);

        Value& operator[](std::size_t index);
        const Value& operator[](std::size_t index) const;
        std::shared_ptr<ReferenceType> reference(std::size_t index) const;

        boost::container::stable_vector<std::shared_ptr<ReferenceType>>::const_iterator begin() const;
        boost::container::stable_vector<std::shared_ptr<ReferenceType>>::const_iterator end() const;

        std::size_t size() const;
        bool empty() const;

        std::shared_ptr<ReferenceType> push_back(Value const& value);
        std::shared_ptr<ReferenceType> push_back(std::shared_ptr<ReferenceType> const& reference);

        void clearUndefinedAndNull();

        Object const& asObject() const;

        void print(int indent = 0) const;

      private:
        void updateArrayObject();

      private:
        boost::container::stable_vector<std::shared_ptr<ReferenceType>> values_;
        Object arrayObject_;
    };
}