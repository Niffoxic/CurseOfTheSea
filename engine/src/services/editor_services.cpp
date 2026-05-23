// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include "engine/system/feature_locator.h"

#include <cots/cots_config.h>

#if COTS_EDITOR_ENABLED
#include "editor/editor.h"
#include <imgui.h>
#endif

namespace
{
#if COTS_EDITOR_ENABLED
    //~ availability
    bool enabled() { return cots::editor::enabled(); }

    //~ window scoping
    bool begin_window(const char* name)
    {
        if (!cots::editor::enabled()) return false;
        return ImGui::Begin(name);
    }
    void end_window()
    {
        if (!cots::editor::enabled()) return;
        ImGui::End();
    }

    //~ widgets
    void text(const char* msg)
    {
        if (!cots::editor::enabled() || !msg) return;
        ImGui::TextUnformatted(msg);
    }
    bool button(const char* label)
    {
        if (!cots::editor::enabled()) return false;
        return ImGui::Button(label);
    }
    bool checkbox(const char* label, bool* value)
    {
        if (!cots::editor::enabled() || !value) return false;
        return ImGui::Checkbox(label, value);
    }
    bool slider_float(const char* label, float* value, float min, float max)
    {
        if (!cots::editor::enabled() || !value) return false;
        return ImGui::SliderFloat(label, value, min, max);
    }
    bool slider_int(const char* label, int* value, int min, int max)
    {
        if (!cots::editor::enabled() || !value) return false;
        return ImGui::SliderInt(label, value, min, max);
    }
    void separator()
    {
        if (!cots::editor::enabled()) return;
        ImGui::Separator();
    }
    bool combo(const char* label, int* current, const char* const* items, int count)
    {
        if (!cots::editor::enabled() || !current || !items || count <= 0) return false;
        return ImGui::Combo(label, current, items, count);
    }
    bool color_edit3(const char* label, float color[3])
    {
        if (!cots::editor::enabled() || !color) return false;
        return ImGui::ColorEdit3(label, color);
    }
#else
    //~ stub set used when the editor is compiled out
    bool enabled() { return false; }

    bool begin_window(const char*) { return false; }
    void end_window  ()            {}

    void text       (const char*)                                          {}
    bool button     (const char*)                                          { return false; }
    bool checkbox   (const char*, bool*)                                   { return false; }
    bool slider_float(const char*, float*, float, float)                   { return false; }
    bool slider_int (const char*, int*,   int,   int)                      { return false; }
    void separator  ()                                                     {}
    bool combo      (const char*, int*, const char* const*, int)           { return false; }
    bool color_edit3(const char*, float[3])                                { return false; }
#endif

    void install(cots::module::services& s)
    {
        s.editor.enabled       = &enabled;

        s.editor.begin_window  = &begin_window;
        s.editor.end_window    = &end_window;

        s.editor.text          = &text;
        s.editor.button        = &button;
        s.editor.checkbox      = &checkbox;
        s.editor.slider_float  = &slider_float;
        s.editor.slider_int    = &slider_int;
        s.editor.separator     = &separator;

        s.editor.combo         = &combo;
        s.editor.color_edit3   = &color_edit3;
    }
} // namespace

COTS_INSTALL_SERVICES(install)
