#ifndef VLIVOX_HUNTER_H
#define VLIVOX_HUNTER_H

#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QHostAddress>

#include "livox_def.h"
#include "comm/sdk_protocol.h"
#include "command_handler/command_impl.h"

uint16_t calc_crc16( const char* buf, uint len );
uint32_t calc_crc32( const char* buf, uint len );

//=======================================================================================

struct Info
{
    QHostAddress address;

    int port;

    struct
    {
        uint8_t     sof;
        uint8_t     version;
        uint16_t    length;
        uint8_t     packet_type;
        uint16_t    seq_num;
        uint16_t    preamble_crc;
        uint8_t     cmd_set;
        uint8_t     cmd_id;

    } sdk;

    std::string broadcast_code;

    uint8_t dev_type;                        //< \ref DeviceType.

    uint16_t reserved;

    std::string str() const;
};

struct Control
{
    livox::CommandSet cmd_set      = livox::kCommandSetGeneral;
    livox::GeneralCommandID cmd_id = livox::kCommandIDGeneralBroadcast;
    livox::CommandType packet_type = livox::kCommandTypeCmd;
};

enum
{
    local_data_port = 55000,
    local_cmd_port = 56000,
    livox_port = 65000
};

//=======================================================================================

class VLivox_Hunter : public QObject
{
    Q_OBJECT

public:

    explicit VLivox_Hunter( const QString broadcast_code = "",
                            const QString livox_ip = "",
                            QObject* parent = nullptr );

    //-----------------------------------------------------------------------------------

signals:

    void receive( const Info&, const Control& );

    void send( const Info&, const Control& );

    //-----------------------------------------------------------------------------------

private:

    QUdpSocket _recv;
    QUdpSocket _cmd;

    QString _broadcast_code;
    QString _livox_ip;

    QTimer *_heart_timer;

    uint16_t _seq_num = 0;

    Control _control;

    //-----------------------------------------------------------------------------------

private slots:

    void _on_udp();
    void _read_one_udp();

    void _to_udp();
    void _write_one_udp();

};

//=======================================================================================

#endif // VLIVOX_HUNTER_H
