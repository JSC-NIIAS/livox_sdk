#ifndef VLIVOX_HUNTER_H
#define VLIVOX_HUNTER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

class VLivox_Hunter : public QObject
{
    Q_OBJECT
public:
    enum { _port = 55000 };

    //-----------------------------------------------------------------------------------
    struct Info
    {
        QHostAddress address;
        int port;

        //livox::SdkPacket packet;
        struct
        {
            //uint8_t     sof;
            uint8_t     version;
            //uint16_t    length;
            uint8_t     packet_type;
            uint16_t    seq_num;
            //uint16_t    preamble_crc;
            //uint8_t     cmd_set;
            //uint8_t     cmd_id;

        } sdk;

        std::string broadcast_code;

        //BroadcastDeviceInfo bb;
        uint8_t dev_type;                        //< \ref DeviceType.
        //uint16_t reserved;

        std::string str() const;
    };
    //-----------------------------------------------------------------------------------

    explicit VLivox_Hunter( QObject* parent = nullptr );

signals:
    void trapped( const Info& );

private:
    QUdpSocket _bcast;
    void _on_udp();
    void _read_one_udp();
};

#endif // VLIVOX_HUNTER_H
