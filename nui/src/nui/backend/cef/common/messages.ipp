#pragma once

#include <include/cef_shared_memory_region.h>
#include <include/cef_values.h>

#include <string>

namespace Nui::Detail::Cef
{
    template <typename VisitorT>
    bool visitPayload(CefRefPtr<CefProcessMessage> const& message, VisitorT&& visitor)
    {
        if (const auto region = message->GetSharedMemoryRegion(); region && region->IsValid())
        {
            visitor(std::string_view{static_cast<char const*>(region->Memory()), region->Size()});
            return true;
        }
        const auto arguments = message->GetArgumentList();
        if (!arguments || arguments->GetSize() < 1u || arguments->GetType(0) != VTYPE_BINARY)
            return false;
        const auto binary = arguments->GetBinary(0);
        const auto size = binary->GetSize();
        if (size == 0u)
        {
            visitor(std::string_view{});
            return true;
        }
        if (auto const* data = binary->GetRawData(); data != nullptr)
        {
            visitor(std::string_view{static_cast<char const*>(data), size});
            return true;
        }
        std::string copy(size, '\0');
        binary->GetData(copy.data(), size, 0u);
        visitor(std::string_view{copy});
        return true;
    }
}
