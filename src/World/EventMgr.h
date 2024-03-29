﻿/*
 * Copyright (C) 2015 Dehravor <dehravor@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Define.h"
#include <list>
#include <functional>
#include <thread>
#include <memory>
#include <mutex>

enum EventId
{
    EVENT_PROCESS_INCOMING      = 0,
    EVENT_SEND_KEEP_ALIVE       = 1,
    EVENT_SEND_PING             = 2,
    EVENT_PERIODIC_SAVE         = 4
};

typedef std::function<void()> EventCallback;

class Event
{
    public:
        Event(EventId id);

        EventId GetId();

        void SetPeriod(uint32 period);
        void SetEnabled(bool enabled);
        void SetCallback(EventCallback callback);

        void Update(uint32 diff);
    private:
        EventId id_;
        bool enabled_;
        uint32 period_;
        int32 remaining_;
        EventCallback callback_;
};

class EventMgr
{
    public:
        EventMgr();
        ~EventMgr();

        void AddEvent(std::shared_ptr<Event> event);
        void RemoveEvent(EventId id);
        std::shared_ptr<Event> GetEvent(EventId id);

        void Start();
        void Stop();

    private:
        std::thread thread_;
        bool isRunning_;
        std::recursive_mutex eventMutex_;
        std::list<std::shared_ptr<Event>> events_;

        void ProcessEvents();
};
