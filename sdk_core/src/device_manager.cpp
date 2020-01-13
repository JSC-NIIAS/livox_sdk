//
// The MIT License (MIT)
//
// Copyright (c) 2019 Livox. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "device_manager.h"
#include <boost/atomic.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/locks.hpp>
#include <vector>
#include "base/logging.h"
#include "command_handler/command_handler.h"
#include "command_handler/command_impl.h"
#include "data_handler/data_handler.h"

using boost::lock_guard;
using boost::mutex;
using std::string;
using std::vector;

//=======================================================================================

namespace livox {

//=======================================================================================
inline bool IsHub( const uint8_t mode )
{
    return mode == kDeviceTypeHub;
}
//=======================================================================================

//=======================================================================================
inline bool IsLidar( const uint8_t mode )
{
    return ( mode == kDeviceTypeLidarTele ) ||
            ( mode == kDeviceTypeLidarMid40 ) ||
            ( mode == kDeviceTypeLidarHorizon );
}
//=======================================================================================

//=======================================================================================
DeviceManager::DeviceManager()
    : _mem_pool     ( NULL            )
    , _device_mode  ( kDeviceModeNone )
    , _connected_cb ( NULL            )
    , _broadcast_cb ( NULL            )
{

}
//=======================================================================================

//=======================================================================================
bool DeviceManager::Init()
{
    apr_status_t rv = apr_pool_create( &_mem_pool, NULL );

    if ( rv != APR_SUCCESS )
        return false;

    return true;
}
//=======================================================================================

//=======================================================================================
void DeviceManager::Uninit()
{
    _broadcast_cb = NULL;
    _connected_cb = NULL;
    _device_mode = kDeviceModeNone;

    lock_guard<mutex> lock(_mutex);

    for ( auto ite = _devices.begin(); ite != _devices.end(); ++ite )
        ite->clear();

    if ( _mem_pool )
    {
        apr_pool_destroy( _mem_pool );
        _mem_pool = NULL;
    }
}
//=======================================================================================

//=======================================================================================
bool DeviceManager::AddDevice( const DeviceInfo& device )
{
    if ( _device_mode == kDeviceModeNone )
    {
        if ( IsHub( device.type ) )
            _device_mode = kDeviceModeHub;

        else if ( IsLidar( device.type ) )
            _device_mode = kDeviceModeLidar;
    }

    lock_guard<mutex> lock( _mutex );

    if ( device.handle < _devices.size() )
    {
        DetailDeviceInfo &info = _devices[ device.handle ];
        info.connected = true;
        info.info = device;
    }

    return true;
}
//=======================================================================================

//=======================================================================================
void DeviceManager::RemoveDevice( const uint8_t handle )
{
    lock_guard<mutex> lock( _mutex );

    if ( _device_mode == kDeviceModeHub )
        for ( auto ite = _devices.begin(); ite != _devices.end(); ++ite )
            ite->connected = false;

    else if ( _device_mode == kDeviceModeLidar )
        if ( handle < _devices.size() )
            _devices[handle].connected = false;

    LOG_INFO( " Device {} removed ", (uint16_t) handle );
}
//=======================================================================================

//=======================================================================================
void DeviceManager::BroadcastDevices( const BroadcastDeviceInfo* info )
{
    if ( _broadcast_cb )
        _broadcast_cb( info );
}
//=======================================================================================

//=======================================================================================
void DeviceManager::UpdateDevices( const DeviceInfo& device, const DeviceEvent& type)
{
    if ( _device_mode == kDeviceModeLidar && _connected_cb )
        _connected_cb( &device, type );

    if ( _device_mode == kDeviceModeHub )
    {
        if ( type == kEventHubConnectionChange )
        {
            LOG_DEBUG( "Send Query lidars command" );

            command_handler().SendCommand( kHubDefaultHandle,
                                           kCommandSetHub,
                                           kCommandIDHubQueryLidarInformation,
                                           NULL,
                                           0,
                                           MakeCommandCallback<DeviceManager,
                                           HubQueryLidarInformationResponse>(
                                               this,
                                               &DeviceManager::HubLidarInfomationCallback ) );
        }

        else if ( _connected_cb )
            _connected_cb( &device, type );
    }
}
//=======================================================================================

//=======================================================================================
void DeviceManager::HubLidarInfomationCallback( const livox_status status,
                                                const uint8_t,
                                                HubQueryLidarInformationResponse* response )
{
    if ( status != kStatusSuccess )
    {
        LOG_ERROR( "Failed to query lidars information connected to hub." );
        return;
    }

    lock_guard<mutex> lock( _mutex );

    for ( auto ite = _devices.begin(); ite != _devices.end(); ++ite )
        if ( ite->info.handle != kHubDefaultHandle )
            ite->connected = false;

    for ( auto i = 0; i < response->count; i++)
    {
        auto index = ( response->device_info_list[i].slot - 1 ) * 3 +
                     response->device_info_list[i].id - 1;

        if ( index < _devices.size() )
        {
            DetailDeviceInfo &info = _devices[index];
            info.connected = true;
            strncpy( info.info.broadcast_code,
                     response->device_info_list[i].broadcast_code,
                     sizeof( info.info.broadcast_code ) );

            info.info.handle = index;
        }
    }

    if ( _connected_cb )
        _connected_cb( &( _devices[kHubDefaultHandle].info ),
                       kEventHubConnectionChange );
}
//=======================================================================================

//=======================================================================================
DeviceMode DeviceManager::device_mode() const
{
    return _device_mode;
}
//=======================================================================================

//=======================================================================================
void DeviceManager::GetConnectedDevices( vector<DeviceInfo> &devices )
{
    lock_guard<mutex> lock( _mutex );

    for ( auto ite = _devices.begin(); ite != _devices.end(); ++ite )
        if ( ite->connected == true )
            devices.push_back( ite->info );
}
//=======================================================================================

//=======================================================================================
bool DeviceManager::AddListeningDevice( const string& broadcast_code,
                                        const DeviceMode mode,
                                        uint8_t& handle )
{
    lock_guard<mutex> lock(_mutex);

    if ( mode == kDeviceModeHub )
    {
        handle = kHubDefaultHandle;

        _devices[kHubDefaultHandle].connected = false;

        strncpy(_devices[kHubDefaultHandle].info.broadcast_code,
                broadcast_code.c_str(),
                sizeof( _devices[kHubDefaultHandle].info.broadcast_code ) );

        _devices[kHubDefaultHandle].info.handle = kHubDefaultHandle;

        return true;
    }

    for ( auto ite = _devices.begin(); ite != _devices.end(); ++ite )
    {
        if ( strlen( ite->info.broadcast_code ) == 0 )
        {
            handle = ite - _devices.begin();
            ite->connected = false;
            strncpy( ite->info.broadcast_code,
                     broadcast_code.c_str(),
                     sizeof( ite->info.broadcast_code ) );
            ite->info.handle = handle;

            return true;
        }

        else if ( strncmp( ite->info.broadcast_code,
                           broadcast_code.c_str(),
                           sizeof( ite->info.broadcast_code ) ) == 0 )
        {
            handle = ite->info.handle;
            return true;
        }
    }

    return false;
}
//=======================================================================================

//=======================================================================================
bool DeviceManager::FindDevice( const uint8_t handle, DeviceInfo& info )
{
    lock_guard<mutex> lock(_mutex);

    if ( handle >= _devices.size() )
        return false;

    info = _devices[handle].info;

    return true;
}
//=======================================================================================

//=======================================================================================
bool DeviceManager::FindDevice( const string& broadcast_code, DeviceInfo& info )
{
    lock_guard<mutex> lock(_mutex);

    for ( auto ite = _devices.begin(); ite != _devices.end(); ++ite )
        if ( ite->info.broadcast_code == broadcast_code )
        {
            info = ite->info;
            return true;
        }

    return false;
}
//=======================================================================================

//=======================================================================================
bool DeviceManager::IsDeviceConnected( const uint8_t handle )
{
    lock_guard<mutex> lock( _mutex );

    if ( handle >= _devices.size() )
        return false;

    return _devices[handle].connected;
}
//=======================================================================================

//=======================================================================================
void DeviceManager::SetDeviceConnectedCallback( const InfoEventFoo& cb )
{
    _connected_cb = cb;
}
//=======================================================================================

//=======================================================================================
void DeviceManager::SetDeviceBroadcastCallback( const BrDeviceInfoFoo& cb )
{
    _broadcast_cb = cb;
}
//=======================================================================================

//=======================================================================================
void DeviceManager::UpdateDeviceState( const uint8_t handle,
                                       const HeartbeatResponse &response )
{
    if ( handle >= _devices.size() )
        return;

    bool update = false;

    DeviceInfo &info = _devices[ handle ].info;

    if ( info.state != response.state )
    {
        LOG_INFO( " Update State to {}, device connect {}",
                  (uint16_t) response.state,
                  _devices[handle].connected );
        info.state = static_cast<LidarState>( response.state );
        update = true;
    }

    if ( info.feature != response.feature )
    {
        LOG_INFO( " Update feature to {}, device connect {}",
                  (uint16_t) response.feature,
                  _devices[handle].connected );
        info.feature = static_cast<LidarFeature>( response.feature );
        update = true;
    }

    if ( ( response.state == kLidarStateInit ) &&
         ( info.status.progress != response.error_union.progress ) )
    {
        LOG_INFO( " Update progress {}, device connect {}",
                  (uint16_t) response.error_union.progress,
                  _devices[handle].connected );
        info.status.progress = response.error_union.progress;
        update = true;
    }

    else if ( info.status.status_code.error_code !=
              response.error_union.status_code.error_code )
    {
        info.status.status_code.error_code = response.error_union.status_code.error_code;
        update = true;
    }

    if ( ( _devices[handle].connected && update == true ) &&  _connected_cb )
        _connected_cb( &info, kEventStateChange );
}
//=======================================================================================

//=======================================================================================
bool DeviceManager::IsLidarMid40( const uint8_t handle )
{
    DeviceInfo lidar_info;

    bool found = device_manager().FindDevice( handle, lidar_info );

    if ( found && lidar_info.type == kDeviceTypeLidarMid40 )
        return true;

    return false;
}
//=======================================================================================

//=======================================================================================
DeviceManager& device_manager()
{
    static DeviceManager lidar_manager;
    return lidar_manager;
}
//=======================================================================================

//=======================================================================================
void DeviceFound( const DeviceInfo& lidar_data )
{
    device_manager().AddDevice( lidar_data );
    command_handler().AddDevice( lidar_data );
    data_handler().AddDevice( lidar_data );

    if ( device_manager().device_mode() == kDeviceModeHub )
        device_manager().UpdateDevices( lidar_data, kEventHubConnectionChange );

    else
        device_manager().UpdateDevices( lidar_data, kEventConnect );
}
//=======================================================================================

//=======================================================================================
void DeviceRemove( const uint8_t handle, const DeviceEvent device_event )
{
    DeviceInfo info;

    bool found = device_manager().FindDevice( handle, info );

    device_manager().RemoveDevice( handle );
    command_handler().RemoveDevice( handle );
    data_handler().RemoveDevice( handle );

    if ( found )
        device_manager().UpdateDevices(info, device_event);
}
//=======================================================================================

}  // namespace livox
