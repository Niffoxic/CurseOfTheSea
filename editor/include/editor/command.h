// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_COMMAND_H
#define CURSEOFTHESEA_EDITOR_COMMAND_H

#include <memory>
#include <vector>

namespace cots::editor
{
    //~ discrete editor
    class editor_command
    {
    public:
        virtual ~editor_command() = default;

        editor_command() = default;
        editor_command(const editor_command&) = delete;
        editor_command(editor_command&&)      = delete;
        editor_command& operator=(const editor_command&) = delete;
        editor_command& operator=(editor_command&&)      = delete;

        //~ apply the change
        virtual void execute() = 0;

        //~ reverse the change
        virtual void undo() = 0;

        //~ short debug name
        [[nodiscard]] virtual const char* name() const noexcept = 0;
    };

    //~ owned by the editor
    // executes the command appends to undo and wipes redo
    void push_command(std::unique_ptr<editor_command> cmd);

    //~ stack size accessors for the editor side ui
    std::size_t undo_depth();
    std::size_t redo_depth();

    //~ wipe both stacks for fresh state
    void clear_command_history();
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_COMMAND_H
