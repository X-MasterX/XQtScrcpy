#ifndef XAIDEBUGFORM_H
#define XAIDEBUGFORM_H

#include <QTcpSocket>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollBar>

#include "magneticwidget.h"

class XaiDebugForm : public MagneticWidget
{
    Q_OBJECT

public:
    explicit XaiDebugForm(QWidget *adsorbWidget, AdsorbPositions adsorbPos);
    ~XaiDebugForm();

    void setSerial(const QString &serial);

private slots:
    void onConnectClicked();
    void onGetScreenshotClicked();
    void onGetLogsClicked();
    void onActivateContextClicked();
    void onScanDeeplinksClicked();

    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    void initUI();
    void setupAdbForward();
    void sendMcpRequest(const QString &method, const QJsonObject &params, int id);
    void handleMcpResponse(const QJsonObject &response);
    void logToConsole(const QString &text, const QString &color = "white");

private:
    QString m_serial;
    QTcpSocket *m_socket = nullptr;
    int m_requestId = 1;

    // UI elements
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_connectBtn = nullptr;
    
    QPushButton *m_screenshotBtn = nullptr;
    QPushButton *m_logsBtn = nullptr;
    
    QLineEdit *m_activateEdt = nullptr;
    QPushButton *m_activateBtn = nullptr;
    
    QLineEdit *m_scanEdt = nullptr;
    QPushButton *m_scanBtn = nullptr;
    
    QLabel *m_thumbnailLabel = nullptr;
    QTextEdit *m_logConsole = nullptr;
};

#endif // XAIDEBUGFORM_H
