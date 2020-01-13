#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>

#include <vector>
#include <string>

#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/locks.hpp>

#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_pools.h"

#include "base/io_thread.h"
#include "base/noncopyable.h"
#include "base/logging.h"
#include "base/network_util.h"
#include "comm/comm_port.h"
#include "arpa/inet.h"

#include "livox_def.h"
#include "livox_sdk.h"

namespace livox
{

using InfoEventFoo = boost::function<void( const DeviceInfo *, DeviceEvent )>;
using BrDeviceInfoFoo = boost::function<void( const BroadcastDeviceInfo *info )>;
using TupleAprDevice = boost::tuple< apr_pool_t *, apr_time_t, DeviceInfo >;
using ConnectingDeviceMap = std::map< apr_socket_t *, TupleAprDevice >;

//=======================================================================================
/** Maximum number of connected devices supported. */
static constexpr auto kMaxConnectedDeviceNum = 32;

/** Default handle value of hub. */
static constexpr auto kHubDefaultHandle = kMaxConnectedDeviceNum - 1;
//=======================================================================================

//=======================================================================================
/**
 * Device connected mode.
 */
typedef enum {
    kDeviceModeNone = 0, /**< no devices connected. */
    kDeviceModeHub = 1,  /**< connected with hub. */
    kDeviceModeLidar = 2 /**< connected with lidar. */
} DeviceMode;
//=======================================================================================

typedef struct _DetailDeviceInfo
{
    bool connected;
    DeviceInfo info;

    _DetailDeviceInfo()
    {
        connected = false;
    }

    _DetailDeviceInfo( const bool _connected,
                       const char *broadcast_code,
                       const uint8_t handle,
                       const uint8_t port,
                       const uint8_t id,
                       const uint32_t type,
                       const uint16_t data_port,
                       const uint16_t cmd_port,
                       const char* ip )
    {
        connected = _connected;
        strncpy( info.broadcast_code, broadcast_code, sizeof( info.broadcast_code ) );
        info.handle = handle;
        info.slot = port;
        info.id = id;
        info.type = type;
        info.data_port = data_port;
        info.cmd_port = cmd_port;
        strncpy( info.ip, ip, sizeof( info.ip ) );
    }

    void clear()
    {
        connected = false;
        memset( info.broadcast_code, 0, sizeof( info.broadcast_code ) );
        info.handle = 0;
        info.slot = 0;
        info.id = 0;
        info.type = 0;
        info.data_port = 0;
        info.cmd_port = 0;
        memset( info.ip, 0, sizeof( info.ip ) );
    }

} DetailDeviceInfo;

using DeviceContainer = boost::array<DetailDeviceInfo, kMaxConnectedDeviceNum>;

//=======================================================================================
class Device
{
    using VecStr = std::vector<std::string>;

public:

    Device();
    Device( const char* broadcast_code );
    Device( const VecStr& broadcast_codes );

    bool Init();

    //-----------------------------------------------------------------------------------

private:

    bool _auto_connect_mode;
    VecStr _code_list;
    DeviceContainer _devices;
    apr_pool_t *_mem_pool;
    DeviceMode _device_mode;
    InfoEventFoo _connected_cb;
    BrDeviceInfoFoo _broadcast_cb;

    boost::mutex _mutex;

    /** broadcast listening port number. */
    static constexpr auto kListenPort = 55000;
    /** command port number start offset. */
    static constexpr auto kCmdPortOffset = 500;
    /** data port number start offset. */
    static constexpr auto kDataPortOffset = 1000;
    /** sensor port number start offset. */
    static constexpr auto kSensorPortOffset = 1000;
    static uint16_t _port_count;
    apr_socket_t *_sock;
    IOLoop *_loop;
    boost::scoped_ptr<CommPort> _comm_port;
    ConnectingDeviceMap _connecting_devices;
};
//=======================================================================================
}

#endif // DEVICE_H
