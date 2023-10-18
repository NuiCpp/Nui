#pragma once

#include <inline_parser/constants.hpp>
#include <inline_parser/section_cache.hpp>
#include <inline_parser/parser.hpp>

#include <filesystem>
#include <functional>
#include <fstream>
#include <mutex>

class InlineExtractor
{
  public:
    InlineExtractor(std::function<void(std::string const&)> onWarning = [](std::string const&) {});
    void parseFileForSections(std::filesystem::path const& path);
    void parallelParseFilesForSections(std::vector<std::filesystem::path> const& paths);

    SectionCache& sectionCache();
    SectionCache const& sectionCache() const;

  private:
    bool parseInlineDirective(std::string const& line, Section& section) const;

  private:
    SectionCache sectionCache_;
    std::function<void(std::string const&)> onWarning_;
    std::mutex sectionCacheMutex_;
};