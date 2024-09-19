#include "raw_encoder.hpp"
#include "base64_encoder.hpp"
#include "compressed_encoder.hpp"

#include <cctype>
#include <fstream>
#include <ios>
#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cout << "Expected 5 arguments: <yes/no> <input file> <output file> <name> <encoding>, but got " << argc - 1
                  << "\n";
        std::cout << "Usage: " << argv[0] << " <yes/no> <input file> <output file> <name> <encoding>"
                  << "\n";
        return 1;
    }

    std::string enable = argv[1];
    std::string inputFile = argv[2];
    std::string outputFile = argv[3];
    std::string name = argv[4];
    std::string encoding = argv[5];

    if (enable == "no")
    {
        return 0;
    }

    std::unique_ptr<Encoder> encoder;
    if (encoding == "raw")
        encoder = std::make_unique<RawEncoder>(name);
    else if (encoding == "base64")
        encoder = std::make_unique<Base64Encoder>(name);
    else if (encoding == "gz_base64")
        encoder = std::make_unique<CompressedEncoder>(name);
    else
    {
        std::cout << "Unknown encoding: " << encoding << "\n";
        return 1;
    }

    auto outputPath = std::filesystem::path(outputFile).parent_path();
    if (!std::filesystem::exists(outputPath))
        std::filesystem::create_directories(outputPath);

    std::ifstream input(inputFile, std::ios_base::binary);
    std::ofstream output(outputFile, std::ios_base::binary);
    if (!input.is_open())
    {
        std::cout << "Could not open file \"" << inputFile << "\"\n";
        return 1;
    }
    if (!output.is_open())
    {
        std::cout << "Could not open file \"" << outputFile << "\"\n";
        return 1;
    }

    encoder->header(output);
    encoder->content(output, input);
    encoder->index(output);
}