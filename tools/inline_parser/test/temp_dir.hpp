#pragma once

#include <filesystem>

class TempDir
{
  public:
    TempDir();
    ~TempDir();

    std::filesystem::path const& path() const;

  private:
    std::filesystem::path path_;
};