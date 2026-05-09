// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_INTERFACE_TICKABLE_H
#define CURSEOFTHESEA_INTERFACE_TICKABLE_H

namespace cots::interface
{
    class tickable
    {
    public:
        virtual ~tickable() = default;

        // called before end update after calling begin on all tickable
        virtual void begin_update(float dt) = 0;
        virtual void end_update  ()         = 0;
    };
} // namespace cots::interface

#endif //CURSEOFTHESEA_INTERFACE_TICKABLE_H
