// Created by Niffoxic (Harsh Dubey)
#include "editor/editor.h"
#include "editor/panel.h"
#include "editor/command.h"
#include "editor/world.h"
#include "editor/passes/editor_pass.h"

#include "editor/panels/stats_panel.h"
#include "editor/panels/scene_panel.h"
#include "editor/panels/resource_panel.h"

#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/descriptor_heap.h"
#include "engine/graphics/hardware/command_context.h"
#include "engine/graphics/hardware/types.h"
#include "engine/graphics/graph/resource_registry.h"
#include "engine/graphics/passes/pass.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include <d3d12.h>
#include <dxgiformat.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

namespace cots::editor
{
    namespace
    {
        struct queued_msg
        {
            HWND   hwnd;
            UINT   msg;
            WPARAM w;
            LPARAM l;
        };

        //~ all editor state lives on the render thread
        struct editor_impl
        {
            //~ imgui context and backends
            ImGuiContext* ctx       { nullptr };
            HWND          hwnd      { nullptr };

            //~ bindless heap font slot
            cots::graphics::hardware::descriptor_heap* heap{ nullptr };
            std::uint32_t font_slot { ~0u };

            //~ retained engine panels
            std::vector<std::unique_ptr<panel>> panels;

            //~ command stacks
            std::vector<std::unique_ptr<editor_command>> undo_stack;
            std::vector<std::unique_ptr<editor_command>> redo_stack;

            //~ editor side world
            world scene{};

            //~ win32 message queue
            // main thread pushes render thread drains
            std::mutex              msg_mutex;
            std::vector<queued_msg> msg_queue;

            //~ cached io flags read by the main thread
            std::atomic<bool> want_mouse { false };
            std::atomic<bool> want_kbd   { false };

            //~ lifecycle
            std::atomic<bool> rt_inited  { false };
            bool              frame_open { false };
            std::thread::id   rt_id      {};
        };

        editor_impl& impl()
        {
            static editor_impl s;
            return s;
        }

        void build_default_panels(editor_impl& e)
        {
            e.panels.push_back(std::make_unique<stats_panel>());
            e.panels.push_back(std::make_unique<scene_panel>());
            e.panels.push_back(std::make_unique<resource_panel>());
        }
    } // namespace

    bool init_on_render_thread(
        graphics::hardware::device&          dev,
        graphics::hardware::descriptor_heap& heap,
        HWND                                 hwnd)
    {
        auto& e = impl();
        if (e.rt_inited.load(std::memory_order_acquire)) return true;

        if (!hwnd)
        {
            spdlog::error("[editor] init_on_render_thread null hwnd");
            return false;
        }

        auto* d3d = dev.d3d12_device();
        if (!d3d)
        {
            spdlog::error("[editor] init_on_render_thread no d3d device");
            return false;
        }

        IMGUI_CHECKVERSION();
        e.ctx = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename  = "editor.ini";

        if (!ImGui_ImplWin32_Init(hwnd))
        {
            spdlog::error("[editor] ImGui_ImplWin32_Init failed");
            ImGui::DestroyContext(e.ctx);
            e.ctx = nullptr;
            return false;
        }

        //~ build font atlas before backend texture upload
        io.Fonts->AddFontDefault();
        unsigned char* px = nullptr;
        int            fw = 0;
        int            fh = 0;
        io.Fonts->GetTexDataAsRGBA32(&px, &fw, &fh);

        e.font_slot = heap.acquire();
        if (e.font_slot == graphics::hardware::descriptor_heap::invalid_slot)
        {
            spdlog::error("[editor] bindless heap exhausted no font slot");
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(e.ctx);
            e.ctx = nullptr;
            return false;
        }

        const auto cpu = heap.cpu_handle(e.font_slot);
        const auto gpu = heap.gpu_handle(e.font_slot);

        if (!ImGui_ImplDX12_Init(
                d3d,
                static_cast<int>(graphics::hardware::frame_count),
                DXGI_FORMAT_R8G8B8A8_UNORM,
                heap.heap(),
                cpu,
                gpu))
        {
            spdlog::error("[editor] ImGui_ImplDX12_Init failed");
            heap.release(e.font_slot);
            e.font_slot = ~0u;
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(e.ctx);
            e.ctx = nullptr;
            return false;
        }

        ImGui_ImplDX12_CreateDeviceObjects();

        e.hwnd  = hwnd;
        e.heap  = &heap;
        e.rt_id = std::this_thread::get_id();
        build_default_panels(e);

        e.rt_inited.store(true, std::memory_order_release);
        spdlog::info("[editor] render thread init complete font {}x{} slot {}",
                     fw, fh, e.font_slot);
        return true;
    }

    void shutdown_on_render_thread()
    {
        auto& e = impl();
        if (!e.rt_inited.load(std::memory_order_acquire)) return;

        e.panels.clear();
        e.undo_stack.clear();
        e.redo_stack.clear();

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();

        if (e.heap && e.font_slot != ~0u)
        {
            e.heap->release(e.font_slot);
        }
        e.font_slot = ~0u;
        e.heap      = nullptr;
        e.hwnd      = nullptr;

        if (e.ctx)
        {
            ImGui::DestroyContext(e.ctx);
            e.ctx = nullptr;
        }

        {
            std::lock_guard lock(e.msg_mutex);
            e.msg_queue.clear();
        }
        e.want_mouse.store(false, std::memory_order_release);
        e.want_kbd  .store(false, std::memory_order_release);
        e.rt_inited .store(false, std::memory_order_release);
        spdlog::info("[editor] render thread shutdown complete");
    }

    void queue_win32_message(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
    {
        auto& e = impl();
        if (!e.rt_inited.load(std::memory_order_acquire)) return;
        std::lock_guard lock(e.msg_mutex);
        e.msg_queue.push_back({ hwnd, msg, w, l });
    }

    bool want_capture_mouse()
    {
        return impl().want_mouse.load(std::memory_order_acquire);
    }

    bool want_capture_keyboard()
    {
        return impl().want_kbd.load(std::memory_order_acquire);
    }

    void frame_step(const graphics::pass_context&    pc,
                    graphics::graph::resource_handle backbuffer)
    {
        auto& e = impl();
        if (!e.rt_inited.load(std::memory_order_acquire)) return;

        //~ drain queued win32 messages and forward to the imgui backend
        std::vector<queued_msg> drained;
        {
            std::lock_guard lock(e.msg_mutex);
            drained.swap(e.msg_queue);
        }
        for (const auto& m : drained)
        {
            ImGui_ImplWin32_WndProcHandler(m.hwnd, m.msg, m.w, m.l);
        }

        //~ start the frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        e.frame_open = true;

        //~ retained panels render themselves
        for (auto& p : e.panels)
        {
            if (p) p->draw();
        }

        //~ publish capture flags for the main thread input gate
        const ImGuiIO& io = ImGui::GetIO();
        e.want_mouse.store(io.WantCaptureMouse,    std::memory_order_release);
        e.want_kbd  .store(io.WantCaptureKeyboard, std::memory_order_release);

        ImGui::Render();
        e.frame_open = false;

        //~ bind the backbuffer and submit the draw data
        const auto& bb = pc.resources.view(backbuffer);
        pc.ctx.set_render_target(bb.view_handle);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pc.ctx.list());
    }

    bool enabled()
    {
        auto& e = impl();
        if (!e.rt_inited.load(std::memory_order_acquire)) return false;
        if (std::this_thread::get_id() != e.rt_id)        return false;
        return e.frame_open;
    }

    std::unique_ptr<graphics::pass>
    make_editor_pass(graphics::graph::resource_handle backbuffer)
    {
        return std::make_unique<editor_pass>(backbuffer);
    }

    void undo()
    {
        auto& e = impl();
        if (e.undo_stack.empty()) return;
        auto cmd = std::move(e.undo_stack.back());
        e.undo_stack.pop_back();
        cmd->undo();
        e.redo_stack.push_back(std::move(cmd));
    }

    void redo()
    {
        auto& e = impl();
        if (e.redo_stack.empty()) return;
        auto cmd = std::move(e.redo_stack.back());
        e.redo_stack.pop_back();
        cmd->execute();
        e.undo_stack.push_back(std::move(cmd));
    }

    //~ command stack helpers exposed via command h
    void push_command(std::unique_ptr<editor_command> cmd)
    {
        if (!cmd) return;
        cmd->execute();
        impl().undo_stack.push_back(std::move(cmd));
        impl().redo_stack.clear();
    }

    std::size_t undo_depth() { return impl().undo_stack.size(); }
    std::size_t redo_depth() { return impl().redo_stack.size(); }

    void clear_command_history()
    {
        impl().undo_stack.clear();
        impl().redo_stack.clear();
    }

    world& world_instance()
    {
        return impl().scene;
    }
} // namespace cots::editor
