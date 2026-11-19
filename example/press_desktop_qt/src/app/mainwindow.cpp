#include "app/mainwindow.h"
#include "core/format.h"
#include "core/machine_state.h"
#include "core/press_service.h"
#include "dialogs/overlaydialogs.h"
#include "pages/cooppage.h"
#include "pages/settingspage.h"
#include "pages/singlepage.h"
#include "theme/theme.h"
#include "widgets/actionbar.h"
#include "widgets/logpanel.h"
#include "widgets/sidebar.h"
#include "widgets/titlebar.h"
#include "widgets/toast.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("自动热压机 · 上位机"));
    setWindowFlag(Qt::FramelessWindowHint);
    resize(1440, 900);
    setMinimumSize(1180, 700);

    // --sdk <地址>：启动时自动连接设备（例如 192.168.1.20:9000 或 COM3）
    QString sdkEndpoint;
    const QStringList args = QCoreApplication::arguments();
    const int sdkIndex = args.indexOf(QStringLiteral("--sdk"));
    if (sdkIndex >= 0 && sdkIndex + 1 < args.size())
        sdkEndpoint = args.at(sdkIndex + 1).trimmed();
    if (!sdkEndpoint.isEmpty()) {
        QSettings(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"))
            .setValue(QStringLiteral("link/endpoint"), sdkEndpoint);
    }

    m_state = new MachineState(this);
    m_service = new PressService(this);
    m_state->attachService(m_service);

    m_root = new QWidget(this);
    m_root->setObjectName(QStringLiteral("root"));
    m_root->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(m_root);

    auto *layout = new QVBoxLayout(m_root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_titleBar = new TitleBar(m_root);
    layout->addWidget(m_titleBar);

    auto *body = new QWidget(m_root);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_sidebar = new Sidebar(body);
    bodyLayout->addWidget(m_sidebar);
    bodyLayout->addWidget(createContent(), 1);
    layout->addWidget(body, 1);

    // 日志面板位于底栏上方，默认隐藏
    m_logPanel = new LogPanel(m_root);
    m_logPanel->hide();
    layout->addWidget(m_logPanel);

    m_actionBar = new ActionBar(m_state, m_root);
    layout->addWidget(m_actionBar);

    m_toast = new Toast(m_root);
    m_toast->raise();

    m_logPanel->append(QStringLiteral("上位机已启动，等待连接设备"));

    // ---- 页面信号 ----
    connect(m_sidebar, &Sidebar::navRequested, this, &MainWindow::navigate);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRequested, this, [this] {
        if (isMaximized())
            showNormal();
        else
            showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);

    connect(m_actionBar, &ActionBar::pressRequested, this, &MainWindow::handlePressRequest);
    connect(m_actionBar, &ActionBar::heatRequested, this, &MainWindow::handleHeatRequest);
    connect(m_actionBar, &ActionBar::ventRequested, this, &MainWindow::handleVentRequest);
    connect(m_actionBar, &ActionBar::coopRequested, this, &MainWindow::handleCoopRequest);
    connect(m_actionBar, &ActionBar::logToggled, this, &MainWindow::toggleLogPanel);
    connect(m_logPanel, &LogPanel::collapseRequested, this, [this] {
        m_logPanel->hide();
        positionToast();
    });
    connect(m_logPanel, &LogPanel::unseenChanged, this, [this](int count) {
        m_actionBar->setLogBadge(m_logPanel->isVisible() ? 0 : count);
    });

    // SDK 通讯状态 / 错误提示
    connect(m_state, &MachineState::sdkError, this, &MainWindow::showToast);
    connect(m_service, &PressService::logMessage, this, [this](const QString &text) {
        if (m_logPanel)
            m_logPanel->append(text);
    });
    connect(m_state, &MachineState::linkChanged, this,
            [this](bool connected, const QString &endpoint) {
                m_sidebar->setLinkStatus(connected, connected ? endpoint : QString());
                if (connected) {
                    // 连接成功后清空历史曲线，从当前时刻重新采集
                    m_singlePage->resetChart();
                    m_coopPage->resetChart();
                }
                showToast(connected ? QStringLiteral("设备已连接：%1").arg(endpoint)
                                    : QStringLiteral("设备未连接，当前为本地模拟"));
            });

    // 设备上报的工作模式驱动页面切换：1 合作模式，2 单独模式
    connect(m_state, &MachineState::modelStateChanged, this, [this](int model) {
        const QString key = model == 1 ? QStringLiteral("coop")
                          : model == 2 ? QStringLiteral("single")
                                       : QString();
        const bool allowed = (key == QStringLiteral("coop") && m_state->coopAllowed)
                          || (key == QStringLiteral("single") && m_state->singleAllowed);
        if (key.isEmpty() || !allowed || key == m_current || m_state->running())
            return;
        showPage(key);
        if (auto *scroll = qobject_cast<QScrollArea *>(m_pageWidgets.value(key)))
            scroll->verticalScrollBar()->setValue(0);
        showToast(model == 1 ? QStringLiteral("设备切换为合作模式")
                             : QStringLiteral("设备切换为单独模式"));
    });

    connect(m_state, &MachineState::ticked, m_singlePage, &SinglePage::onTick);
    connect(m_state, &MachineState::ticked, m_coopPage, &CoopPage::onTick);
    connect(m_state, &MachineState::changed, this, &MainWindow::updateActionBar);
    connect(m_state, &MachineState::changed, this, [this] {
        m_sidebar->setLocked(m_current, m_state->running());
    });
    connect(m_state, &MachineState::settingsChanged, this,
            &MainWindow::syncModeAvailability);

    m_sidebar->setActive(m_current);
    m_actionBar->setMode(ActionBar::Mode::Run);
    m_stack->setCurrentWidget(m_pageWidgets.value(m_current));
    updateActionBar();

    if (!sdkEndpoint.isEmpty()) {
        QTimer::singleShot(150, this, [this, sdkEndpoint] {
            m_service->connectDevice(sdkEndpoint,
                                     sdkEndpoint.contains(QLatin1Char(':')));
        });
    }
}

QWidget *MainWindow::createContent()
{
    auto *content = new QWidget(this);
    content->setObjectName(QStringLiteral("contentArea"));
    content->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(content);
    m_stack->setObjectName(QStringLiteral("stack"));

    m_singlePage = new SinglePage(m_state, m_stack);
    m_coopPage = new CoopPage(m_state, m_stack);
    m_settingsPage = new SettingsPage(m_state, m_service, m_stack);

    m_pageWidgets.insert(QStringLiteral("single"), wrapPage(m_singlePage));
    m_pageWidgets.insert(QStringLiteral("coop"), wrapPage(m_coopPage));
    m_pageWidgets.insert(QStringLiteral("settings"), wrapPage(m_settingsPage));

    for (auto *page : m_pageWidgets)
        m_stack->addWidget(page);

    layout->addWidget(m_stack, 1);
    return content;
}

QScrollArea *MainWindow::wrapPage(QWidget *page)
{
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("pageScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(page);
    scroll->viewport()->setAutoFillBackground(false);
    return scroll;
}

void MainWindow::showPage(const QString &key)
{
    if (!m_pageWidgets.contains(key))
        return;
    m_current = key;
    m_sidebar->setActive(key);
    m_stack->setCurrentWidget(m_pageWidgets.value(key));
    m_actionBar->setMode(key == QStringLiteral("coop") ? ActionBar::Mode::Coop
                         : key == QStringLiteral("settings") ? ActionBar::Mode::Settings
                                                              : ActionBar::Mode::Run);
    m_sidebar->setLocked(key, m_state->running());

    if (m_logPanel) {
        const QString label = key == QStringLiteral("coop") ? QStringLiteral("合作模式")
                            : key == QStringLiteral("settings") ? QStringLiteral("系统设置")
                                                                : QStringLiteral("单独模式");
        m_logPanel->append(QStringLiteral("切换到%1页面").arg(label));
    }
}

void MainWindow::navigate(const QString &key)
{
    // “连接”不是页面，点击后弹出连接对话框
    if (key == QStringLiteral("connect")) {
        if (m_state->running()) {
            showToast(QStringLiteral("设备运行中，请先停止后再连接"));
            return;
        }
        Dialogs::connectDevice(this, m_service);
        return;
    }

    if (!m_pageWidgets.contains(key) || key == m_current)
        return;
    if (key == QStringLiteral("single") && !m_state->singleAllowed) {
        showToast(QStringLiteral("当前设备版本仅支持合作模式"));
        return;
    }
    if (key == QStringLiteral("coop") && !m_state->coopAllowed) {
        showToast(QStringLiteral("当前设备版本仅支持单独模式"));
        return;
    }
    if (m_state->running()) {
        showToast(QStringLiteral("设备运行中，请先停止后再切换"));
        return;
    }
    // 模式切换需要下发到下位机（1 合作模式，2 单独模式）
    if (key == QStringLiteral("single") || key == QStringLiteral("coop")) {
        if (m_service->isConnected()
            && !m_service->selectModel(key == QStringLiteral("coop") ? 1 : 2)) {
            showToast(QStringLiteral("模式切换失败，请检查设备状态"));
            return;
        }
    }
    showPage(key);
    if (auto *scroll = qobject_cast<QScrollArea *>(m_pageWidgets.value(key)))
        scroll->verticalScrollBar()->setValue(0);
}

void MainWindow::syncModeAvailability()
{
    m_sidebar->setModeVisible(QStringLiteral("single"), m_state->singleAllowed);
    m_sidebar->setModeVisible(QStringLiteral("coop"), m_state->coopAllowed);

    if (m_current == QStringLiteral("single") && !m_state->singleAllowed
        && m_state->coopAllowed)
        showPage(QStringLiteral("coop"));
    else if (m_current == QStringLiteral("coop") && !m_state->coopAllowed
             && m_state->singleAllowed)
        showPage(QStringLiteral("single"));
}

void MainWindow::updateActionBar()
{
    m_actionBar->refresh();
    m_sidebar->setLocked(m_current, m_state->running());
}

void MainWindow::handlePressRequest()
{
    if (m_state->press) {
        m_state->stopPress();
        showToast(QStringLiteral("加压已停止"));
        return;
    }

    const PressureStage &first = m_state->pStages.at(0);
    const QString text = QStringLiteral(
        "将按压力流程执行 %1 段加压，首段 %2 Ton / 保压 %3。"
        "请确认人员已离开压机工作区。")
        .arg(m_state->pCount)
        .arg(fmtNumber(first.p))
        .arg(fmtTime(first.hold));

    if (Dialogs::confirm(this, QStringLiteral("启动加压"), text,
                         QStringLiteral("确认启动"), DialogTone::Info, AppIcon::Power)) {
        m_state->startPress();
        showToast(QStringLiteral("加压已启动"));
    }
}

void MainWindow::handleHeatRequest()
{
    if (m_state->heat) {
        m_state->stopHeat();
        showToast(QStringLiteral("加热已停止"));
        return;
    }

    const TemperatureStage &last = m_state->tStages.at(m_state->tCount - 1);
    const QString text = QStringLiteral(
        "将按温度流程执行 %1 段升温，末段目标 %2 °C，升温速率 %3 °C/min。")
        .arg(m_state->tCount)
        .arg(fmtNumber(last.t))
        .arg(fmtNumber(m_state->tStages.at(0).rate));

    if (Dialogs::confirm(this, QStringLiteral("启动加热"), text,
                         QStringLiteral("确认启动"), DialogTone::Warn, AppIcon::Flame)) {
        m_state->startHeat();
        showToast(QStringLiteral("加热已启动"));
    }
}

void MainWindow::handleVentRequest()
{
    const QString text = QStringLiteral("将打开排气阀释放管路压力，请确认模具与人员安全。");
    if (Dialogs::confirm(this, QStringLiteral("排气"), text,
                         QStringLiteral("确认排气"), DialogTone::Warn, AppIcon::Air)) {
        m_state->vent();
        showToast(QStringLiteral("排气完成，压力已释放"));
    }
}

void MainWindow::handleCoopRequest()
{
    m_state->toggleCoop();
    showToast(m_state->coopRunning ? QStringLiteral("合作模式已启动")
                                   : QStringLiteral("合作模式已停止"));
}

void MainWindow::showToast(const QString &text)
{
    if (m_logPanel)
        m_logPanel->append(text);
    m_toast->showMessage(text);
    positionToast();
    m_toast->raise();
}

void MainWindow::toggleLogPanel()
{
    if (!m_logPanel)
        return;
    const bool show = !m_logPanel->isVisible();
    m_logPanel->setVisible(show);
    if (show)
        m_logPanel->clearUnseen();
    else
        m_actionBar->setLogBadge(0);
    positionToast();
    if (show)
        m_logPanel->raise();
}

void MainWindow::positionToast()
{
    if (!m_toast || !m_root)
        return;
    m_toast->adjustSize();
    const int x = (m_root->width() - m_toast->width()) / 2;
    const int y = m_root->height() - 88 - m_toast->height();
    m_toast->move(x, y);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionToast();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    positionToast();

    // 首次显示时把页面滚动条归零，避免焦点自动定位导致顶部卡片标题被滚出视野
    if (!m_didInitialScroll) {
        m_didInitialScroll = true;
        QTimer::singleShot(0, this, [this] {
            if (auto *scroll = qobject_cast<QScrollArea *>(m_pageWidgets.value(m_current)))
                scroll->verticalScrollBar()->setValue(0);
        });
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#ifdef Q_OS_WIN
    // 无边框窗口默认没有系统缩放边框，这里恢复八方向拖拽缩放，
    // 宽高可分别调整，不再被角落手柄绑定成同一比例。
    if (eventType == QByteArrayLiteral("windows_generic_MSG") && !isMaximized()
        && !isFullScreen()) {
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_NCHITTEST) {
            constexpr int kBorder = 6;
            const QPoint pos = mapFromGlobal(
                QPoint(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)));
            const bool left = pos.x() >= 0 && pos.x() < kBorder;
            const bool right = pos.x() <= width() && pos.x() >= width() - kBorder;
            const bool top = pos.y() >= 0 && pos.y() < kBorder;
            const bool bottom = pos.y() <= height() && pos.y() >= height() - kBorder;

            if (top && left)
                *result = HTTOPLEFT;
            else if (top && right)
                *result = HTTOPRIGHT;
            else if (bottom && left)
                *result = HTBOTTOMLEFT;
            else if (bottom && right)
                *result = HTBOTTOMRIGHT;
            else if (left)
                *result = HTLEFT;
            else if (right)
                *result = HTRIGHT;
            else if (top)
                *result = HTTOP;
            else if (bottom)
                *result = HTBOTTOM;
            else
                return QMainWindow::nativeEvent(eventType, message, result);
            return true;
        }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}
