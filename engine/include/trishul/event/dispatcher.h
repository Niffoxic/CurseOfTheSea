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
#ifndef CURSEOFTHESEA_DISPATCHER_H
#define CURSEOFTHESEA_DISPATCHER_H

#include "trishul/core/interface/tickable.h"
#include "trishul/core/service_locator.h"

#include <entt/entt.hpp>
#include <mutex>

namespace trishul::events
{
    class dispatcher final: public interfaces::tickable
    {
    public:
         dispatcher() = default;
        ~dispatcher() override = default;

        dispatcher(const dispatcher&) = delete;
        dispatcher(dispatcher&&)      = delete;

        dispatcher& operator=(const dispatcher&) = delete;
        dispatcher& operator=(dispatcher&&)      = delete;

        void begin_update([[maybe_unused]] float dt) override
        {
            std::lock_guard lock(mutex_);
            dispatcher_.update();
        }

        void end_update() override
        {}

        //~ specifically for main thread only
        template<typename Event, typename...Args>
        void publish(Args&&...args)
        {
            dispatcher_.trigger(Event{std::forward<Args>(args)...});
        }

        //~ specifically for main thread only
        template<typename Event, typename...Args>
        void enqueue(Args&&...args)
        {
            dispatcher_.enqueue<Event>(std::forward<Args>(args)...);
        }

        template<typename Event, typename...Args>
        void enqueue_threadsafe(Args&&...args)
        {
            std::lock_guard lock(mutex_);
            dispatcher_.enqueue<Event>(std::forward<Args>(args)...);
        }

        template<typename Event, auto Method, typename Instance>
        void subscribe(Instance& instance)
        {
            dispatcher_.sink<Event>().template connect<Method>(instance);
        }

        template<typename Event, auto Method, typename Instance>
        void unsubscribe(Instance& instance)
        {
            dispatcher_.sink<Event>().template disconnect<Method>(instance);
        }

    private:
        entt::dispatcher dispatcher_;
        std::mutex       mutex_;
    };

    //~ profoundly for main thread no op when no dispatcher is registered yet
    template<typename Event, typename...Args>
    void publish(Args&&...args)
    {
        if (const auto d = service_locator::try_get<dispatcher>())
        {
            d->publish<Event>(std::forward<Args>(args)...);
        }
    }

    //~ mainly for render thread no op when no dispatcher is registered yet
    template<typename Event, typename...Args>
    void publish_threadsafe(Args&&...args)
    {
        if (const auto d = service_locator::try_get<dispatcher>())
        {
            d->enqueue_threadsafe<Event>(std::forward<Args>(args)...);
        }
    }
} // namespace trishul::events

#endif //CURSEOFTHESEA_DISPATCHER_H
