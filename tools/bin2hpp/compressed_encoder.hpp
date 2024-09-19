#pragma once

#include "encoder.hpp"

class CompressedEncoder : public Encoder
{
  public:
    using Encoder::Encoder;

    std::ostream& header(std::ostream& output) const override;
    std::ostream& content(std::ostream& output, std::istream& input) const override;
    std::ostream& index(std::ostream& output) const override;
};