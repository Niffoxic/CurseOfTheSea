// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/swapchain.h"
#include "engine/graphics/hardware/device.h"
#include "engine/core/cots_assert.h"
#include "cots/cots_config.h"
#include "engine/utils/helpers.h"

#include <dxgi1_6.h>
#include <d3d12.h>

cots::graphics::hardware::swapchain::~swapchain()
{

}

bool cots::graphics::hardware::swapchain::initialize(const swapchain_create_info &info)
{
    return true;
}

void cots::graphics::hardware::swapchain::deinitialize() noexcept
{
}
