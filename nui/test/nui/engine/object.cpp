#include "object.hpp"

#include "global_object.hpp"

#include <iostream>

namespace Nui::Tests::Engine
{
    Value& Object::operator[](std::string_view key)
    {
        if (!members_.contains(key.data()))
            members_.emplace(key.data(), std::make_shared<ReferenceType>(createValue(key)));
        return allValues[*members_.at(key.data())];
    }

    const Value& Object::operator[](std::string_view key) const
    {
        if (!members_.contains(key.data()))
            throw std::runtime_error{"operator[]: invalid key"};
        return allValues.at(*members_.at(key.data()));
    }

    std::shared_ptr<ReferenceType> Object::reference(std::string_view key) const
    {
        using namespace std::string_literals;
        if (!members_.contains(key.data()))
            throw std::runtime_error{"reference: invalid key '"s + std::string{key} + "'"};
        return members_.at(key.data());
    }

    void Object::set(std::string_view key, std::shared_ptr<ReferenceType> const& value)
    {
        members_[key.data()] = value;
    }

    bool Object::has(std::string_view key) const
    {
        return members_.contains(key.data());
    }

    std::unordered_map<std::string, std::shared_ptr<ReferenceType>>::const_iterator Object::begin() const
    {
        return members_.begin();
    }

    std::unordered_map<std::string, std::shared_ptr<ReferenceType>>::const_iterator Object::end() const
    {
        return members_.end();
    }

    std::size_t Object::size() const
    {
        return members_.size();
    }

    bool Object::empty() const
    {
        return members_.empty();
    }

    void Object::erase(std::string_view key)
    {
        members_.erase(key.data());
    }

    void Object::print(int indent, std::vector<ReferenceType> referenceStack) const
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
        allValues[*begin->second].print(indent + 1, referenceStack);
        for (auto it = ++begin; it != members_.end(); ++it)
        {
            std::cout << ",\n";
            printIndent(indent + 1);
            std::cout << "\"" << it->first << "\": ";
            allValues[*it->second].print(indent + 1, referenceStack);
        }
        std::cout << ",\n";
        printIndent(indent + 1);
        if (referenceStack.empty())
            std::cout << "\"__instanceCounter\": ???\n";
        else
            std::cout << "\"__instanceCounter\": " << static_cast<long long>(referenceStack.back()) << "\n";
        printIndent(indent);
        std::cout << "}";
    }

}