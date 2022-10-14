#include <nui/filesystem/file_dialog_options.hpp>

namespace Nui::FileDialog
{
    //#####################################################################################################################
    template <typename T>
    void to_json_common(nlohmann::json& json, T const& options)
    {
        if (options.title)
            json["title"] = *options.title;
        if (options.defaultPath)
            json["defaultPath"] = options.defaultPath->string();
        json["filters"] = options.filters;
        json["forcePath"] = options.forcePath;
    }
    //---------------------------------------------------------------------------------------------------------------------
    template <typename T>
    void from_json_common(nlohmann::json const& json, T& options)
    {
        if (json.contains("title"))
            options.title = json["title"].get<std::string>();
        else
            options.title = std::nullopt;

        if (json.contains("defaultPath"))
            options.defaultPath = json["defaultPath"].get<std::string>();
        else
            options.defaultPath = std::nullopt;

        json.at("filters").get_to(options.filters);
        json.at("forcePath").get_to(options.forcePath);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& json, OpenDialogOptions const& options)
    {
        to_json_common(json, options);
        json["allowMultiSelect"] = options.allowMultiSelect;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void from_json(nlohmann::json const& json, OpenDialogOptions& options)
    {
        from_json_common(json, options);
        options.allowMultiSelect = json["allowMultiSelect"].get<bool>();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& json, DirectoryDialogOptions const& options)
    {
        to_json_common(json, options);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void from_json(nlohmann::json const& json, DirectoryDialogOptions& options)
    {
        from_json_common(json, options);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void to_json(nlohmann::json& json, SaveDialogOptions const& options)
    {
        to_json_common(json, options);
        json["forceOverwrite"] = options.forceOverwrite;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void from_json(nlohmann::json const& json, SaveDialogOptions& options)
    {
        from_json_common(json, options);
        options.forceOverwrite = json["forceOverwrite"].get<bool>();
    }
    //#####################################################################################################################
}