// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include "engine/system/define_features.h"
#include "engine/platform/platform_windows.h"

namespace
{
    void get_size(int* w, int* h)
    {
        const auto win = cots::feature::locator::resolve<cots::platform::windows>();
        const auto size = win->get_window_size<int>();
        if (w) *w = size.width;
        if (h) *h = size.height;
    }

    void install(cots::module::services& s)
    {
        s.window.get_size = &get_size;
    }
}

COTS_INSTALL_SERVICES(install)
