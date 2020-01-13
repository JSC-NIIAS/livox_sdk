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

#include "device_discovery.h"
#include <algorithm>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/locks.hpp>
#include <iostream>
#include <vector>
#include "apr_network_io.h"
#include "apr_pools.h"
#ifdef WIN32
#include "winsock.h"
#else
#include "arpa/inet.h"
#endif
#include "base/logging.h"
#include "base/network_util.h"
#include "command_handler/command_impl.h"
#include "device_manager.h"
#include "livox_def.h"

using boost::tuple;
using std::string;
using std::vector;

//=======================================================================================

namespace livox {

//=======================================================================================

uint16_t DeviceDiscovery::_port_count = 0;

//=======================================================================================
DeviceDiscovery::DeviceDiscovery()
    : _sock      ( NULL )
    , _mem_pool  ( NULL )
    , _loop      ( NULL )
    , _comm_port ( NULL )
{

}
//=======================================================================================

//=======================================================================================
bool DeviceDiscovery::Init()
{
    apr_status_t rv = apr_pool_create( &_mem_pool, NULL );

    if ( rv != APR_SUCCESS )
    {
        LOG_ERROR( PrintAPRStatus( rv ) );
        return false;
    }

    if ( _comm_port == NULL )
        _comm_port.reset( new CommPort() );

    return true;
}
//=======================================================================================

//=======================================================================================
bool DeviceDiscovery::Start( IOLoop* loop )
{
    if ( loop == NULL )
        return false;

    _loop = loop;
    _sock = util::CreateBindSocket( kListenPort, _mem_pool, true );

    if ( _sock == NULL )
    {
        LOG_ERROR( "DeviceDiscovery Create Socket Failed" );
        return false;
    }

    _loop->AddDelegate( _sock, this );

    return true;
}
//=======================================================================================

//=======================================================================================
void DeviceDiscovery::OnData( apr_socket_t* sock, void * )
{
    apr_sockaddr_t addr;
    uint32_t buf_size = 0;
    uint8_t *cache_buf = _comm_port->FetchCacheFreeSpace( &buf_size );
    apr_size_t size = buf_size;
    apr_status_t rv = apr_socket_recvfrom( &addr,
                                           sock,
                                           0,
                                           reinterpret_cast<char*>( cache_buf ),
                                           &size );

    _comm_port->UpdateCacheWrIdx( size );

    if ( rv != APR_SUCCESS )
    {
        LOG_WARN( " Receive Failed {}", PrintAPRStatus(rv) );
        return;
    }

    CommPacket packet;
    memset( &packet, 0, sizeof( packet ) );

    while ( ( kParseSuccess == _comm_port->ParseCommStream( &packet ) ) )
    {
        if ( packet.cmd_set == kCommandSetGeneral &&
             packet.cmd_code == kCommandIDGeneralBroadcast )
            OnBroadcast( packet, &addr );

        else if ( packet.cmd_set == kCommandSetGeneral &&
                  packet.cmd_code == kCommandIDGeneralHandshake )
        {
            if ( _connecting_devices.find( sock ) == _connecting_devices.end() )
                continue;

            DeviceInfo info = boost::get<2>( _connecting_devices[ sock ] );

            _loop->RemoveDelegate( sock, this );
            apr_socket_close( sock );
            apr_pool_destroy( boost::get<0>( _connecting_devices[ sock ] ) );
            _connecting_devices.erase( sock );

            if (packet.data == NULL)
                continue;

            if ( *( uint8_t * )packet.data == 0 )
            {
                LOG_INFO( "New Device" );
                LOG_INFO( "Handle: {}", static_cast<uint16_t>( info.handle ) );
                LOG_INFO( "Broadcast Code: {}", info.broadcast_code );
                LOG_INFO( "Type: {}", info.type );
                LOG_INFO( "IP: {}", info.ip );
                LOG_INFO( "Command Port: {}", info.cmd_port );
                LOG_INFO( "Data Port: {}", info.data_port );

                DeviceFound(info);
            }
        }
    }
}
//=======================================================================================

//=======================================================================================
void DeviceDiscovery::OnTimer( const apr_time_t now )
{
    ConnectingDeviceMap::iterator ite = _connecting_devices.begin();

    while ( ite != _connecting_devices.end() )
    {
        TupleAprDevice &device_tuple = ite->second;

        if ( now - boost::get<1>( device_tuple ) > apr_time_from_msec( 500 ) )
        {
            _loop->RemoveDelegate( ite->first, this );
            apr_socket_close( ite->first );
            apr_pool_destroy( boost::get<0>( device_tuple ) );
            _connecting_devices.erase( ite++ );
        }

        else
            ++ite;
    }
}
//=======================================================================================

//=======================================================================================
void DeviceDiscovery::Uninit()
{
    if ( _sock )
    {
        _loop->RemoveDelegate( _sock, this );
        apr_socket_close( _sock );
        _sock = NULL;
    }

    if ( _comm_port )
        _comm_port.reset( NULL );

    if ( _mem_pool )
    {
        apr_pool_destroy( _mem_pool );
        _mem_pool = NULL;
    }
}
//=======================================================================================

//=======================================================================================
void DeviceDiscovery::OnBroadcast( const CommPacket& packet, apr_sockaddr_t* addr )
{
    if ( packet.data == NULL )
        return;

    BroadcastDeviceInfo device_info;
    memcpy( (void*)( &device_info ),
            (void*)( packet.data ),
            ( sizeof( BroadcastDeviceInfo ) - sizeof( device_info.ip ) ) );

    string broadcast_code = device_info.broadcast_code;
    LOG_INFO( " Broadcast broadcast code: {}", broadcast_code );

    char ip[16];
    memset( &ip, 0, sizeof(ip) );

    apr_status_t rv = apr_sockaddr_ip_getbuf( ip, sizeof(ip), addr );

    if ( rv != APR_SUCCESS )
    {
        LOG_ERROR( PrintAPRStatus(rv) );
        return;
    }

    strncpy( device_info.ip, ip, sizeof( device_info.ip ) );

    device_manager().BroadcastDevices( &device_info );

    DeviceInfo lidar_info;
    bool found = device_manager().FindDevice( broadcast_code, lidar_info );

    if ( !found )
        LOG_INFO("Broadcast code : {} not add to connect", broadcast_code);

    if ( !found || device_manager().IsDeviceConnected( lidar_info.handle ) )
        return;

    _port_count++;

    strncpy( lidar_info.broadcast_code,
             broadcast_code.c_str(),
             sizeof( lidar_info.broadcast_code ) );

    lidar_info.cmd_port = kListenPort + kCmdPortOffset + _port_count;
    lidar_info.data_port = kListenPort + kDataPortOffset + _port_count;
    lidar_info.sensor_port = kListenPort + kSensorPortOffset + _port_count;
    lidar_info.type = device_info.dev_type;
    lidar_info.state = kLidarStateUnknown;
    lidar_info.feature = kLidarFeatureNone;
    lidar_info.status.progress = 0;

    strncpy( lidar_info.ip, ip, sizeof( lidar_info.ip ) );

    apr_pool_t *pool = NULL;
    rv = apr_pool_create( &pool, _mem_pool );

    if ( rv != APR_SUCCESS )
    {
        LOG_ERROR( PrintAPRStatus(rv) );
        return;
    }

    apr_socket_t *cmd_sock = util::CreateBindSocket( lidar_info.cmd_port, pool );

    if ( cmd_sock == NULL )
    {
        apr_pool_destroy( pool );
        pool = NULL;
        return;
    }

    _loop->AddDelegate( cmd_sock, this );

    OnTimer( apr_time_now() );

    boost::get<0>( _connecting_devices[ cmd_sock ] ) = pool;
    boost::get<2>( _connecting_devices[ cmd_sock ] ) = lidar_info;

    bool result = false;

    do
    {
        HandshakeRequest handshake_req;

        uint32_t local_ip = 0;

        if ( util::FindLocalIp( addr->sa.sin, local_ip ) == false )
        {
            result = false;
            LOG_INFO("LocalIp and DeviceIp are not in same subnet");
            break;
        }

        LOG_INFO( "LocalIP: {}", inet_ntoa(*( struct in_addr *)&local_ip ) );
        LOG_INFO( "Command Port: {}", lidar_info.cmd_port );
        LOG_INFO( "Data Port: {}", lidar_info.data_port );

        CommPacket packet;
        memset( &packet, 0, sizeof( packet ) );

        handshake_req.ip_addr = local_ip;
        handshake_req.cmd_port = lidar_info.cmd_port;
        handshake_req.data_port = lidar_info.data_port;
        handshake_req.sensor_port = lidar_info.sensor_port;

        packet.packet_type = kCommandTypeAck;
        packet.seq_num = CommandChannel::GenerateSeq();
        packet.cmd_set = kCommandSetGeneral;
        packet.cmd_code = kCommandIDGeneralHandshake;
        packet.data_len = sizeof(handshake_req);
        packet.data = (uint8_t *)&handshake_req;

        vector<uint8_t> buf( kMaxCommandBufferSize + 1 );
        apr_size_t o_len = kMaxCommandBufferSize;

        _comm_port->Pack( buf.data(), kMaxCommandBufferSize, (uint32_t *)&o_len, packet );

        rv = apr_socket_sendto( cmd_sock,
                                addr,
                                0,
                                reinterpret_cast<const char *>( buf.data() ),
                                &o_len );

        if ( rv != APR_SUCCESS )
        {
            result = false;
            break;
        }

        boost::get<1>( _connecting_devices[ cmd_sock ]) = apr_time_now();
        result = true;

    } while (0);

    if ( result == false )
    {
        _loop->RemoveDelegate( cmd_sock, this );
        apr_socket_close( cmd_sock );
        apr_pool_destroy( pool );
        _connecting_devices.erase( cmd_sock );
    }
}
//=======================================================================================

//=======================================================================================
DeviceDiscovery& device_discovery()
{
    static DeviceDiscovery discovery;
    return discovery;
}
//=======================================================================================

}  // namespace livox

//=======================================================================================
