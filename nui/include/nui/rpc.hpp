#pragma once

#include <nui/core.hpp>

#ifdef NUI_BACKEND
#    include <nui/backend/rpc_hub.hpp>
#elif defined(NUI_FRONTEND)
#    include <nui/frontend/rpc_client.hpp>
#endif