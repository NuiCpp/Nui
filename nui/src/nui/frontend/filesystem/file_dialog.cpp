#include <nui/frontend/filesystem/file_dialog.hpp>
#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>
#include <nui/frontend/api/console.hpp>

#include <emscripten/val.h>

#include <algorithm>

namespace Nui::FileDialog
{
    //#####################################################################################################################
    namespace
    {
        template <typename T>
        void convertOptions(emscripten::val& opts, T const& options)
        {
            opts.set("title", to_val(options.title));
            opts.set("defaultPath", to_val(options.defaultPath));
            opts.set("filters", emscripten::val::array());
            for (auto const& filter : options.filters)
            {
                emscripten::val filterVal;
                filterVal.set("name", to_val(filter.name));
                filterVal.set("masks", to_val(filter.masks));
                opts["filter"].call<void>("push", filterVal);
            }
            opts.set("forcePath", to_val(options.forcePath));
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void showOpenDialog(
        OpenDialogOptions const& options,
        std::function<void(std::optional<std::vector<std::filesystem::path>>)> onResult)
    {
        emscripten::val opts = emscripten::val::object();
        convertOptions(opts, options);
        opts.set("allowMultiSelect", options.allowMultiSelect);
        const auto id = RpcClient::registerFunctionOnce([onResult](emscripten::val const& param) {
            if (param.typeOf().as<std::string>() == "null")
                onResult(std::nullopt);
            else
            {
                const auto stringPaths = emscripten::vecFromJSArray<std::string>(param);
                std::vector<std::filesystem::path> paths(stringPaths.size());
                std::transform(stringPaths.begin(), stringPaths.end(), paths.begin(), [](std::string const& path) {
                    return std::filesystem::path(path);
                });
                onResult(paths);
            }
        });
        opts.set("callbackId", id);
        RpcClient::getRemoteCallable("nui_showOpenDialog")(opts);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void showDirectoryDialog(
        DirectoryDialogOptions const& options,
        std::function<void(std::optional<std::vector<std::filesystem::path>>)> onResult)
    {
        emscripten::val opts = emscripten::val::object();
        convertOptions(opts, options);
        const auto id = RpcClient::registerFunctionOnce([onResult = std::move(onResult)](emscripten::val const& param) {
            Console::log(param);
            if (param.typeOf().as<std::string>() == "null")
                return onResult(std::nullopt);
            else
            {
                const auto stringPaths = emscripten::vecFromJSArray<std::string>(param);
                std::vector<std::filesystem::path> paths(stringPaths.size());
                std::transform(stringPaths.begin(), stringPaths.end(), paths.begin(), [](std::string const& path) {
                    return std::filesystem::path(path);
                });
                return onResult(paths);
            }
        });
        opts.set("callbackId", id);
        RpcClient::getRemoteCallable("nui_showDirectoryDialog")(opts);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void
    showSaveDialog(SaveDialogOptions const& options, std::function<void(std::optional<std::filesystem::path>)> onResult)
    {
        emscripten::val opts = emscripten::val::object();
        convertOptions(opts, options);
        opts.set("forceOverwrite", options.forceOverwrite);
        const auto id = RpcClient::registerFunctionOnce([onResult = std::move(onResult)](emscripten::val const& param) {
            if (param.typeOf().as<std::string>() == "null")
                return onResult(std::nullopt);
            return onResult(std::filesystem::path{param.as<std::string>()});
        });
        opts.set("callbackId", id);
        RpcClient::getRemoteCallable("nui_showSaveDialog")(opts);
    }
    //#####################################################################################################################
}