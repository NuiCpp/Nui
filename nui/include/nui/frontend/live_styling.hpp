#pragma once

#ifdef NUI_ENABLE_LIVE_RELOAD
#    include <nui/frontend/attributes/class.hpp>
#    include <nui/frontend/event_system/observed_value.hpp>
#    include <nui/frontend/rpc_client.hpp>

#    include <unordered_map>

namespace Nui::Detail
{
    struct LiveObservations
    {
        struct LineObservations
        {
            struct IdObservations
            {
                std::unordered_map<int, Observed<std::string>> observations;

                auto operator[](int id) -> decltype(auto)
                {
                    return observations[id];
                }
                auto find(int id) -> decltype(auto)
                {
                    return observations.find(id);
                }
                auto begin() -> decltype(auto)
                {
                    return observations.begin();
                }
                auto end() -> decltype(auto)
                {
                    return observations.end();
                }
                auto size() const
                {
                    return observations.size();
                }
            };

            std::unordered_map<int, IdObservations> lineObservations;

            auto operator[](int line) -> decltype(auto)
            {
                return lineObservations[line];
            }
            auto find(int line) -> decltype(auto)
            {
                return lineObservations.find(line);
            }
            auto end() -> decltype(auto)
            {
                return lineObservations.end();
            }
        };

        std::unordered_map<std::string /*file*/, LineObservations> fileObservations;

        auto operator[](std::string const& file) -> decltype(auto)
        {
            return fileObservations[file];
        }
        auto find(std::string const& file) -> decltype(auto)
        {
            return fileObservations.find(file);
        }
        auto end() -> decltype(auto)
        {
            return fileObservations.end();
        }
    };

    static LiveObservations liveObservations;
}

namespace Nui
{
    namespace Detail
    {
        class LiveLiteralAttribute
        {
          public:
            constexpr LiveLiteralAttribute(char const* name, const char* file, int line, int counter)
                : name_{name}
                , file_{file}
                , line_{line}
                , counter_{counter}
            {}

            constexpr const char* name() const
            {
                return name_;
            }

            constexpr const char* file() const
            {
                return file_;
            }

            constexpr int line() const
            {
                return line_;
            }

            constexpr int counter() const
            {
                return counter_;
            }

            auto operator=(const char* value) const
            {
                Detail::liveObservations[file()][line()][counter()] = value;
                auto& val = Detail::liveObservations[file()][line()][counter()];

                return Attribute{
                    [name = name(), counter = counter(), &val](Dom::ChildlessElement& element) {
                        element.setAttribute(name, val.value());
                    },
                    [name = name(), &val](std::weak_ptr<Dom::ChildlessElement>&& element) {
                        return Attributes::Detail::defaultAttributeEvent(
                            std::move(element), Nui::Detail::CopyableObservedWrap{val}, name);
                    },
                    [&val](EventContext::EventIdType const& id) {
                        val.detachEvent(id);
                    },
                };
            }

          private:
            const char* name_;
            const char* file_;
            int line_;
            int counter_;
        };

        class LiveLiteralAttributeProxy
        {
          public:
            constexpr LiveLiteralAttribute
            withSourceLocation(const char* name, const char* file, int line, int counter) const
            {
                return LiveLiteralAttribute{name, file, line, counter};
            }
        };
    }

    constexpr static auto LiveLiteralAttribute = Detail::LiveLiteralAttributeProxy{};

    inline void registerLiveReloadListener()
    {
        Nui::RpcClient::registerFunction(
            "nui::liveStyleReload",
            [](std::string const& fileName, int line, std::vector<std::string> const& literals) {
                const auto fileIter = Detail::liveObservations.find(fileName);
                if (fileIter == Detail::liveObservations.end())
                    return;

                const auto lineIter = fileIter->second.find(line);
                if (lineIter == fileIter->second.end())
                    return;

                if (literals.size() != lineIter->second.size())
                {
                    Nui::Console::log("Live reload failed: Mismatch in number of literals");
                    return;
                }
                for (std::size_t i = 0; auto& [counter, obs] : lineIter->second)
                {
                    Nui::Console::log("Updated", fileName, "line", line, "counter", counter, "to", literals[i]);
                    obs = literals[i++];
                }
            });
    }
}
#else
#endif

#ifdef NUI_ENABLE_LIVE_RELOAD
#    define NUI_LIVE_CLASS Nui::LiveLiteralAttribute.withSourceLocation("class", __FILE__, __LINE__, __COUNTER__)
#else
#    define NUI_LIVE_CLASS class_
#endif