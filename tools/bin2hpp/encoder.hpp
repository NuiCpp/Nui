#pragma once

#include <iosfwd>
#include <string>

class Encoder
{
  public:
    Encoder(std::string name)
        : name_(std::move(name))
    {}

    virtual std::ostream& header(std::ostream& output) const = 0;
    virtual std::ostream& content(std::ostream& output, std::istream& input) const = 0;
    virtual std::ostream& index(std::ostream& output) const = 0;

    virtual ~Encoder() = default;
    Encoder(Encoder const&) = default;
    Encoder(Encoder&&) = default;
    Encoder& operator=(Encoder const&) = default;
    Encoder& operator=(Encoder&&) = default;

  protected:
    std::string name_;
};