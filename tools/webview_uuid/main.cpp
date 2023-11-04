#include <iostream>
#include <string>
#include <filesystem>
#include <optional>
#include <vector>
#include <fstream>

struct Id
{
    std::string name;
    std::vector<std::string> idParts;
};

class IdExtractor
{
  public:
    IdExtractor(std::string data);
    std::optional<Id> extractId();

  private:
    std::optional<std::string> readNextHexSkipJunk(std::string::const_iterator& it) const;

  private:
    std::string data_;
};

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <path to WebView2.h> <path to id_header>" << std::endl;
        return 1;
    }

    std::filesystem::path input{argv[1]};
    std::filesystem::path output{argv[2]};
    std::ifstream file{input, std::ios_base::binary};

    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << input << std::endl;
        return 1;
    }

    std::string line;
    std::vector<Id> ids;
    while (std::getline(file, line))
    {
        IdExtractor extractor{line};
        const auto id = extractor.extractId();
        if (id)
            ids.push_back(*id);
    }

    std::cout << "Found " << ids.size() << " ids" << std::endl;

    std::ofstream outputFile{output, std::ios_base::binary};
    if (!outputFile.is_open())
    {
        std::cerr << "Failed to open file: " << output << std::endl;
        return 1;
    }

    outputFile << "#pragma once\n\n";
    outputFile << "#include <guiddef.h>\n\n";
    for (const auto& id : ids)
    {
        outputFile << "__CRT_UUID_DECL(" << id.name << ", ";
        for (size_t i = 0; i < id.idParts.size(); ++i)
        {
            outputFile << id.idParts[i];
            if (i != id.idParts.size() - 1)
                outputFile << ", ";
        }
        outputFile << ")\n";
    }
}

IdExtractor::IdExtractor(std::string data)
    : data_{std::move(data)}
{}

std::optional<Id> IdExtractor::extractId()
{
    const auto start = data_.find("const IID IID_");
    if (start == std::string::npos)
    {
        return std::nullopt;
    }

    const auto nameStart = start + 14;
    const auto nameEnd = data_.find(' ', nameStart);
    const auto name = data_.substr(nameStart, nameEnd - nameStart);

    const auto firstCurlyPos = data_.find('{', start);
    const auto lastCurlyPos = data_.find('}', firstCurlyPos);

    std::string::const_iterator it = data_.begin() + firstCurlyPos + 1;
    std::vector<std::string> idParts;
    for (std::optional<std::string> part; (part = readNextHexSkipJunk(it));)
        idParts.push_back(*part);

    if (idParts.size() != 11)
    {
        std::cerr << "Failed to parse id: " << name << std::endl;
        return std::nullopt;
    }

    return Id{name, idParts};
}

std::optional<std::string> IdExtractor::readNextHexSkipJunk(std::string::const_iterator& it) const
{
    if (it == data_.end() || it + 1 == data_.end())
        return std::nullopt;

    auto isHexCharacter = [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    };

    std::string part = "0x";
    if (*it != '0' || *(it + 1) != 'x')
        return std::nullopt;

    it += 2;

    while (it != data_.end() && isHexCharacter(*it))
    {
        part += *it;
        ++it;
    }

    // perform skipping:
    while (it != data_.end() && *it != '0')
        ++it;

    return part;
}