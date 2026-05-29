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
#ifndef CURSEOFTHESEA_ISUBSYSTEMS_H
#define CURSEOFTHESEA_ISUBSYSTEMS_H
#include <string_view>

namespace trishul::interfaces
{
    class __declspec(novtable) subsystems
    {
    public:
        virtual ~subsystems() noexcept = default;

        subsystems(const subsystems&)            = delete;
        subsystems(subsystems&&)                 = delete;
        subsystems& operator=(const subsystems&) = delete;
        subsystems& operator=(subsystems&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        virtual bool initialize  ()          = 0;
        virtual void deinitialize() noexcept = 0;

        [[nodiscard]]
        std::string_view name() const noexcept { return name_; }

    protected:
        subsystems() = default;

        explicit subsystems(const std::string_view name)
            : name_{ name }
        {}

        void set_name(const std::string_view name) { name_ = name; }

    private:
        std::string name_{ "No Name" };
    };
} // namespace trishul::interfaces

#endif //CURSEOFTHESEA_ISUBSYSTEMS_H
