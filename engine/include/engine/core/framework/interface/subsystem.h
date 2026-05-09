// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SUBSYSTEM_H
#define CURSEOFTHESEA_SUBSYSTEM_H

namespace cots::interfaces
{
    class subsystem
    {
    public:
        virtual ~subsystem() = default;

        [[nodiscard]] virtual bool initialize  () = 0;
                      virtual void deinitialize() noexcept = 0;

        void setup_config(const std::byte* config)
        {
            config_ = config;
        }

        [[nodiscard]] const std::byte* get_config() const noexcept
        {
            return config_;
        }

    protected:
        const std::byte* config_ = nullptr;
    };
} // namespace cots::interface

#endif //CURSEOFTHESEA_SUBSYSTEM_H
