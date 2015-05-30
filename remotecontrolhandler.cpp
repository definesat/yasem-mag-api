#include "remotecontrolhandler.h"
#include "macros.h"
#include "abstractwebpage.h"
#include "magprofile.h"

#include <QHostAddress>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtCrypto/QtCrypto>
#include <QtEndian>

using namespace yasem;

RemoteControlHandler::RemoteControlHandler(MagProfile *profile)
    : QObject(profile),
      m_profile(profile),
      m_password(""),
      m_iv_salt("erghnlhbnmbnkghy"),
      m_device(new Device(this)),
      m_multicast_socket(new QUdpSocket(this)),
      m_udp_socket(new QUdpSocket(this))
{

}

RemoteControlHandler::~RemoteControlHandler()
{
    stop();
}

void RemoteControlHandler::setParams(const QString &deviceName, const QString &password)
{
    m_device->setName(deviceName);
    m_password = password;
}

void RemoteControlHandler::start()
{
    if(m_multicast_socket->isOpen())
    {
        WARN() << "Multicast socket is already opened";
    }
    else
    {
        m_multicast_socket->bind(QHostAddress::AnyIPv4, 6000, QUdpSocket::ShareAddress);
        m_multicast_socket->joinMulticastGroup(QHostAddress::AnyIPv4);
        connect(m_multicast_socket, &QUdpSocket::readyRead, this, &RemoteControlHandler::processPendingDatagrams);
    }

    if(m_udp_socket->isOpen())
    {
        WARN() << "Remote constrol UDP socket is already opened";
    }
    else
    {
        m_udp_socket->bind(QHostAddress::Any, 9999, QUdpSocket::ShareAddress);
        connect(m_udp_socket, &QUdpSocket::readyRead, this, &RemoteControlHandler::onGetMessageFromClient);
        DEBUG() << "Remote control server started at " << m_udp_socket->localAddress() << m_udp_socket->localPort();
    }
}


void RemoteControlHandler::stop()
{
    if(m_multicast_socket->state() == QAbstractSocket::BoundState)
    {
        m_multicast_socket->close();
        DEBUG() << "Remote control multicast server stopped";
    }

    if(m_udp_socket->state() == QAbstractSocket::BoundState)
    {
        m_udp_socket->close();
        DEBUG() << "Remote control UDP socket closed";
    }

}

void RemoteControlHandler::processPendingDatagrams()
{
    while (m_multicast_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_multicast_socket->pendingDatagramSize());
        QHostAddress remote_address;
        quint16 remote_port;
        m_multicast_socket->readDatagram(datagram.data(), datagram.size(), &remote_address, &remote_port);
        DEBUG() << "DATA:" << datagram.data();
        DEBUG() << remote_address << remote_port;

        QJsonObject json = QJsonDocument::fromJson(datagram.data()).object();

        if(json.contains("protocol") && json.contains("port"))
        {
            sendDeviceInfo(remote_address, json.value("port").toInt());
        }
    }
}

void RemoteControlHandler::onGetMessageFromClient()
{
    while (m_udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        qint64 size = m_udp_socket->pendingDatagramSize();
        datagram.resize(size);
        QHostAddress remote_address;
        quint16 remote_port;
        m_udp_socket->readDatagram(datagram.data(), datagram.size(), &remote_address, &remote_port);
        parseDataAndExec(aes_256_enc_dec(datagram, QCA::Decode));
    }
}

void RemoteControlHandler::sendDeviceInfo(const QHostAddress &host, int port)
{
    QJsonObject json = m_device->toJson();
    json.insert("msgType", "info");
    m_multicast_socket->writeDatagram(QJsonDocument(json).toJson(QJsonDocument::Compact), host, port);
}

void RemoteControlHandler::parseDataAndExec(const QByteArray &data)
{
    QJsonObject obj = QJsonDocument::fromJson(data).object();
    if(obj.contains("msgType"))
        execRemoteAction(obj);
    else
        WARN() << "Incorrect data" << data;

}

void RemoteControlHandler::execRemoteAction(const QJsonObject &json)
{
    QString msgType = json.value("msgType").toString();
    if(msgType == "keyboardKey")
    {
        m_profile->page()->execKeyEvent(json.value("action").toString(), json.value("keycode").toInt(), { json.value("meta").toInt() }, json.value("unicode").toString());
    }
}

QByteArray RemoteControlHandler::aes_256_enc_dec(const QByteArray & strToEnrypt, QCA::Direction direction)
{
    QCA::SecureArray secureData = strToEnrypt;

    QByteArray res;

    if (!QCA::isSupported("aes256-cbc-pkcs7"))
    {
        WARN() << "aes128 is not supported by QCA";
        return res;
    }
    QCA::SymmetricKey key(m_password.toUtf8());
    QCA::InitializationVector iv(m_iv_salt.toUtf8());

    QCA::Cipher cipher(QString("aes256"), QCA::Cipher::CBC,
                       QCA::Cipher::DefaultPadding,
                       direction,
                       key, iv);

    QCA::SecureArray decryptedData = cipher.process(secureData);

    if (!cipher.ok())
    {
        DEBUG() << "can't update";
        return res;
    }

    QCA::SecureArray f = cipher.final();
    if (!cipher.ok())
    {
        DEBUG() << "can't final";
        return res;
    }
    res = QByteArray(decryptedData.data());

    return res;
}

Device::Device(QObject *parent):
    QObject(parent),
    m_name("YASEM"),
    m_type("MAG250"),
    m_port(9999),
    m_serial_number("555666777"),
    m_screen_width(1280),
    m_screen_height(720),
    m_device_family("unknown"),
    m_modes("TOUCHSCREEN,MOUSE,KEYBOARD"),
    m_protocol_version("")
{

}

Device::~Device()
{

}

void Device::setName(const QString &name)
{
    this->m_name = name;
}

void Device::fromJson(QJsonObject json)
{

}

QJsonObject Device::toJson()
{
    QJsonObject result;
    result.insert("name", m_name);
    result.insert("type", m_type);
    result.insert("port", m_port);
    result.insert("serialNumber", m_serial_number);
    result.insert("screenWidth", m_screen_width);
    result.insert("screenHeight", m_screen_height);
    result.insert("deviceFamily", m_device_family);
    result.insert("modes", m_modes);
    return result;
}
