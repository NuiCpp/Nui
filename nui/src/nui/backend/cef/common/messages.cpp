#include "messages.hpp"

#include <include/cef_shared_process_message_builder.h>

#include <cstring>
#include <string>

namespace Nui::Detail::Cef
{
    CefRefPtr<CefProcessMessage> makePayloadMessage(char const* name, std::string_view const* parts, std::size_t count)
    {
        std::size_t size = 0u;
        for (std::size_t index = 0u; index != count; ++index)
            size += parts[index].size();

        if (size >= sharedMemoryThreshold)
        {
            auto builder = CefSharedProcessMessageBuilder::Create(name, size);
            if (builder && builder->IsValid())
            {
                auto* memory = static_cast<char*>(builder->Memory());
                for (std::size_t index = 0u; index != count; ++index)
                {
                    std::memcpy(memory, parts[index].data(), parts[index].size());
                    memory += parts[index].size();
                }
                return builder->Build();
            }
        }

        auto message = CefProcessMessage::Create(name);
        // CefBinaryValue cannot be empty.
        if (size == 0u)
        {
            message->GetArgumentList()->SetNull(0);
            return message;
        }
        std::string buffer;
        buffer.reserve(size);
        for (std::size_t index = 0u; index != count; ++index)
            buffer.append(parts[index]);
        message->GetArgumentList()->SetBinary(0, CefBinaryValue::Create(buffer.data(), buffer.size()));
        return message;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefProcessMessage> makeCallMessage(std::string_view functionName, std::string_view json)
    {
        const auto nameSize = static_cast<std::uint32_t>(functionName.size());
        const std::string_view parts[] = {
            std::string_view{reinterpret_cast<char const*>(&nameSize), sizeof(nameSize)}, functionName, json};
        return makePayloadMessage(callMessageName, parts, std::size(parts));
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefProcessMessage> makeTextMessage(char const* name, std::string_view text)
    {
        return makePayloadMessage(name, &text, 1u);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool splitCallPayload(std::string_view payload, std::string_view& functionName, std::string_view& json)
    {
        std::uint32_t nameSize = 0u;
        if (payload.size() < sizeof(nameSize))
            return false;
        std::memcpy(&nameSize, payload.data(), sizeof(nameSize));
        if (payload.size() - sizeof(nameSize) < nameSize)
            return false;
        functionName = payload.substr(sizeof(nameSize), nameSize);
        json = payload.substr(sizeof(nameSize) + nameSize);
        return true;
    }
}
