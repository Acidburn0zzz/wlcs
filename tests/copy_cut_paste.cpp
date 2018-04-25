/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "helpers.h"
#include "in_process_server.h"

#include <gmock/gmock.h>

#include <memory>
#include <mutex>

using namespace testing;
using namespace wlcs;

namespace
{
struct StartedInProcessServer : InProcessServer
{
    StartedInProcessServer() { InProcessServer::SetUp(); }

    void SetUp() override {}
};

auto static const any_width = 100;
auto static const any_height = 100;
auto static const any_mime_type = "AnyMimeType";

struct CCnPClient : Client
{
    using Client::Client;
    Surface surface = create_visible_surface(any_width, any_height);
};

class DataSource
{
public:
    DataSource() = default;

    explicit DataSource(struct wl_data_source* ds) : self{ds, deleter} {}

    operator struct wl_data_source*() const { return self.get(); }

    void reset() { self.reset(); }

    void reset(struct wl_data_source* ds) { self.reset(ds, deleter); }

    friend void wl_data_source_destroy(DataSource const&) = delete;

private:
    static void deleter(struct wl_data_source* ds) { wl_data_source_destroy(ds); }

    std::shared_ptr<struct wl_data_source> self;
};

class DataDevice
{
public:
    DataDevice() = default;

    explicit DataDevice(struct wl_data_device* dd) : self{dd, deleter} {}

    operator struct wl_data_device*() const { return self.get(); }

    void reset() { self.reset(); }

    void reset(struct wl_data_device* dd) { self.reset(dd, deleter); }

    friend void wl_data_device_destroy(DataDevice const&) = delete;

private:
    static void deleter(struct wl_data_device* dd) { wl_data_device_destroy(dd); }

    std::shared_ptr<struct wl_data_device> self;
};

struct CopyCutPaste : StartedInProcessServer
{
    CCnPClient source{the_server()};
    DataSource source_data{wl_data_device_manager_create_data_source(source.data_device_manager())};

    CCnPClient sink{the_server()};
    DataDevice sink_data{wl_data_device_manager_get_data_device(sink.data_device_manager(), sink.seat())};
};

class ActiveListeners
{
public:
    void add(void* listener)
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        listeners.insert(listener);
    }

    void del(void* listener)
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        listeners.erase(listener);
    }

    bool includes(void* listener) const
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return listeners.find(listener) != end(listeners);
    }

private:
    std::mutex mutable mutex;
    std::set<void*> listeners;
};

struct DataDeviceListener
{
    DataDeviceListener(struct wl_data_device* data_device)
    {
        active_listeners.add(this);
        wl_data_device_add_listener(data_device, &thunks, this);
    }

    virtual ~DataDeviceListener() { active_listeners.del(this); }

    DataDeviceListener(DataDeviceListener const&) = delete;

    DataDeviceListener& operator=(DataDeviceListener const&) = delete;

    virtual void data_offer(
        struct wl_data_device* wl_data_device,
        struct wl_data_offer* id);

    virtual void enter(
        struct wl_data_device* wl_data_device,
        uint32_t serial,
        struct wl_surface* surface,
        wl_fixed_t x,
        wl_fixed_t y,
        struct wl_data_offer* id);

    virtual void leave(struct wl_data_device* wl_data_device);

    virtual void motion(
        struct wl_data_device* wl_data_device,
        uint32_t time,
        wl_fixed_t x,
        wl_fixed_t y);

    virtual void drop(struct wl_data_device* wl_data_device);

    virtual void selection(
        struct wl_data_device* wl_data_device,
        struct wl_data_offer* id);

private:
    static void data_offer(
        void* data,
        struct wl_data_device* wl_data_device,
        struct wl_data_offer* id);

    static void enter(
        void* data,
        struct wl_data_device* wl_data_device,
        uint32_t serial,
        struct wl_surface* surface,
        wl_fixed_t x,
        wl_fixed_t y,
        struct wl_data_offer* id);

    static void leave(void* data, struct wl_data_device* wl_data_device);

    static void motion(
        void* data,
        struct wl_data_device* wl_data_device,
        uint32_t time,
        wl_fixed_t x,
        wl_fixed_t y);

    static void drop(void* data, struct wl_data_device* wl_data_device);

    static void selection(
        void* data,
        struct wl_data_device* wl_data_device,
        struct wl_data_offer* id);


    static ActiveListeners active_listeners;
    constexpr static wl_data_device_listener thunks =
        {
            &data_offer,
            &enter,
            &leave,
            &motion,
            &drop,
            &selection
        };
};

ActiveListeners DataDeviceListener::active_listeners;
constexpr wl_data_device_listener DataDeviceListener::thunks;
}

TEST_F(CopyCutPaste, given_source_offers_data_sink_sees_offer)
{
    struct MockDataDeviceListener : DataDeviceListener
    {
        using DataDeviceListener::DataDeviceListener;

        MOCK_METHOD2(data_offer, void(struct wl_data_device* wl_data_device, struct wl_data_offer* id));
    };

    // TODO set expectation


    MockDataDeviceListener listener{sink_data};
    EXPECT_CALL(listener, data_offer(_,_));

    wl_data_source_offer(source_data, any_mime_type);
    // TODO pass offer to sink
}

///////////////////////////////////////////
// Should probably end up in a helper TU //
namespace
{
//ActiveListeners DataDeviceListener::active_listeners;
//constexpr wl_data_device_listener DataDeviceListener::thunks;

void DataDeviceListener::data_offer(void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id)
{
    if (active_listeners.includes(data))
        static_cast<DataDeviceListener*>(data)->data_offer(wl_data_device, id);
}

void DataDeviceListener::enter(
    void* data,
    struct wl_data_device* wl_data_device,
    uint32_t serial,
    struct wl_surface* surface,
    wl_fixed_t x,
    wl_fixed_t y,
    struct wl_data_offer* id)
{
    if (active_listeners.includes(data))
        static_cast<DataDeviceListener*>(data)->enter(wl_data_device, serial, surface, x, y, id);
}

void DataDeviceListener::leave(void* data, struct wl_data_device* wl_data_device)
{
    if (active_listeners.includes(data))
        static_cast<DataDeviceListener*>(data)->leave(wl_data_device);
}

void DataDeviceListener::motion(
    void* data,
    struct wl_data_device* wl_data_device,
    uint32_t time,
    wl_fixed_t x,
    wl_fixed_t y)
{
    if (active_listeners.includes(data))
        static_cast<DataDeviceListener*>(data)->motion(wl_data_device, time, x, y);
}

void DataDeviceListener::drop(void* data, struct wl_data_device* wl_data_device)
{
    if (active_listeners.includes(data))
        static_cast<DataDeviceListener*>(data)->drop(wl_data_device);
}

void DataDeviceListener::selection(
    void* data,
    struct wl_data_device* wl_data_device,
    struct wl_data_offer* id)
{
    if (active_listeners.includes(data))
        static_cast<DataDeviceListener*>(data)->selection(wl_data_device, wl_data_device, id);
}

void DataDeviceListener::data_offer(struct wl_data_device* /*wl_data_device*/, struct wl_data_offer* /*id*/)
{
}

void DataDeviceListener::enter(
    struct wl_data_device* /*wl_data_device*/,
    uint32_t /*serial*/,
    struct wl_surface* /*surface*/,
    wl_fixed_t /*x*/,
    wl_fixed_t /*y*/,
    struct wl_data_offer* /*id*/)
{
}

void DataDeviceListener::leave(struct wl_data_device* /*wl_data_device*/)
{
}

void DataDeviceListener::motion(
    struct wl_data_device* /*wl_data_device*/,
    uint32_t /*time*/,
    wl_fixed_t /*x*/,
    wl_fixed_t /*y*/)
{
}

void DataDeviceListener::drop(struct wl_data_device* /*wl_data_device*/)
{
}

void DataDeviceListener::selection(
    struct wl_data_device* /*wl_data_device*/,
    struct wl_data_offer* /*id*/)
{
}
}

