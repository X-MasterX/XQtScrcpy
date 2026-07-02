#include "xaidebugform.h"
#include "config.h"
#include <QDebug>
#include <QBuffer>
#include <QPixmap>
#include <QCoreApplication>

XaiDebugForm::XaiDebugForm(QWidget *adsorbWidget, AdsorbPositions adsorbPos)
    : MagneticWidget(adsorbWidget, adsorbPos)
{
    initUI();
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &XaiDebugForm::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &XaiDebugForm::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &XaiDebugForm::onSocketReadyRead);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    connect(m_socket, static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
            this, &XaiDebugForm::onSocketError);
#else
    connect(m_socket, &QTcpSocket::errorOccurred, this, &XaiDebugForm::onSocketError);
#endif
}

XaiDebugForm::~XaiDebugForm()
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->close();
    }
}

void XaiDebugForm::setSerial(const QString &serial)
{
    m_serial = serial;
    logToConsole(QString("Device Serial set to: %1").arg(serial), "#00ff66");
}

void XaiDebugForm::initUI()
{
    // Bố cục tổng thể
    setWindowTitle("XAI MCP Debugger");
    setFixedWidth(320);
    
    // Thiết lập StyleSheet tối màu
    setStyleSheet(
        "QWidget { background-color: #1e1e24; color: #e0e0e0; font-family: 'Segoe UI', Arial, sans-serif; font-size: 12px; }"
        "QGroupBox { border: 1px solid #3e3e48; border-radius: 6px; margin-top: 12px; padding-top: 12px; font-weight: bold; color: #00f2fe; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 8px; padding: 0 3px; }"
        "QPushButton { background-color: #2d2d35; border: 1px solid #3e3e48; border-radius: 4px; padding: 6px 12px; min-height: 20px; font-weight: bold; color: #f3f4f6; }"
        "QPushButton:hover { background-color: #3e3e48; border-color: #00f2fe; }"
        "QPushButton:pressed { background-color: #4e4e58; }"
        "QLineEdit { background-color: #121216; border: 1px solid #3e3e48; border-radius: 4px; padding: 4px; color: #fff; }"
        "QLineEdit:focus { border-color: #00f2fe; }"
        "QTextEdit { background-color: #0b0b0d; border: 1px solid #2d2d35; border-radius: 4px; color: #a2e8dd; font-family: 'Consolas', monospace; font-size: 11px; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Group 1: Trạng thái & Kết nối
    QGroupBox *connGroup = new QGroupBox("Kết nối MCP Gateway", this);
    QVBoxLayout *connLayout = new QVBoxLayout(connGroup);
    
    QHBoxLayout *statusLayout = new QHBoxLayout();
    QLabel *statusTitle = new QLabel("Trạng thái:", this);
    m_statusLabel = new QLabel("Disconnected", this);
    m_statusLabel->setStyleSheet("color: #ff3333; font-weight: bold;");
    statusLayout->addWidget(statusTitle);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    
    m_connectBtn = new QPushButton("Connect MCP", this);
    connect(m_connectBtn, &QPushButton::clicked, this, &XaiDebugForm::onConnectClicked);
    
    connLayout->addLayout(statusLayout);
    connLayout->addWidget(m_connectBtn);
    mainLayout->addWidget(connGroup);

    // Group 2: Lệnh gỡ lỗi nhanh (MCP Tools)
    QGroupBox *toolsGroup = new QGroupBox("MCP Action Tools", this);
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsGroup);
    
    QHBoxLayout *quickBtnLayout = new QHBoxLayout();
    m_screenshotBtn = new QPushButton("Chụp màn hình", this);
    m_logsBtn = new QPushButton("Lấy Log Agent", this);
    m_screenshotBtn->setEnabled(false);
    m_logsBtn->setEnabled(false);
    
    connect(m_screenshotBtn, &QPushButton::clicked, this, &XaiDebugForm::onGetScreenshotClicked);
    connect(m_logsBtn, &QPushButton::clicked, this, &XaiDebugForm::onGetLogsClicked);
    quickBtnLayout->addWidget(m_screenshotBtn);
    quickBtnLayout->addWidget(m_logsBtn);
    toolsLayout->addLayout(quickBtnLayout);
    
    // Form cho các tham số khác
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setVerticalSpacing(6);
    formLayout->setHorizontalSpacing(6);
    
    m_activateEdt = new QLineEdit(this);
    m_activateEdt->setPlaceholderText("e.g. shopee");
    m_activateBtn = new QPushButton("Kích hoạt", this);
    m_activateBtn->setEnabled(false);
    connect(m_activateBtn, &QPushButton::clicked, this, &XaiDebugForm::onActivateContextClicked);
    
    QHBoxLayout *actLayout = new QHBoxLayout();
    actLayout->addWidget(m_activateEdt);
    actLayout->addWidget(m_activateBtn);
    formLayout->addRow("Ngữ cảnh:", actLayout);
    
    m_scanEdt = new QLineEdit(this);
    m_scanEdt->setPlaceholderText("e.g. com.shopee.vn");
    m_scanBtn = new QPushButton("Dò quét", this);
    m_scanBtn->setEnabled(false);
    connect(m_scanBtn, &QPushButton::clicked, this, &XaiDebugForm::onScanDeeplinksClicked);
    
    QHBoxLayout *scanLayout = new QHBoxLayout();
    scanLayout->addWidget(m_scanEdt);
    scanLayout->addWidget(m_scanBtn);
    formLayout->addRow("DeepLink:", scanLayout);
    
    toolsLayout->addLayout(formLayout);
    mainLayout->addWidget(toolsGroup);

    // Group 3: Console Logs
    QGroupBox *consoleGroup = new QGroupBox("XAI Execution Logs", this);
    QVBoxLayout *consoleLayout = new QVBoxLayout(consoleGroup);
    m_logConsole = new QTextEdit(this);
    m_logConsole->setReadOnly(true);
    m_logConsole->setMinimumHeight(150);
    consoleLayout->addWidget(m_logConsole);
    mainLayout->addWidget(consoleGroup);

    // Group 4: Thumbnail Ảnh chụp màn hình MCP
    QGroupBox *thumbGroup = new QGroupBox("MCP Screen View (Base64)", this);
    QVBoxLayout *thumbLayout = new QVBoxLayout(thumbGroup);
    m_thumbnailLabel = new QLabel(this);
    m_thumbnailLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_thumbnailLabel->setAlignment(Qt::AlignCenter);
    m_thumbnailLabel->setMinimumHeight(160);
    m_thumbnailLabel->setStyleSheet("background-color: #0b0b0d; color: #55555d;");
    m_thumbnailLabel->setText("No Image Captured");
    thumbLayout->addWidget(m_thumbnailLabel);
    mainLayout->addWidget(thumbGroup);
    
    mainLayout->addStretch();
}

void XaiDebugForm::setupAdbForward()
{
    if (m_serial.isEmpty()) return;
    
    QProcess process;
    QString adbPath = Config::getInstance().getAdbPath();
    QStringList args;
    args << "-s" << m_serial << "forward" << "tcp:18789" << "localabstract:xai_mcp";
    
    logToConsole(QString("Running: adb -s %1 forward tcp:18789 localabstract:xai_mcp").arg(m_serial), "#888888");
    process.start(adbPath, args);
    if (process.waitForFinished(3000)) {
        logToConsole("ADB port forward configured successfully.", "#00ff66");
    } else {
        logToConsole("ADB port forward command timed out or failed.", "#ff3333");
    }
}

void XaiDebugForm::onConnectClicked()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    } else {
        setupAdbForward();
        logToConsole("Connecting to localhost:18789...", "#00f2fe");
        m_socket->connectToHost("127.0.0.1", 18789);
    }
}

void XaiDebugForm::onSocketConnected()
{
    m_statusLabel->setText("Connected");
    m_statusLabel->setStyleSheet("color: #00ff66; font-weight: bold;");
    m_connectBtn->setText("Disconnect MCP");
    
    m_screenshotBtn->setEnabled(true);
    m_logsBtn->setEnabled(true);
    m_activateBtn->setEnabled(true);
    m_scanBtn->setEnabled(true);
    
    logToConsole("Connected to XAI MCP Server successfully!", "#00ff66");
    
    // Gửi bản tin initialize bắt tay theo chuẩn MCP
    QJsonObject params;
    params["protocolVersion"] = "2024-11-05";
    
    QJsonObject clientInfo;
    clientInfo["name"] = "QtScrcpy-MCP-Debugger";
    clientInfo["version"] = "1.0.0";
    params["clientInfo"] = clientInfo;
    
    sendMcpRequest("initialize", params, m_requestId++);
}

void XaiDebugForm::onSocketDisconnected()
{
    m_statusLabel->setText("Disconnected");
    m_statusLabel->setStyleSheet("color: #ff3333; font-weight: bold;");
    m_connectBtn->setText("Connect MCP");
    
    m_screenshotBtn->setEnabled(false);
    m_logsBtn->setEnabled(false);
    m_activateBtn->setEnabled(false);
    m_scanBtn->setEnabled(false);
    
    logToConsole("Disconnected from XAI MCP Server.", "#ff3333");
}

void XaiDebugForm::onSocketReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) continue;
        
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }
        
        QJsonObject response = doc.object();
        handleMcpResponse(response);
    }
}

void XaiDebugForm::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    logToConsole(QString("Socket Error: %1").arg(m_socket->errorString()), "#ff3333");
}

void XaiDebugForm::sendMcpRequest(const QString &method, const QJsonObject &params, int id)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = id;
    request["method"] = method;
    if (!params.isEmpty()) {
        request["params"] = params;
    }
    
    QJsonDocument doc(request);
    m_socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void XaiDebugForm::onGetScreenshotClicked()
{
    QJsonObject params;
    params["name"] = "get_screenshot";
    params["arguments"] = QJsonObject();
    
    logToConsole("Calling MCP tool: get_screenshot...", "#00f2fe");
    sendMcpRequest("tools/call", params, 100);
}

void XaiDebugForm::onGetLogsClicked()
{
    QJsonObject params;
    params["name"] = "get_logs";
    params["arguments"] = QJsonObject();
    
    logToConsole("Calling MCP tool: get_logs...", "#00f2fe");
    sendMcpRequest("tools/call", params, 101);
}

void XaiDebugForm::onActivateContextClicked()
{
    QString target = m_activateEdt->text().trim();
    if (target.isEmpty()) return;
    
    QJsonObject args;
    args["target"] = target;
    
    QJsonObject params;
    params["name"] = "activate_app_context";
    params["arguments"] = args;
    
    logToConsole(QString("Calling MCP tool: activate_app_context(target='%1')...").arg(target), "#00f2fe");
    sendMcpRequest("tools/call", params, 102);
}

void XaiDebugForm::onScanDeeplinksClicked()
{
    QString pkg = m_scanEdt->text().trim();
    if (pkg.isEmpty()) return;
    
    QJsonObject args;
    args["package_name"] = pkg;
    
    QJsonObject params;
    params["name"] = "scan_app_deeplinks";
    params["arguments"] = args;
    
    logToConsole(QString("Calling MCP tool: scan_app_deeplinks(package_name='%1')...").arg(pkg), "#00f2fe");
    sendMcpRequest("tools/call", params, 103);
}

void XaiDebugForm::handleMcpResponse(const QJsonObject &response)
{
    int id = response.value("id").toInt();
    
    if (response.contains("error")) {
        QJsonObject err = response.value("error").toObject();
        logToConsole(QString("[MCP Error] Code %1: %2").arg(err.value("code").toInt()).arg(err.value("message").toString()), "#ff3333");
        return;
    }
    
    QJsonObject result = response.value("result").toObject();
    
    if (id == 100) { // get_screenshot
        QJsonArray contentArray = result.value("content").toArray();
        if (!contentArray.isEmpty()) {
            QJsonObject contentObj = contentArray.at(0).toObject();
            QString base64Image = contentObj.value("text").toString();
            
            QByteArray imgBytes = QByteArray::fromBase64(base64Image.toUtf8());
            QPixmap pixmap;
            if (pixmap.loadFromData(imgBytes)) {
                m_thumbnailLabel->setPixmap(pixmap.scaled(m_thumbnailLabel->width() - 4, m_thumbnailLabel->height() - 4, 
                                                          Qt::KeepAspectRatio, Qt::SmoothTransformation));
                logToConsole("Screenshot updated successfully via MCP.", "#00ff66");
            } else {
                logToConsole("Failed to decode screenshot base64 image data.", "#ff3333");
            }
        }
    } else if (id == 101) { // get_logs
        QJsonArray contentArray = result.value("content").toArray();
        if (!contentArray.isEmpty()) {
            QJsonObject contentObj = contentArray.at(0).toObject();
            QString logsJsonStr = contentObj.value("text").toString();
            
            QJsonDocument logsDoc = QJsonDocument::fromJson(logsJsonStr.toUtf8());
            if (logsDoc.isArray()) {
                m_logConsole->clear();
                QJsonArray logsArray = logsDoc.array();
                for (int i = 0; i < logsArray.size(); ++i) {
                    m_logConsole->append(logsArray.at(i).toString());
                }
                logToConsole("Agent Logs refreshed.", "#00ff66");
            } else {
                logToConsole("No active logs received or parsing failed.", "#ff8800");
            }
        }
    } else if (id == 102) { // activate_app_context
        logToConsole("App context activated successfully.", "#00ff66");
    } else if (id == 103) { // scan_app_deeplinks
        logToConsole("Deep links scan triggered.", "#00ff66");
    } else {
        // initialize or other tools response
        if (response.contains("result")) {
            logToConsole(QString("MCP Response [%1]: Success").arg(id), "#888888");
        }
    }
}

void XaiDebugForm::logToConsole(const QString &text, const QString &color)
{
    m_logConsole->append(QString("<font color=\"%1\">%2</font>").arg(color).arg(text));
    m_logConsole->verticalScrollBar()->setValue(m_logConsole->verticalScrollBar()->maximum());
}
