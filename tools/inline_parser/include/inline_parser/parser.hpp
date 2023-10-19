#pragma once

#include <utility>
#include <string>
#include <cctype>

template <typename IteratorT>
class Parser
{
  public:
    enum class State
    {
        ReachedEnd,
        Success,
        Failure
    };

    Parser(IteratorT begin, IteratorT end)
        : begin_(begin)
        , end_(end)
    {}
    Parser(std::string_view str)
        : begin_(str.begin())
        , end_(str.end())
    {}
    Parser(std::string const& str)
        : begin_(str.begin())
        , end_(str.end())
    {}

    State skipWhitespace()
    {
        while (begin_ != end_ && std::isspace(*begin_))
            ++begin_;
        return begin_ == end_ ? State::ReachedEnd : State::Success;
    }

    State matchComment()
    {
        if (begin_ == end_ || *begin_ != '/')
            return State::Failure;
        ++begin_;
        if (begin_ == end_ || *begin_ != '/')
            return State::Failure;
        ++begin_;
        return State::Success;
    }

    State match(std::string_view str)
    {
        auto it = begin_;
        for (auto c : str)
        {
            if (it == end_ || *it != c)
                return State::Failure;
            ++it;
        }
        begin_ = it;
        return State::Success;
    }

    State match(char c)
    {
        if (begin_ == end_ || *begin_ != c)
            return State::Failure;
        ++begin_;
        return State::Success;
    }

    std::pair<State, std::string> match(std::function<bool(char)> const& predicate)
    {
        std::string result;
        while (begin_ != end_ && predicate(*begin_))
        {
            result += *begin_;
            ++begin_;
        }
        if (result.empty())
            return {State::Failure, result};
        else
            return {begin_ == end_ ? State::ReachedEnd : State::Success, result};
    }

    std::pair<State, std::string> matchUntil(char endMarker)
    {
        std::string result;
        while (begin_ != end_ && *begin_ != endMarker)
        {
            result += *begin_;
            ++begin_;
        }
        if (begin_ == end_)
            return {State::ReachedEnd, result};
        else
            return {State::Success, result};
    }

  private:
    IteratorT begin_;
    IteratorT end_;
};