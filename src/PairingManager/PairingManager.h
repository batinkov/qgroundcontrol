/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>
#include <QTime>
#include <QVariantMap>

#include "openssl_aes.h"
#include "openssl_rsa.h"
#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "Fact.h"
#include "UDPLink.h"
#if defined QGC_ENABLE_NFC
#include "PairingNFC.h"
#endif
#if defined QGC_ENABLE_QTNFC
#include "QtNFC.h"
#endif
#ifdef __android__
#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#endif

Q_DECLARE_LOGGING_CATEGORY(PairingManagerLog)

class AppSettings;
class QGCApplication;

//-----------------------------------------------------------------------------
class PairingManager : public QGCTool
{
    Q_OBJECT
public:
    explicit PairingManager (QGCApplication* app, QGCToolbox* toolbox);
    ~PairingManager         () override;

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox) override;

    enum PairingStatus {
        PairingIdle,
        PairingActive,
        PairingSuccess,
        PairingConnecting,
        PairingConnected,
        PairingRejected,
        PairingConnectionRejected,
        PairingError
    };

    Q_ENUM(PairingStatus)

    QStringList     pairingLinkTypeStrings      ();
    QString         pairingStatusStr            () const;
    QStringList     connectedDeviceNameList     ();
    QStringList     pairedDeviceNameList        ();
    PairingStatus   pairingStatus               () { return _status; }
    QString         connectedVehicle            () { return _lastConnected; }
    int             nfcIndex                    () { return _nfcIndex; }
    int             microhardIndex              () { return _microhardIndex; }
    bool            firstBoot                   () { return _firstBoot; }
    bool            usePairing                  () { return _usePairing; }
    bool            videoCanRestart             () { return !_usePairing || !_connectedDevices.empty(); }
    bool            errorState                  () { return _status == PairingRejected || _status == PairingConnectionRejected || _status == PairingError; }
    void            setStatusMessage            (PairingStatus status, const QString& statusStr) { emit setPairingStatus(status, statusStr); }
    void            setFirstBoot                (bool set) { _firstBoot = set; emit firstBootChanged(); }
    void            setUsePairing               (bool set);
    void            jsonReceivedStartPairing    (const QString& jsonEnc);
#ifdef __android__
    static void     setNativeMethods            (void);
#endif
    Q_INVOKABLE void    connectToDevice         (const QString& name);
    Q_INVOKABLE void    removePairedDevice      (const QString& name);
    Q_INVOKABLE void    setConnectingChannel    (int channel);
    Q_INVOKABLE QString extractName             (const QString& name);
    Q_INVOKABLE QString extractChannel          (const QString& name);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    Q_INVOKABLE void    startNFCScan            ();
#endif    
#if QGC_GST_MICROHARD_ENABLED
    Q_INVOKABLE void    startMicrohardPairing   (const QString& pairingKey);
#endif
    Q_INVOKABLE void    stopPairing             ();
    Q_INVOKABLE void    disconnectDevice        (const QString& name);

    Q_PROPERTY(QString          pairingStatusStr        READ pairingStatusStr        NOTIFY pairingStatusChanged)
    Q_PROPERTY(PairingStatus    pairingStatus           READ pairingStatus           NOTIFY pairingStatusChanged)
    Q_PROPERTY(QStringList      connectedDeviceNameList READ connectedDeviceNameList NOTIFY deviceListChanged)
    Q_PROPERTY(QStringList      pairedDeviceNameList    READ pairedDeviceNameList    NOTIFY deviceListChanged)
    Q_PROPERTY(QStringList      pairingLinkTypeStrings  READ pairingLinkTypeStrings  CONSTANT)
    Q_PROPERTY(QString          connectedVehicle        READ connectedVehicle        NOTIFY connectedVehicleChanged)
    Q_PROPERTY(bool             errorState              READ errorState              NOTIFY pairingStatusChanged)
    Q_PROPERTY(int              nfcIndex                READ nfcIndex                CONSTANT)
    Q_PROPERTY(int              microhardIndex          READ microhardIndex          CONSTANT)
    Q_PROPERTY(bool             firstBoot               READ firstBoot               WRITE setFirstBoot  NOTIFY firstBootChanged)
    Q_PROPERTY(bool             usePairing              READ usePairing              WRITE setUsePairing NOTIFY usePairingChanged)

signals:
    void startUpload                            (const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt, int retries);
    void stopUpload                             ();
    void startCommand                           (const QString& name, const QString& url, const QString& content);
    void closeConnection                        ();
    void pairingConfigurationsChanged           ();
    void nameListChanged                        ();
    void pairingStatusChanged                   ();
    void setPairingStatus                       (PairingStatus status, const QString& pairingStatus);
    void deviceListChanged                      ();
    void connectedVehicleChanged                ();
    void firstBootChanged                       ();
    void usePairingChanged                      ();
    void connectToPairedDevice                  (const QString& name);

private slots:
    void _startCommand                          (const QString& name, const QString& pairURL, const QString& content);
    void _startUpload                           (const QString& name, const QString& pairURL, const QJsonDocument& jsonDoc, bool signAndEncrypt, int retries);
    void _startUploadRequest                    (const QString& name, const QString& url, const QString& data, int retries);
    void _parsePairingJsonAndConnect            (const QString& jsonEnc);
    void _setPairingStatus                      (PairingStatus status, const QString& pairingStatus);
    void _connectToPairedDevice                 (const QString& name);
    void _setEnabled                            ();

private:
    int                           _nfcIndex = -1;
    int                           _microhardIndex = -1;
    PairingStatus                 _status = PairingIdle;
    QString                       _statusString;
    QString                       _lastConnected;
    QString                       _encryptionKey;
    QString                       _publicKey;
    OpenSSL_AES                   _aes;
    OpenSSL_RSA                   _rsa;
    OpenSSL_RSA                   _device_rsa;
    QJsonDocument                 _gcsJsonDoc{};
    QMap<QString, QJsonDocument>  _devices{};
    QNetworkAccessManager         _uploadManager;
    bool                          _firstBoot = true;
    bool                          _usePairing = false;
    QMap<QString, qint64>         _devicesToConnect{};
    QTimer                        _reconnectTimer;
    QMap<QString, LinkInterface*> _connectedDevices;

    QJsonDocument           _createZeroTierConnectJson  (const QVariantMap& remotePairingMap);
    QJsonDocument           _createMicrohardConnectJson (const QVariantMap& remotePairingMap);
    QJsonDocument           _createZeroTierPairingJson  (const QVariantMap& remotePairingMap);
    QJsonDocument           _createMicrohardPairingJson (const QVariantMap& remotePairingMap);
    void                    _writeJson                  (const QJsonDocument &jsonDoc, const QString& fileName);
    QString                 _getLocalIPInNetwork        (const QString& remoteIP, int num);
    void                    _commandFinished            ();
    void                    _uploadFinished             ();
    void                    _uploadError                (QNetworkReply::NetworkError code);
    void                    _pairingCompleted           (const QString& tempName, const QString& newName, const QString& devicePublicKey);
    bool                    _connectionCompleted        (const QString& response);
    QDir                    _pairingCacheDir            ();
    QDir                    _pairingCacheTempDir        ();
    QString                 _pairingCacheFile           (const QString& uavName);
    void                    _updatePairedDeviceNameList ();
    QString                 _random_string              (uint length);
    void                    _readPairingConfig          ();
    void                    _resetPairingConfig         ();
    void                    _updateConnectedDevices     ();
    void                    _createUDPLink              (const QString& name, quint16 port);
    void                    _removeUDPLink              (const QString& name);
    void                    _linkActiveChanged          (LinkInterface* link, bool active, int vehicleID);
    void                    _autoConnect                ();
    QJsonDocument           _getPairingJsonDoc          (const QString& name, bool remove = false);
    QVariantMap             _getPairingMap              (const QString& name);
    void                    _setConnectingChannel       (const QString& name, int channel);
    QString                 _removeRSAkey               (const QString& s);
    int                     _getDeviceChannel           (const QString& name);
    QDateTime               _getDeviceConnectTime       (const QString& name);
    QString                 _getDeviceIP                (const QString& name);

#if defined QGC_ENABLE_NFC || defined QGC_ENABLE_QTNFC
    PairingNFC              pairingNFC;
#endif
};
