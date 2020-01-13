#include "device.h"

using namespace livox;
using namespace std;

//=======================================================================================
Device::Device()
    : _mem_pool     ( NULL            )
    , _device_mode  ( kDeviceModeNone )
    , _connected_cb ( NULL            )
    , _broadcast_cb ( NULL            )
    , _sock         ( NULL            )
    , _loop         ( NULL            )
    , _comm_port    ( NULL            )
{

}
//=======================================================================================

//=======================================================================================
Device::Device( const char* broadcast_code )
    : _mem_pool     ( NULL            )
    , _device_mode  ( kDeviceModeNone )
    , _connected_cb ( NULL            )
    , _broadcast_cb ( NULL            )
    , _sock         ( NULL            )
    , _loop         ( NULL            )
    , _comm_port    ( NULL            )
{
    _code_list.push_back( broadcast_code );
}
//=======================================================================================

//=======================================================================================
Device::Device( const std::vector<string>& broadcast_codes )
    : _mem_pool     ( NULL            )
    , _device_mode  ( kDeviceModeNone )
    , _connected_cb ( NULL            )
    , _broadcast_cb ( NULL            )
    , _sock         ( NULL            )
    , _loop         ( NULL            )
    , _comm_port    ( NULL            )
{
    _code_list = broadcast_codes;
}
//=======================================================================================

//=======================================================================================
bool Device::Init()
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
