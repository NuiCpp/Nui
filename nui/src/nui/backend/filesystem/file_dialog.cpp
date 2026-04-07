#include <nui/backend/filesystem/file_dialog.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define PFD_HAS_IFILEDIALOG 1
#include <portable-file-dialogs.h>
#pragma clang diagnostic pop

#include <nui/backend/filesystem/special_paths.hpp>

#include <iostream>

#ifdef __linux__
#    include <systemd/sd-bus.h>
#    include <atomic>
#    include <cstring>
#    include <algorithm>
#    include <filesystem>
#endif

namespace Nui::FileDialog
{
    // #####################################################################################################################
    namespace
    {
        //---------------------------------------------------------------------------------------------------------------------
        std::vector<std::string> flattenFilters(std::vector<Filter> const& filters)
        {
            std::vector<std::string> flattenedFilters;
            if (filters.empty())
            {
                flattenedFilters.push_back("All files");
                flattenedFilters.push_back("*");
            }
            else
            {
                for (auto const& filter : filters)
                {
                    flattenedFilters.push_back(filter.name);
                    std::string combinedMask;
                    for (auto const& mask : filter.masks)
                    {
                        combinedMask += mask;
                        combinedMask += " ";
                    }
                    combinedMask.pop_back();
                    flattenedFilters.push_back(combinedMask);
                }
            }
            return flattenedFilters;
        }
        //---------------------------------------------------------------------------------------------------------------------
        pfd::opt convertOptions(bool allowMultiSelect, bool forceOverwrite, bool forcePath)
        {
            pfd::opt opts = pfd::opt::none;
            if (allowMultiSelect)
                opts = static_cast<pfd::opt>(static_cast<int>(opts) | static_cast<int>(pfd::opt::multiselect));
            if (forceOverwrite)
                opts = static_cast<pfd::opt>(static_cast<int>(opts) | static_cast<int>(pfd::opt::force_overwrite));
            if (forcePath)
                opts = static_cast<pfd::opt>(static_cast<int>(opts) | static_cast<int>(pfd::opt::force_path));
            return opts;
        }
        //---------------------------------------------------------------------------------------------------------------------
        std::string getTitle(std::optional<std::string> const& givenTitle)
        {
            std::string title = "File Dialog";
            if (givenTitle)
                title = *givenTitle;
            return title;
        }
        //---------------------------------------------------------------------------------------------------------------------
        std::string getDefaultPath(std::optional<std::filesystem::path> const& path)
        {
            std::string defaultPath = resolvePath("%userprofile%").string();
            if (path)
                defaultPath = path->string();
            return defaultPath;
        }
        //---------------------------------------------------------------------------------------------------------------------
        std::optional<std::vector<std::filesystem::path>> getResults(auto& dialog)
        {
            const auto result = dialog.result();
            if (result.empty())
                return std::nullopt;
            std::vector<std::filesystem::path> paths;
            paths.resize(result.size());
            std::transform(result.begin(), result.end(), paths.begin(), [](std::string const& path) {
                return std::filesystem::path(path);
            });
            return paths;
        }
        //---------------------------------------------------------------------------------------------------------------------

#ifdef __linux__
        // #####################################################################################################################
        // XDG portal file chooser via sd-bus (libsystemd), no sdbus-c++ dependency
        // #####################################################################################################################
        namespace portal
        {
            //---------------------------------------------------------------------------------------------------------------------
            // Percent-decode a URI component (handles %XX sequences)
            std::string percentDecode(std::string const& encoded)
            {
                std::string result;
                result.reserve(encoded.size());
                for (std::size_t i = 0; i < encoded.size(); ++i)
                {
                    if (encoded[i] == '%' && i + 2 < encoded.size())
                    {
                        char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
                        char* end = nullptr;
                        unsigned long val = std::strtoul(hex, &end, 16);
                        if (end == hex + 2)
                        {
                            result += static_cast<char>(val);
                            i += 2;
                            continue;
                        }
                    }
                    result += encoded[i];
                }
                return result;
            }
            //---------------------------------------------------------------------------------------------------------------------
            // Convert a "file:///path/to/file" URI to a filesystem path
            std::filesystem::path uriToPath(std::string const& uri)
            {
                constexpr std::string_view prefix = "file://";
                if (uri.size() > prefix.size() && uri.substr(0, prefix.size()) == prefix)
                    return std::filesystem::path{percentDecode(uri.substr(prefix.size()))};
                return std::filesystem::path{uri};
            }
            //---------------------------------------------------------------------------------------------------------------------
            // Sanitize D-Bus unique name for use in object path:
            //   ":1.123"  ->  "1_123"  (drop leading ':', replace '.' with '_')
            std::string sanitizeSender(char const* unique_name)
            {
                std::string s = (unique_name[0] == ':') ? unique_name + 1 : unique_name;
                std::replace(s.begin(), s.end(), '.', '_');
                return s;
            }
            //---------------------------------------------------------------------------------------------------------------------
            std::string nextToken()
            {
                static std::atomic<int> counter{0};
                return "nui_" + std::to_string(++counter);
            }
            //---------------------------------------------------------------------------------------------------------------------
            struct ResponseData
            {
                bool responded = false;
                uint32_t code = 2; // 0=success, 1=cancel, 2=error
                std::vector<std::string> uris;
            };
            //---------------------------------------------------------------------------------------------------------------------
            int responseHandler(sd_bus_message* m, void* userdata, sd_bus_error* /*err*/)
            {
                auto* data = static_cast<ResponseData*>(userdata);

                uint32_t response = 2;
                if (sd_bus_message_read(m, "u", &response) < 0)
                {
                    data->responded = true;
                    return 1;
                }
                data->code = response;

                // results: a{sv}
                if (sd_bus_message_enter_container(m, 'a', "{sv}") >= 0)
                {
                    while (sd_bus_message_enter_container(m, 'e', "sv") > 0)
                    {
                        char const* key = nullptr;
                        sd_bus_message_read(m, "s", &key);
                        if (key && std::strcmp(key, "uris") == 0)
                        {
                            if (sd_bus_message_enter_container(m, 'v', "as") >= 0)
                            {
                                if (sd_bus_message_enter_container(m, 'a', "s") >= 0)
                                {
                                    char const* uri = nullptr;
                                    while (sd_bus_message_read(m, "s", &uri) > 0 && uri)
                                        data->uris.push_back(uri);
                                    sd_bus_message_exit_container(m); // a
                                }
                                sd_bus_message_exit_container(m); // v
                            }
                        }
                        else
                        {
                            sd_bus_message_skip(m, "v");
                        }
                        sd_bus_message_exit_container(m); // e
                    }
                    sd_bus_message_exit_container(m); // a
                }

                data->responded = true;
                return 1;
            }
            //---------------------------------------------------------------------------------------------------------------------
            // Append a{sv} options dict to an in-progress message.
            // Returns false on any sd-bus error.
            bool appendOptions(
                sd_bus_message* m,
                std::string const& token,
                std::vector<Filter> const& filters,
                bool multiSelect,
                bool directory,
                bool isSave,
                std::string const& defaultPath)
            {
                auto ok = [](int r) {
                    return r >= 0;
                };

                if (!ok(sd_bus_message_open_container(m, 'a', "{sv}")))
                    return false;

                // handle_token
                {
                    if (!ok(sd_bus_message_open_container(m, 'e', "sv")))
                        return false;
                    sd_bus_message_append(m, "s", "handle_token");
                    sd_bus_message_open_container(m, 'v', "s");
                    sd_bus_message_append(m, "s", token.c_str());
                    sd_bus_message_close_container(m); // v
                    sd_bus_message_close_container(m); // e
                }

                // filters: a(sa(us))
                if (!filters.empty())
                {
                    sd_bus_message_open_container(m, 'e', "sv");
                    sd_bus_message_append(m, "s", "filters");
                    sd_bus_message_open_container(m, 'v', "a(sa(us))");
                    sd_bus_message_open_container(m, 'a', "(sa(us))");
                    for (auto const& f : filters)
                    {
                        sd_bus_message_open_container(m, 'r', "sa(us)");
                        sd_bus_message_append(m, "s", f.name.c_str());
                        sd_bus_message_open_container(m, 'a', "(us)");
                        for (auto const& mask : f.masks)
                        {
                            sd_bus_message_open_container(m, 'r', "us");
                            sd_bus_message_append(m, "us", 0u, mask.c_str()); // 0 = glob pattern
                            sd_bus_message_close_container(m); // r
                        }
                        sd_bus_message_close_container(m); // a
                        sd_bus_message_close_container(m); // r
                    }
                    sd_bus_message_close_container(m); // a
                    sd_bus_message_close_container(m); // v
                    sd_bus_message_close_container(m); // e
                }

                // multiple (open only)
                if (!isSave && multiSelect)
                {
                    sd_bus_message_open_container(m, 'e', "sv");
                    sd_bus_message_append(m, "s", "multiple");
                    sd_bus_message_open_container(m, 'v', "b");
                    sd_bus_message_append(m, "b", 1);
                    sd_bus_message_close_container(m); // v
                    sd_bus_message_close_container(m); // e
                }

                // directory (open only)
                if (!isSave && directory)
                {
                    sd_bus_message_open_container(m, 'e', "sv");
                    sd_bus_message_append(m, "s", "directory");
                    sd_bus_message_open_container(m, 'v', "b");
                    sd_bus_message_append(m, "b", 1);
                    sd_bus_message_close_container(m); // v
                    sd_bus_message_close_container(m); // e
                }

                // current_name + current_folder (save only)
                if (isSave && !defaultPath.empty())
                {
                    auto p = std::filesystem::path{defaultPath};

                    auto filename = p.filename().string();
                    if (!filename.empty())
                    {
                        sd_bus_message_open_container(m, 'e', "sv");
                        sd_bus_message_append(m, "s", "current_name");
                        sd_bus_message_open_container(m, 'v', "s");
                        sd_bus_message_append(m, "s", filename.c_str());
                        sd_bus_message_close_container(m); // v
                        sd_bus_message_close_container(m); // e
                    }

                    auto parent = p.parent_path().string();
                    if (!parent.empty())
                    {
                        // current_folder is ay (null-terminated byte array)
                        sd_bus_message_open_container(m, 'e', "sv");
                        sd_bus_message_append(m, "s", "current_folder");
                        sd_bus_message_open_container(m, 'v', "ay");
                        sd_bus_message_open_container(m, 'a', "y");
                        for (char c : parent)
                            sd_bus_message_append(m, "y", static_cast<uint8_t>(c));
                        sd_bus_message_append(m, "y", static_cast<uint8_t>(0)); // null terminator
                        sd_bus_message_close_container(m); // a
                        sd_bus_message_close_container(m); // v
                        sd_bus_message_close_container(m); // e
                    }
                }

                return ok(sd_bus_message_close_container(m)); // a{sv}
            }
            //---------------------------------------------------------------------------------------------------------------------
            enum class Type
            {
                Open,
                Directory,
                Save
            };

            // Returns:
            //   outer nullopt  -> portal not available; caller should fall back to pfd
            //   inner nullopt  -> portal showed dialog; user cancelled
            //   inner vector   -> paths selected by user
            std::optional<std::optional<std::vector<std::filesystem::path>>> showDialog(
                std::string const& title,
                std::string const& defaultPath,
                std::vector<Filter> const& filters,
                bool multiSelect,
                Type type)
            {
                sd_bus* bus = nullptr;
                if (sd_bus_open_user(&bus) < 0)
                    return std::nullopt; // portal unavailable

                sd_bus_slot* slot = nullptr;

                auto cleanup = [&] {
                    if (slot)
                        sd_bus_slot_unref(slot);
                    sd_bus_unref(bus);
                };

                char const* unique_name = nullptr;
                if (sd_bus_get_unique_name(bus, &unique_name) < 0)
                {
                    cleanup();
                    return std::nullopt;
                }

                std::string token = nextToken();
                std::string handle_path =
                    "/org/freedesktop/portal/desktop/request/" + sanitizeSender(unique_name) + "/" + token;

                // Register the signal match BEFORE calling the method to avoid race conditions.
                ResponseData responseData;
                std::string matchRule =
                    "type='signal',"
                    "interface='org.freedesktop.portal.Request',"
                    "member='Response',"
                    "path='" +
                    handle_path + "'";
                if (sd_bus_add_match(bus, &slot, matchRule.c_str(), responseHandler, &responseData) < 0)
                {
                    cleanup();
                    return std::nullopt;
                }

                // Build the method call message
                sd_bus_message* callMsg = nullptr;
                char const* method = (type == Type::Save) ? "SaveFile" : "OpenFile";
                if (sd_bus_message_new_method_call(
                        bus,
                        &callMsg,
                        "org.freedesktop.portal.Desktop",
                        "/org/freedesktop/portal/desktop",
                        "org.freedesktop.portal.FileChooser",
                        method) < 0)
                {
                    cleanup();
                    return std::nullopt;
                }

                // parent_window (s), title (s)
                sd_bus_message_append(callMsg, "ss", "", title.c_str());

                // options (a{sv})
                bool const isSave = (type == Type::Save);
                bool const isDirectory = (type == Type::Directory);
                if (!appendOptions(callMsg, token, filters, multiSelect, isDirectory, isSave, defaultPath))
                {
                    sd_bus_message_unref(callMsg);
                    cleanup();
                    return std::nullopt;
                }

#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wc99-extensions"
                sd_bus_error error = SD_BUS_ERROR_NULL;
#    pragma clang diagnostic pop
                sd_bus_message* reply = nullptr;
                int r = sd_bus_call(bus, callMsg, 0, &error, &reply);
                sd_bus_message_unref(callMsg);

                if (r < 0)
                {
                    // Service not available → fall back to pfd
                    sd_bus_error_free(&error);
                    if (reply)
                        sd_bus_message_unref(reply);
                    cleanup();
                    return std::nullopt;
                }

                sd_bus_message_unref(reply);
                sd_bus_error_free(&error);

                // Portal accepted the request. Now run the event loop until the Response signal arrives.
                // Use a 5-minute timeout to avoid hanging indefinitely.
                constexpr uint64_t timeoutUs = 5ULL * 60ULL * 1'000'000ULL;
                while (!responseData.responded)
                {
                    r = sd_bus_wait(bus, timeoutUs);
                    if (r < 0)
                        break;
                    while (!responseData.responded)
                    {
                        r = sd_bus_process(bus, nullptr);
                        if (r <= 0)
                            break;
                    }
                }

                cleanup();

                if (!responseData.responded || responseData.code != 0)
                    return std::optional<std::vector<std::filesystem::path>>{std::nullopt}; // user cancelled

                std::vector<std::filesystem::path> paths;
                paths.reserve(responseData.uris.size());
                for (auto const& uri : responseData.uris)
                    paths.push_back(uriToPath(uri));

                return std::optional<std::vector<std::filesystem::path>>{std::move(paths)};
            }
        } // namespace portal
#endif // __linux__
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::vector<std::filesystem::path>> showOpenDialog(OpenDialogOptions const& options)
    {
#ifdef __linux__
        auto attempt = portal::showDialog(
            getTitle(options.title),
            getDefaultPath(options.defaultPath),
            options.filters,
            options.allowMultiSelect,
            portal::Type::Open);
        if (attempt.has_value())
            return *attempt; // portal handled it (may be nullopt if user cancelled)
#endif
        auto dialog = pfd::open_file(
            getTitle(options.title),
            getDefaultPath(options.defaultPath),
            flattenFilters(options.filters),
            convertOptions(options.allowMultiSelect, false, options.forcePath));
        return getResults(dialog);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::vector<std::filesystem::path>> showDirectoryDialog(DirectoryDialogOptions const& options)
    {
#ifdef __linux__
        auto attempt = portal::showDialog(
            getTitle(options.title), getDefaultPath(options.defaultPath), {}, false, portal::Type::Directory);
        if (attempt.has_value())
            return *attempt;
#endif
        auto dialog = pfd::select_folder(getTitle(options.title), getDefaultPath(options.defaultPath));
        const auto result = dialog.result();
        if (result.empty())
            return std::nullopt;
        return std::vector<std::filesystem::path>{std::filesystem::path(result)};
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::filesystem::path> showSaveDialog(SaveDialogOptions const& options)
    {
#ifdef __linux__
        auto attempt = portal::showDialog(
            getTitle(options.title), getDefaultPath(options.defaultPath), options.filters, false, portal::Type::Save);
        if (attempt.has_value())
        {
            if (!attempt->has_value() || (*attempt)->empty())
                return std::nullopt;
            return (*attempt)->front();
        }
#endif
        auto dialog = pfd::save_file(
            getTitle(options.title),
            getDefaultPath(options.defaultPath),
            flattenFilters(options.filters),
            convertOptions(false, options.forceOverwrite, options.forcePath));
        const auto result = dialog.result();
        if (result.empty())
            return std::nullopt;
        return std::filesystem::path(result);
    }
    // #####################################################################################################################
}
