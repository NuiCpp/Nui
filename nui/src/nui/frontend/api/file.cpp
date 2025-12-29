#include <nui/frontend/api/file.hpp>

namespace Nui::WebApi
{
    File::File(Nui::val file)
        : ValWrapper{std::move(file)}
    {}
    std::chrono::system_clock::time_point File::lastModified() const
    {
        return std::chrono::system_clock::time_point{
            std::chrono::milliseconds{val_["lastModified"].as<unsigned long long>()}};
    }
    std::string File::name() const
    {
        return val_["name"].as<std::string>();
    }
    std::string File::webkitRelativePath() const
    {
        return val_["webkitRelativePath"].as<std::string>();
    }
}