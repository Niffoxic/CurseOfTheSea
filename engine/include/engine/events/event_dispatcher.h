// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EVENT_DISPATCHER_H
#define CURSEOFTHESEA_EVENT_DISPATCHER_H

#include "engine/core/framework/interface/tickable.h"
#include "engine/system/feature_locator.h"

#include <entt/entt.hpp>
#include <mutex>

namespace cots::events
{
    class dispatcher final: public interfaces::tickable
    {
    public:
         dispatcher() = default;
        ~dispatcher() override = default;

        dispatcher(const dispatcher&) = delete;
        dispatcher(dispatcher&&)      = default;

        dispatcher& operator=(const dispatcher&) = delete;
        dispatcher& operator=(dispatcher&&)      = default;

        void begin_update([[maybe_unused]] float dt) override
        {
            std::lock_guard lock(mutex_);
            dispatcher_.update();
        }

        void end_update() override
        {}

        //~ specifically, for main thread only
        template<typename Event, typename...Args>
        void publish(Args&&...args)
        {
            dispatcher_.trigger(Event{std::forward<Args>(args)...});
        }

        //~ specifically, for main thread only
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

    //~ profoundly for main thread
    template<typename Event, typename...Args>
    void publish(Args&&...args)
    {
        if (const auto d = feature::locator::resolve<dispatcher>())
        {
            d->publish<Event>(std::forward<Args>(args)...);
        }
    }

    //~ mainly for render thread
    template<typename Event, typename...Args>
    void publish_threadsafe(Args&&...args)
    {
        if (const auto d = feature::locator::resolve<dispatcher>())
        {
            d->enqueue_threadsafe<Event>(std::forward<Args>(args)...);
        }
    }
}

#endif
