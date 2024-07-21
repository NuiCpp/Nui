#pragma once

#include "value.hpp"
#include "print.hpp"
#include "object.hpp"

#include <boost/container/stable_vector.hpp>

#include <list>
#include <memory>
#include <iostream>
#include <vector>

namespace Nui::Tests::Engine
{
    class Array
    {
      public:
        using const_iterator = boost::container::stable_vector<std::shared_ptr<ReferenceType>>::const_iterator;
        using iterator = boost::container::stable_vector<std::shared_ptr<ReferenceType>>::iterator;

        Array();
        Array(const Array&);
        Array(Array&&);
        Array& operator=(const Array&);
        Array& operator=(Array&&);

        Value& operator[](std::size_t index);
        const Value& operator[](std::size_t index) const;
        std::shared_ptr<ReferenceType> reference(std::size_t index) const;

        const_iterator begin() const;
        const_iterator end() const;

        std::size_t size() const;
        bool empty() const;

        std::shared_ptr<ReferenceType> push_back(std::shared_ptr<ReferenceType> const& reference);

        void clearUndefinedAndNull();

        void erase(std::size_t index);
        void erase(const_iterator it);

        void insert(const_iterator it, std::shared_ptr<ReferenceType> const& reference);
        void insert(iterator it, std::shared_ptr<ReferenceType> const& reference);

        Object const& asObject() const;

        void print(int indent = 0, std::vector<ReferenceType> referenceStack = {}) const;

      private:
        void updateArrayObject();

      private:
        boost::container::stable_vector<std::shared_ptr<ReferenceType>> values_;
        Object arrayObject_;
    };
}