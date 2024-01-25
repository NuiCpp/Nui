#include "array.hpp"
#include "global_object.hpp"

#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace Nui::Tests::Engine
{
    Array::Array()
        : values_{}
        , arrayObject_{}
    {}
    Array::Array(const Array&) = default;
    Array::Array(Array&&) = default;
    Array& Array::operator=(const Array&) = default;
    Array& Array::operator=(Array&&) = default;

    Value& Array::operator[](std::size_t index)
    {
        return allValues[*values_[index]];
    }

    const Value& Array::operator[](std::size_t index) const
    {
        return allValues[*values_[index]];
    }

    std::shared_ptr<ReferenceType> Array::reference(std::size_t index) const
    {
        return values_[index];
    }

    boost::container::stable_vector<std::shared_ptr<ReferenceType>>::const_iterator Array::begin() const
    {
        return values_.begin();
    }

    boost::container::stable_vector<std::shared_ptr<ReferenceType>>::const_iterator Array::end() const
    {
        return values_.end();
    }

    void Array::erase(std::size_t index)
    {
        erase(values_.begin() + index);
    }

    void Array::erase(boost::container::stable_vector<std::shared_ptr<ReferenceType>>::const_iterator it)
    {
        if (it >= values_.end())
            throw std::out_of_range{"Iterator out of range."};
        values_.erase(it);
        updateArrayObject();
    }

    std::size_t Array::size() const
    {
        return values_.size();
    }

    bool Array::empty() const
    {
        return values_.empty();
    }

    std::shared_ptr<ReferenceType> Array::push_back(std::shared_ptr<ReferenceType> const& reference)
    {
        values_.push_back(reference);
        updateArrayObject();
        return values_.back();
    }

    void Array::clearUndefinedAndNull()
    {
        values_.erase(
            std::remove_if(
                values_.begin(),
                values_.end(),
                [](auto const& ref) {
                    return allValues[ref->uid()].type() == Value::Type::Undefined ||
                        allValues[ref->uid()].type() == Value::Type::Null;
                }),
            values_.end());
        updateArrayObject();
    }

    Object const& Array::asObject() const
    {
        return arrayObject_;
    }

    void Array::print(int indent, std::vector<ReferenceType> referenceStack) const
    {
        if (values_.empty())
        {
            std::cout << "[]";
            return;
        }
        std::cout << "[\n";
        auto begin = values_.begin();
        printIndent(indent + 1);
        allValues[(*begin)->uid()].print(indent + 1, referenceStack);
        for (auto it = ++begin; it != values_.end(); ++it)
        {
            std::cout << ",\n";
            printIndent(indent + 1);
            allValues[(*it)->uid()].print(indent + 1, referenceStack);
        }
        std::cout << "\n";
        printIndent(indent);
        std::cout << "]";
    }

    void Array::updateArrayObject()
    {
        if (arrayObject_.has("length"))
            arrayObject_["length"] = values_.size();
        else
            arrayObject_.set("length", std::make_shared<ReferenceType>(createValue(values_.size())));
    }
}