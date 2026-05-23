// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_H
#define CURSEOFTHESEA_EDITOR_H

#include <cstdint>
#include <memory>
#include <windows.h>

#include "engine/graphics/graph/resource_handle.h"

namespace cots::graphics
{
    class pass;
    struct pass_context;
    namespace hardware
    {
        class device;
        class descriptor_heap;
    } // namespace hardware
} // namespace cots::graphics

namespace cots::editor
{
    //~ render thread lifetime
    bool init_on_render_thread (graphics::hardware::device&          dev,
                                graphics::hardware::descriptor_heap& heap,
                                HWND                                 hwnd);
    void shutdown_on_render_thread();

    void frame_step(const graphics::pass_context&    pc,
                    graphics::graph::resource_handle backbuffer);

    void queue_win32_message(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

    //~ main thread cached io flags
    bool want_capture_mouse   ();
    bool want_capture_keyboard();
    bool enabled              ();

    //~ factory for the render graph pass
    std::unique_ptr<graphics::pass>
    make_editor_pass(graphics::graph::resource_handle backbuffer);

    //~ undo redo entry points
    // safe only on the render thread
    void undo();
    void redo();
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_H
