//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#include "trishul/core/service_locator.h"

namespace trishul::service_detail
{
    //~ single instance behind the dll boundary

    service_map& services()
    {
        static service_map s_services;
        return s_services;
    }

    service_map& null_services()
    {
        static service_map s_null_services;
        return s_null_services;
    }

    std::mutex& locator_mutex()
    {
        static std::mutex s_mutex;
        return s_mutex;
    }
} // namespace trishul::service_detail
