#include "vlivox_hunter.h"

#include "vlog.h"
#include "vcat.h"
#include "vbyte_buffer.h"
#include "vbyte_buffer_view.h"

#include <QNetworkDatagram>
#include <QIODevice>

//=======================================================================================

static constexpr auto heartbeat_ms = 800;

using namespace livox;

//  Copied from sdk_protocol.cpp
//const uint8_t kSdkProtocolSof = 0xAA;
//const uint32_t kSdkPacketCrcSize = 4;          // crc32

static bool check_crc16( const char* buf, uint len );
//static uint16_t calc_crc16( const char* buf, uint len );
//static uint32_t calc_crc32( const char* buf, uint len );

//=======================================================================================
VLivox_Hunter::VLivox_Hunter( const QString broadcast_code,
                              const QString livox_ip,
                              QObject* parent )
    : QObject         ( parent         )
    , _broadcast_code ( broadcast_code )
    , _livox_ip       ( livox_ip       )
{
    _heart_timer = new QTimer();
    _heart_timer->start( heartbeat_ms );

    auto ok = _recv.bind( local_data_port );

    if ( !ok )
        throw verror << "Cannot bind to port "
                     << local_data_port
                     << " (for listen broadcast livoxes)";

    ok = _cmd.bind( local_cmd_port );

    if ( !ok )
        throw verror << "Cannot bind to port "
                     << local_cmd_port
                     << " (for listen broadcast livoxes)";

    //      объект1, сигнал, объект2, действие
    connect( &_recv, &QUdpSocket::readyRead, this, &VLivox_Hunter::_on_udp );
    connect( this, &VLivox_Hunter::receive, this, &VLivox_Hunter::_to_udp );
}
//=======================================================================================
void VLivox_Hunter::_on_udp()
{
    while ( _recv.hasPendingDatagrams() )
        _read_one_udp();
}
//=======================================================================================
void VLivox_Hunter::_read_one_udp()
{
    auto dgram = _recv.receiveDatagram();

    vdeb << "Receive...";

    Info res;

    if ( _livox_ip.size() == 0 )
        res.address = dgram.senderAddress();
    else
        res.address = QHostAddress( _livox_ip );

    auto ba_data = dgram.data();
    vbyte_buffer_view view( ba_data.data(), uint( ba_data.size() ) );

    res.sdk.sof = view.u8();
    res.sdk.version = view.u8();
    res.sdk.length = view.u16_LE();
    res.sdk.packet_type = view.u8();
    res.sdk.seq_num = view.u16_LE();
    res.sdk.preamble_crc = view.u16_LE();
    (void) res.sdk.preamble_crc;
    res.sdk.cmd_set = view.u8();
    res.sdk.cmd_id = view.u8();

    _seq_num = res.sdk.seq_num;

    constexpr uint preambul_sz = sizeof( livox::SdkPreamble );

    if ( !check_crc16( ba_data.data(), preambul_sz ) )
    {
        vwarning << "Bad preambul CRC16 in bcast message";
        return;
    }

    res.broadcast_code = view.string( kBroadcastCodeSize );

    if ( res.broadcast_code.compare( 0,
                                     kBroadcastCodeSize - 1,
                                     _broadcast_code.toStdString() ) != 0 )
    {
        vwarning << "It's not device with broadcast: " << _broadcast_code;
        return;
    }

    res.dev_type = view.u8();
    view.omit(2);

    if ( res.dev_type != kDeviceTypeLidarMid40 )
    {
        vwarning << "No Mid-40 connected device";
        return;
    }

    auto crc = view.u32_LE();
    uint packet_sz = res.sdk.length - kSdkPacketCrcSize;

    if ( crc != calc_crc32( ba_data.data(), packet_sz ) )
    {
        vwarning << "Bad preambul CRC32 in bcast message";
        return;
    }

    if ( res.sdk.sof != kSdkProtocolSof )
        throw verror.hex() << "sof != kSdkProtocolSof (" << uint( res.sdk.sof ) << ")";

    assert( view.finished() );
    assert( res.sdk.length == 34 );

    if ( res.sdk.cmd_set != livox::kCommandSetGeneral )
        throw verror << "cmd_set != livox::kCommandSetGeneral";

    if ( res.sdk.cmd_id != livox::kCommandIDGeneralBroadcast )
        throw verror << "cmd_id != livox::kCommandIDGeneralBroadcast";

    _control.cmd_set = livox::kCommandSetGeneral;

    if ( _control.cmd_id == livox::kCommandIDGeneralBroadcast )
        _control.cmd_id = livox::kCommandIDGeneralHandshake;

    _control.packet_type = livox::kCommandTypeAck;

    emit receive( res, _control );
}
//=======================================================================================

//=======================================================================================
void VLivox_Hunter::_to_udp()
{
    _write_one_udp();
}
//=======================================================================================
void VLivox_Hunter::_write_one_udp()
{
    QByteArray dgram;

    Info res;

    HandshakeRequest handshake_req;
    {
        QHostAddress adr( _livox_ip );

        handshake_req.ip_addr = 1318496448; //192.168.150.240
        //        handshake_req.ip_addr = adr.toString().toInt();
        handshake_req.cmd_port = local_data_port;
        handshake_req.data_port = local_cmd_port;
        handshake_req.sensor_port = local_cmd_port;
    }

    CommPacket packet;
    {
        packet.packet_type = kCommandTypeAck;
        packet.seq_num = _seq_num;
        packet.cmd_set = kCommandSetGeneral;
        packet.cmd_code = kCommandIDGeneralHandshake;
        packet.data_len = sizeof( handshake_req );
        packet.data = (uint8_t *)&handshake_req;
    }

    dgram.resize( kMaxCommandBufferSize + 1 );


    SdkPacket *sdk_packet = (SdkPacket *)dgram.data();
    {
        sdk_packet->sof = kSdkProtocolSof;
        sdk_packet->length = packet.data_len +
                             sizeof( livox::SdkPacket ) - 1 + kSdkPacketCrcSize;
        sdk_packet->version = livox::kSdkVer0;
        sdk_packet->packet_type = _control.packet_type;
        sdk_packet->seq_num = packet.seq_num & 0xFFFF;
        sdk_packet->preamble_crc = calc_crc16( dgram.data(), sizeof(SdkPreamble) - kSdkPacketPreambleCrcSize );
        sdk_packet->cmd_set = _control.cmd_set;
        sdk_packet->cmd_id = _control.cmd_id;

        memcpy( sdk_packet->data, packet.data, packet.data_len );

        uint32_t crc = calc_crc32( dgram.data(), sdk_packet->length - kSdkPacketCrcSize );
        dgram.data()[sdk_packet->length - 4] = crc & 0xFF;
        dgram.data()[sdk_packet->length - 3] = (crc >> 8) & 0xFF;
        dgram.data()[sdk_packet->length - 2] = (crc >> 16) & 0xFF;
        dgram.data()[sdk_packet->length - 1] = (crc >> 24) & 0xFF;

//        vdeb << vbyte_buffer( {reinterpret_cast<const char *>( dgram.data() ), sdk_packet->length } ).to_Hex();

        _cmd.writeDatagram( dgram.data(), QHostAddress( _livox_ip ), livox_port );

        _control.packet_type = kCommandTypeCmd;
        _control.cmd_id = kCommandIDGeneralHeartbeat;

        vdeb << "Control command sended! ";
    }
}
//=======================================================================================


//=======================================================================================
//      CRC-s
//=======================================================================================
static bool check_crc16( const char* buf, uint len )
{
    static constexpr quint16 seed_16 = 0x4c49;
    static FastCRC16 fast_crc16 { seed_16 };

    auto ptr = static_cast<const uchar*>( static_cast<const void*>( buf ) );

    return fast_crc16.mcrf4xx_calc( ptr, uint16_t( len ) ) == 0;
}
//=======================================================================================
uint16_t calc_crc16( const char* buf, uint len )
{
    static constexpr quint32 seed_16 = 0x4c49;
    static FastCRC16 fast_crc16 { seed_16 };

    auto ptr = static_cast<const uchar*>( static_cast<const void*>( buf ) );

    return fast_crc16.mcrf4xx_calc( ptr, uint16_t( len ) );
}
//=======================================================================================
uint32_t calc_crc32( const char* buf, uint len )
{
    static constexpr quint32 seed_32 = 0x564f580a;
    static FastCRC32 fast_crc32 { seed_32 };

    auto ptr = static_cast<const uchar*>( static_cast<const void*>( buf ) );

    return fast_crc32.crc32_calc( ptr, uint16_t( len ) );
}
//=======================================================================================

//=======================================================================================
std::string Info::str() const
{
    return vcat( "ID=",         broadcast_code,
                 ", Address: ", address,
                 ", port: ",    port,
                 ", sof: ", int( sdk.sof ),
                 ", Version: ", int( sdk.version ),
                 ", Length: ", int( sdk.length ),
                 ", Packet_type: ", int( sdk.packet_type ),
                 ", Seq_num: ", int( sdk.seq_num ),
                 ", Preamble_CRC: ", int( sdk.preamble_crc ),
                 ", CMD Set: ", int( sdk.cmd_set ),
                 ", CMD ID: ", int( sdk.cmd_id )
                 );
}
//=======================================================================================
