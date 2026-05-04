// Created by Niffoxic (Harsh Dubey)
#include "engine/service_locator.h"

namespace cots
{
    std::unordered_map<std::type_index, std::shared_ptr<void>> service_locator::services_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> service_locator::nullServices_;
    std::mutex service_locator::mutex_;
}
