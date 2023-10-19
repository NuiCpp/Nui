#include "temp_dir.hpp"

#include <cstdio>

TempDir::TempDir()
    : path_{[]() {
        auto path = std::tmpnam(nullptr);
        std::filesystem::create_directory(path);
        return path;
    }()}
{}
TempDir::~TempDir()
{
    std::filesystem::remove_all(path_);
}
std::filesystem::path const& TempDir::path() const
{
    return path_;
}