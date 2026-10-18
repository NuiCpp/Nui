#include "gate.hpp"

#include <unordered_map>

namespace Nui::Detail::Cef
{
    namespace
    {
        std::unordered_map<Gate const*, int>& threadEntries()
        {
            thread_local std::unordered_map<Gate const*, int> entries;
            return entries;
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    GateEntry::GateEntry(Gate& gate)
        : gate_{&gate}
        , entered_{gate.enter()}
    {
        if (entered_)
            ++threadEntries()[gate_];
    }
    //---------------------------------------------------------------------------------------------------------------------
    GateEntry::~GateEntry()
    {
        if (!entered_)
            return;
        auto& entries = threadEntries();
        if (const auto iter = entries.find(gate_); iter != entries.end() && --iter->second == 0)
            entries.erase(iter);
        gate_->leave();
    }
    //---------------------------------------------------------------------------------------------------------------------
    int GateEntry::heldByCurrentThread(Gate const& gate)
    {
        auto const& entries = threadEntries();
        const auto iter = entries.find(&gate);
        return iter == entries.end() ? 0 : iter->second;
    }
}
