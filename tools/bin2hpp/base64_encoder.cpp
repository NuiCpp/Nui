#include "base64_encoder.hpp"
#include "raw_encoder.hpp"
#include "constants.hpp"

#include <roar/utility/base64.hpp>

#include <iostream>
#include <sstream>

std::ostream& Base64Encoder::header(std::ostream& output) const
{
    output <<
        R"(#pragma once

#include <roar/utility/base64.hpp>

#include <string_view>
#include <string>

static const std::string_view )"
           << name_ << R"(_data[] = {)";

    return output;
}

std::ostream& Base64Encoder::content(std::ostream& output, std::istream& input) const
{
    std::stringstream readStream;
    readStream << input.rdbuf();

    auto encoded = Roar::base64Encode(std::move(readStream).str());

    std::cout << "Encoded size: " << encoded.size() << "\n";

    std::stringstream ss(std::move(encoded));

    RawEncoder rawEncoder(name_);
    return rawEncoder.content(output, ss);
}

std::ostream& Base64Encoder::index(std::ostream& output) const
{
    output << "};\n\n";
    output << "static std::string " << name_ << "()\n";
    output << "{\n";
    output << "\tstatic std::string memo;\n";
    output << "\tif(!memo.empty())\n";
    output << "\t\treturn memo;\n";
    output << "\tfor (std::size_t i = 0; i != sizeof(" << name_ << "_data) / sizeof(std::string_view)"
           << "; ++i) {\n";
    output << "\t\tmemo += " << name_ << "_data[i];\n";
    output << "\t}\n\n";
    output << "\tmemo = Roar::base64Decode(memo);\n";
    output << "\treturn memo;\n";
    output << "}\n";

    return output;
}