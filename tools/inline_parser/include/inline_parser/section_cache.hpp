#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <optional>
#include <unordered_map>
#include <set>

struct Section
{
    std::string type;
    std::string name;
    std::string content;
};

class SectionCache
{
  public:
    void addSection(Section const& section);
    std::optional<Section> findSection(std::string const& type, std::string const& name) const;
    void fromFile(std::filesystem::path const& path);
    void toFile(std::filesystem::path const& path) const;

    /// Deletes all sections that were not overwritten.
    void removeColdSections();

    void produceToDirectory(
        std::filesystem::path const& basePath,
        std::filesystem::path const& path,
        bool emitComments = true) const;

    friend void to_json(nlohmann::json& j, SectionCache const& sc);
    friend void from_json(nlohmann::json const& j, SectionCache& sc);

    std::unordered_map<std::string, Section>::const_iterator begin() const;
    std::unordered_map<std::string, Section>::const_iterator end() const;
    std::unordered_map<std::string, Section>::const_iterator
    find(std::string const& type, std::string const& name) const;

    std::set<std::string>::const_iterator coldBegin() const;
    std::set<std::string>::const_iterator coldEnd() const;

  private:
    std::unordered_map<std::string, Section> sections;
    std::set<std::string> coldSections;
};