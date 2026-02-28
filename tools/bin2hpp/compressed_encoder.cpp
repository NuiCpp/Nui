#include "compressed_encoder.hpp"
#include "raw_encoder.hpp"
#include "constants.hpp"

#include <nui/base64/base64.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <iostream>
#include <sstream>

std::ostream& CompressedEncoder::header(std::ostream& output) const
{
    output <<
        R"(#pragma once

#include <nui/base64/base64.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <string_view>
#include <string>
#include <sstream>

static const std::string_view )"
           << name_ << R"(_data[] = {)";

    return output;
}

std::ostream& CompressedEncoder::content(std::ostream& output, std::istream& input) const
{
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_compressor());
    in.push(input);

    std::stringstream compressedRaw;
    boost::iostreams::copy(in, compressedRaw);

    auto encoded = Nui::base64Encode(compressedRaw.str());
    std::stringstream ss{std::move(encoded)};

    RawEncoder rawEncoder(name_);
    return rawEncoder.content(output, ss);
}

std::ostream& CompressedEncoder::index(std::ostream& output) const
{
    output << "};\n\n";
    output << "static std::string " << name_ << "()\n";
    output << "{\n";
    output << "\tstatic std::string memo;\n";
    output << "\tif(!memo.empty())\n";
    output << "\t\treturn memo;\n\n";
    output << "\tstd::string compressed;\n";
    output << "\tfor (std::size_t i = 0; i != sizeof(" << name_ << "_data) / sizeof(std::string_view)"
           << "; ++i) {\n";
    output << "\t\tcompressed += " << name_ << "_data[i];\n";
    output << "\t}\n\n";
    output << "\tcompressed = Nui::base64Decode(compressed);\n";
    output << "\tstd::stringstream compressedStream{compressed};\n";
    output << "\tboost::iostreams::filtering_streambuf<boost::iostreams::input> in;\n";
    output << "\tin.push(boost::iostreams::gzip_decompressor());\n";
    output << "\tin.push(compressedStream);\n";
    output << "\tstd::stringstream decompressed;\n";
    output << "\tboost::iostreams::copy(in, decompressed);\n";
    output << "\tmemo = std::move(decompressed).str();\n";
    output << "\treturn memo;\n";
    output << "}\n";

    return output;
}