#include "vlivox_hunter.h"

#include "vlog.h"
#include "vbyte_buffer_view.h"
#include <QNetworkDatagram>

#include "comm/sdk_protocol.h"
#include "livox_def.h"
#include "vcat.h"

#include "command_handler/command_impl.h"

//  Copied from sdk_protocol.cpp
const uint8_t kSdkProtocolSof = 0xAA;
const uint32_t kSdkPacketCrcSize = 4;          // crc32

static bool check_crc16( const char* buf, uint len );
static uint32_t calc_crc32( const char* buf, uint len );

//=======================================================================================
VLivox_Hunter::VLivox_Hunter( QObject *parent )
    : QObject( parent )
{
    auto ok = _bcast.bind( _port );
    if ( !ok )
    {
        throw verror << "Cannot bind to port " << _port
                     << " (for listem broadcast livoxes)";
    }

    connect( &_bcast, &QUdpSocket::readyRead, this, &VLivox_Hunter::_on_udp );
}
//=======================================================================================
void VLivox_Hunter::_on_udp()
{
    while ( _bcast.hasPendingDatagrams() )
        _read_one_udp();
}
//=======================================================================================
void VLivox_Hunter::_read_one_udp()
{
    auto dgram = _bcast.receiveDatagram();

    Info res;
    res.address = dgram.senderAddress();
    res.port = dgram.senderPort();

    auto ba_data = dgram.data();
    vbyte_buffer_view view( ba_data.data(), uint(ba_data.size()) );

    auto sof            = view.u8();
    res.sdk.version     = view.u8();
    auto length         = view.u16_LE();
    res.sdk.packet_type = view.u8();
    res.sdk.seq_num     = view.u16_LE();
    auto preamble_crc   = view.u16_LE(); (void)preamble_crc;
    auto cmd_set        = view.u8();
    auto cmd_id         = view.u8();

    constexpr uint preambul_sz = sizeof(livox::SdkPreamble);
    if ( !check_crc16(ba_data.data(),preambul_sz) )
    {
        vwarning << "Bad preambul CRC16 in bcast message";
        return;
    }

    res.broadcast_code = view.string( kBroadcastCodeSize );

    //BroadcastDeviceInfo bb;
    res.dev_type = view.u8();
    view.omit(2);

    auto crc = view.u32_LE();
    uint packet_sz = length - kSdkPacketCrcSize;
    if ( crc != calc_crc32(ba_data.data(), packet_sz) )
    {
        vwarning << "Bad preambul CRC32 in bcast message";
        return;
    }

    if ( sof != kSdkProtocolSof )
        throw verror.hex() << "sof != kSdkProtocolSof (" << uint(sof) << ")";

    assert( view.finished() );
    assert( length == 34 );

    if ( cmd_set != livox::kCommandSetGeneral )
        throw verror << "cmd_set != livox::kCommandSetGeneral";

    if ( cmd_id != livox::kCommandIDGeneralBroadcast )
        throw verror << "cmd_id != livox::kCommandIDGeneralBroadcast";

    emit trapped( res );


}
//=======================================================================================


//=======================================================================================
//      CRC-s
//=======================================================================================
static bool check_crc16( const char* buf, uint len )
{
    static constexpr quint16 seed_16 = 0x4c49;
    static FastCRC16 fast_crc16 { seed_16 };

    auto ptr = static_cast<const uchar*>(
                    static_cast<const void*>(buf) );

    return fast_crc16.mcrf4xx_calc( ptr, uint16_t(len) ) == 0;
}
//=======================================================================================
static uint32_t calc_crc32( const char* buf, uint len )
{
    static constexpr quint32 seed_32 = 0x564f580a;
    static FastCRC32 fast_crc32 { seed_32 };

    auto ptr = static_cast<const uchar*>(
                    static_cast<const void*>(buf) );

    return fast_crc32.crc32_calc( ptr, uint16_t(len) );
}
//=======================================================================================


std::string VLivox_Hunter::Info::str() const
{
    return vcat("ID=", broadcast_code,
                ", ver=",         int(sdk.version),
                ", type=",       int(sdk.packet_type),
                ", seqN=",       int(sdk.seq_num),
                ", dev-type=",   int(dev_type),

                " [", address, "_", port, "]"
                );
}
