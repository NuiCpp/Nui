#include <inline_parser/section_cache.hpp>

#include <roar/utility/base64.hpp>

#include <fstream>

namespace
{
    bool isStyleSection(std::string const& type)
    {
        return type == "css" || type == "scss" || type == "sass" || type == "less";
    }
    bool isScriptSection(std::string const& type)
    {
        return type == "js" || type == "ts" || type == "coffee";
    }
}

void SectionCache::addSection(Section const& section)
{
    std::string key = section.type + "." + section.name;
    coldSections.erase(key);
    sections[key] = section;
}

std::optional<Section> SectionCache::findSection(std::string const& type, std::string const& name) const
{
    std::string key = type + "." + name;

    auto it = sections.find(key);
    if (it != sections.end())
        return it->second;
    else
        return std::nullopt;
}

void SectionCache::fromFile(std::filesystem::path const& path)
{
    std::ifstream reader{path, std::ios_base::binary};
    if (!reader.is_open())
        return;

    nlohmann::json j = nlohmann::json::parse(reader);

    *this = j.get<SectionCache>();

    for (auto const& [key, section] : sections)
        coldSections.insert(key);
}

void SectionCache::toFile(std::filesystem::path const& path) const
{
    nlohmann::json j = *this;
    std::ofstream writer{path, std::ios_base::binary};
    writer << j.dump(4);
}

void SectionCache::removeColdSections()
{
    for (auto const& key : coldSections)
        sections.erase(key);
    coldSections.clear();
}

std::unordered_map<std::string, Section>::const_iterator SectionCache::begin() const
{
    return sections.begin();
}
std::unordered_map<std::string, Section>::const_iterator SectionCache::end() const
{
    return sections.end();
}

void SectionCache::produceToDirectory(
    std::filesystem::path const& basePath,
    std::filesystem::path const& path,
    bool emitComments) const
{
    const auto fullPath = basePath / path;

    // Always create them! For cmake technical reasons, even if they are empty.
    std::ofstream scriptImportsWriter{fullPath / "inline_imports.js", std::ios_base::binary};
    std::ofstream styleImportsWriter{fullPath / "inline_imports.css", std::ios_base::binary};

    for (auto const& [key, section] : sections)
    {
        const auto sectionSubPath = std::filesystem::path{section.type} / (section.name + "." + section.type);
        const auto sectionPath = fullPath / sectionSubPath;
        std::filesystem::create_directories(sectionPath.parent_path());
        std::ofstream writer{sectionPath, std::ios_base::binary};
        if (emitComments)
        {
            writer << "/* This file was automatically generated by the nui inline parser. */\n";
            writer << "/* Do not edit this file, your changes will be overwritten. */\n\n";
        }
        writer.write(section.content.data(), section.content.size());

        if (isStyleSection(section.type))
        {
            styleImportsWriter << "@import \"./" << sectionSubPath.generic_string() << "\";\n";
        }
        else if (isScriptSection(section.type))
        {
            scriptImportsWriter << "import './" << sectionSubPath.generic_string() << "';\n";
        }
        else
        {
            scriptImportsWriter << "// Unknown section type: " << section.type << "\n";
        }
    }
}

std::unordered_map<std::string, Section>::const_iterator
SectionCache::find(std::string const& type, std::string const& name) const
{
    std::string key = type + "." + name;
    return sections.find(key);
}

std::set<std::string>::const_iterator SectionCache::coldBegin() const
{
    return coldSections.begin();
}
std::set<std::string>::const_iterator SectionCache::coldEnd() const
{
    return coldSections.end();
}

void to_json(nlohmann::json& j, SectionCache const& sc)
{
    j = nlohmann::json::object();
    for (auto const& [key, section] : sc.sections)
    {
        nlohmann::json sectionJson;
        sectionJson["type"] = section.type;
        sectionJson["name"] = section.name;
        sectionJson["content"] = Roar::base64Encode(section.content);
        j[key] = sectionJson;
    }
}

void from_json(nlohmann::json const& j, SectionCache& sc)
{
    sc = {};
    for (auto const& [key, sectionJson] : j.items())
    {
        Section section;
        section.type = sectionJson["type"].get<std::string>();
        section.name = sectionJson["name"].get<std::string>();
        section.content = Roar::base64Decode(sectionJson["content"].get<std::string>());
        sc.sections[key] = section;
    }
}