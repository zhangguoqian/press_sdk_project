//
// Created by 11518 on 2024/7/30.
//

#include "uihome.h"

#include <QAbstractSpinBox>
#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

#include "admin/controldialog.h"
#include "chartview/presschartview.h"
#include "steplist/steptablewidget.h"
#include "zgq.h"

namespace {
// 主界面分页索引 / stacked page indexes
enum PageIndex
{
    PageSeparate = 0,
    PageCooperate = 1,
    PageChart = 2,
    PageProcess = 3,
    PageSystem = 4,
    PageData = 5,
};

QTableWidgetItem* cellItem(QTableWidget* table, int row, int column, const QString& text,
                           const QColor& color = QColor("#333333"), bool bold = false)
{
    QTableWidgetItem* item = table->item(row, column);
    if (item == nullptr)
    {
        item = new QTableWidgetItem;
        table->setItem(row, column, item);
    }
    item->setText(text);
    item->setTextAlignment(Qt::AlignCenter);
    item->setForeground(QBrush(color));
    QFont font = item->font();
    font.setBold(bold);
    item->setFont(font);
    return item;
}

QLabel* heatCellWidget(const QString& upText, bool upOn, const QString& downText, bool downOn)
{
    const QString onColor = QStringLiteral("#1a7fd4");
    const QString offColor = QStringLiteral("#d63b3b");
    auto* label = new QLabel(QStringLiteral("<span style='color:%1'>%2</span><br/>"
                                            "<span style='color:%3'>%4</span>")
                                 .arg(upOn ? onColor : offColor, upText,
                                      downOn ? onColor : offColor, downText));
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("background:transparent;font-size:15px;font-weight:bold;"));
    return label;
}

//! 按按钮名字绘制白色线性图标（底部导航）
//! paint a simple white line icon per navigation button
QPixmap navIconPixmap(const QString& name)
{
    QPixmap pixmap(40, 40);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor("#ffffff"));
    pen.setWidthF(2.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    if (name == QStringLiteral("btnStartPress"))
    {
        // 电源 / power
        painter.drawArc(QRectF(9, 9, 22, 22), 50 * 16, 260 * 16);
        painter.drawLine(QPointF(20, 5), QPointF(20, 17));
    }
    else if (name == QStringLiteral("btnMode"))
    {
        // 压机 / press
        painter.drawLine(QPointF(6, 8), QPointF(34, 8));
        painter.drawLine(QPointF(20, 12), QPointF(20, 25));
        painter.drawLine(QPointF(14, 19), QPointF(20, 25));
        painter.drawLine(QPointF(26, 19), QPointF(20, 25));
        painter.drawLine(QPointF(6, 32), QPointF(34, 32));
    }
    else if (name == QStringLiteral("btnData"))
    {
        // 数据库 / database
        painter.drawEllipse(QRectF(9, 7, 22, 9));
        painter.drawLine(QPointF(9, 11.5), QPointF(9, 27));
        painter.drawLine(QPointF(31, 11.5), QPointF(31, 27));
        painter.drawArc(QRectF(9, 22, 22, 10), 180 * 16, 180 * 16);
        painter.drawArc(QRectF(9, 15, 22, 10), 180 * 16, 180 * 16);
    }
    else if (name == QStringLiteral("btnChart"))
    {
        // 柱状图 / bar chart
        painter.drawLine(QPointF(7, 34), QPointF(34, 34));
        painter.drawLine(QPointF(7, 34), QPointF(7, 6));
        painter.drawLine(QPointF(12, 29), QPointF(12, 21));
        painter.drawLine(QPointF(20, 29), QPointF(20, 13));
        painter.drawLine(QPointF(28, 29), QPointF(28, 17));
    }
    else if (name == QStringLiteral("btnProcess"))
    {
        // 流程 / flow
        painter.drawEllipse(QRectF(15, 15, 10, 10));
        painter.drawEllipse(QRectF(16, 4, 8, 8));
        painter.drawEllipse(QRectF(4, 26, 8, 8));
        painter.drawEllipse(QRectF(28, 26, 8, 8));
        painter.drawLine(QPointF(20, 15), QPointF(20, 12));
        painter.drawLine(QPointF(15, 22), QPointF(11, 26));
        painter.drawLine(QPointF(25, 22), QPointF(29, 26));
    }
    else if (name == QStringLiteral("btnSystem"))
    {
        // 齿轮 / gear
        painter.drawEllipse(QRectF(13, 13, 14, 14));
        painter.drawEllipse(QRectF(17, 17, 6, 6));
        for (int i = 0; i < 8; ++i)
        {
            const qreal angle = qDegreesToRadians(static_cast<qreal>(i * 45));
            painter.drawLine(QPointF(20 + 7.5 * qCos(angle), 20 + 7.5 * qSin(angle)),
                             QPointF(20 + 12.0 * qCos(angle), 20 + 12.0 * qSin(angle)));
        }
    }
    else if (name == QStringLiteral("btnStartHeat"))
    {
        // 火焰 / flame
        QPainterPath flame;
        flame.moveTo(20, 5);
        flame.cubicTo(29, 14, 31, 20, 26, 27);
        flame.cubicTo(23, 32, 17, 32, 14, 27);
        flame.cubicTo(9, 20, 13, 14, 20, 5);
        painter.drawPath(flame);
    }
    else if (name == QStringLiteral("btnStartCooperate"))
    {
        // 启动 / play
        QPainterPath play;
        play.moveTo(14, 7);
        play.lineTo(31, 20);
        play.lineTo(14, 33);
        play.closeSubpath();
        painter.setBrush(QColor("#ffffff"));
        painter.drawPath(play);
    }

    painter.end();
    return pixmap;
}
} // namespace

UiHome::UiHome(QWidget* parent) :
    QMainWindow(parent)
{
    buildUi();
    applyStyle();
    initConnect();

    // 设备数据回填期间抑制写回 / suppress write-back while loading device data
    m_UiReady = false;
    loadDeviceData();
    m_UiReady = true;
}

UiHome::~UiHome()
{
    // 先停止定时采集，避免回调落到已析构的 DataInterface
    // stop the polling timer before this object (and its DataInterface) goes away
    if (epMachine != nullptr)
    {
        epMachine->unregisterDataInterface();
    }
}

// ==================== UI 构建 / UI construction ====================

void UiHome::buildUi()
{
    auto* central = new QWidget(this);
    central->setObjectName("centralwidget");

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(buildTopBar());

    m_pStackMain = new QStackedWidget;
    m_pStackMain->addWidget(buildSeparatePage());  // PageSeparate
    m_pStackMain->addWidget(buildCooperatePage()); // PageCooperate
    m_pStackMain->addWidget(buildChartPage());     // PageChart
    m_pStackMain->addWidget(buildProcessPage());   // PageProcess
    m_pStackMain->addWidget(buildSystemPage());    // PageSystem
    m_pStackMain->addWidget(buildDataPage());      // PageData
    root->addWidget(m_pStackMain, 1);

    root->addWidget(buildBottomBar());

    setCentralWidget(central);
    setWindowTitle(QStringLiteral("远程控制"));
    resize(1024, 600);
    setMinimumSize(1024, 600);
}

QWidget* UiHome::buildTopBar()
{
    auto* bar = new QWidget;
    bar->setObjectName("topBar");
    bar->setFixedHeight(58);

    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(18, 4, 16, 2);
    layout->setSpacing(0);

    layout->addStretch(1);

    auto* title = new QLabel(QStringLiteral("自动热压机"));
    title->setObjectName("labelTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    layout->addStretch(1);

    auto* info = new QWidget;
    auto* infoLayout = new QVBoxLayout(info);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(0);

    m_pLabelDateTime = new QLabel(QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss"));
    m_pLabelDateTime->setObjectName("labelDateTime");
    m_pLabelDateTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_pLabelVersion = new QLabel(QStringLiteral("V3.1.4"));
    m_pLabelVersion->setObjectName("labelVersion");
    m_pLabelVersion->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    infoLayout->addWidget(m_pLabelDateTime);
    infoLayout->addWidget(m_pLabelVersion);
    layout->addWidget(info);

    return bar;
}

QWidget* UiHome::buildSeparatePage()
{
    auto* page = new QWidget;
    auto* main = new QHBoxLayout(page);
    main->setContentsMargins(12, 10, 12, 10);
    main->setSpacing(10);

    // ---------------- 左侧压力信息 / left pressure panel ----------------
    auto* left = new QWidget;
    left->setFixedWidth(208);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(9);

    m_pLabelPressStepHdr = new QLabel(QStringLiteral("当前阶段 0/0"));
    m_pLabelPressStepHdr->setObjectName("panelHeader");
    m_pLabelPressStepHdr->setAlignment(Qt::AlignCenter);
    m_pLabelPressStepHdr->setFixedHeight(46);
    leftLayout->addWidget(m_pLabelPressStepHdr);

    leftLayout->addWidget(createInfoRow(QStringLiteral("设定压力\n(Ton)"), &m_pLabelSetPress, "0.0"));
    leftLayout->addWidget(createInfoRow(QStringLiteral("补压设定\n(Ton)"), &m_pLabelAfterPress, "0.0"));
    leftLayout->addWidget(createInfoRow(QStringLiteral("实时压力\n(Ton)"), &m_pLabelRealPress, "0.0"));
    leftLayout->addWidget(createInfoRow(QStringLiteral("模具压强\n(MPa)"), &m_pLabelMoldPress, "0.00"));
    leftLayout->addWidget(createInfoRow(QStringLiteral("保压计时\n(mm:ss)"), &m_pLabelPressTimer, "0:00"));
    leftLayout->addStretch(1);
    main->addWidget(left);

    // ---------------- 中间步骤表 / center step tables ----------------
    auto* center = new QWidget;
    auto* centerLayout = new QHBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(8);

    m_pTablePressSteps = createTable(QStringList()
                                     << QStringLiteral("阶段")
                                     << QStringLiteral("压力\n(Ton)")
                                     << QStringLiteral("时间\n(mm:ss)"));
    m_pTableTempSteps = createTable(QStringList()
                                    << QStringLiteral("阶段")
                                    << QStringLiteral("温度设置\n(°C)")
                                    << QStringLiteral("速率|时间\n(mm:ss)"));
    // 温度表行高随可用高度拉伸，铺满右侧区域 / stretch temperature rows to fill the panel
    m_pTableTempSteps->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    centerLayout->addWidget(m_pTablePressSteps, 1);
    centerLayout->addWidget(m_pTableTempSteps, 1);
    main->addWidget(center, 1);

    // ---------------- 右侧温度信息 / right temperature panel ----------------
    auto* right = new QWidget;
    right->setFixedWidth(208);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(9);

    m_pLabelTempStepHdr = new QLabel(QStringLiteral("当前阶段 0/0"));
    m_pLabelTempStepHdr->setObjectName("panelHeader");
    m_pLabelTempStepHdr->setAlignment(Qt::AlignCenter);
    m_pLabelTempStepHdr->setFixedHeight(46);
    rightLayout->addWidget(m_pLabelTempStepHdr);

    rightLayout->addWidget(createInfoRow(QStringLiteral("设定温度\n(°C)"), &m_pLabelSetTemp, "0.0"));
    rightLayout->addWidget(createInfoRow(QStringLiteral("上板状态\n(°C)"), &m_pLabelUpTemp, "0.0"));
    rightLayout->addWidget(createInfoRow(QStringLiteral("下板状态\n(°C)"), &m_pLabelDownTemp, "0.0"));
    rightLayout->addWidget(createInfoRow(QStringLiteral("保温计时\n(mm:ss)"), &m_pLabelTempTimer,
                                         QStringLiteral("0:00\n0:00"), 100, 72));
    rightLayout->addStretch(1);
    main->addWidget(right);

    return page;
}

QWidget* UiHome::buildCooperatePage()
{
    auto* page = new QWidget;
    auto* main = new QHBoxLayout(page);
    main->setContentsMargins(12, 10, 12, 10);
    main->setSpacing(10);

    // ---------------- 左侧信息面板 / left info panel ----------------
    auto* left = new QWidget;
    left->setFixedWidth(238);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    m_pLabelMixStepHdr = new QLabel(QStringLiteral("当前阶段 0/0"));
    m_pLabelMixStepHdr->setObjectName("panelHeader");
    m_pLabelMixStepHdr->setAlignment(Qt::AlignCenter);
    m_pLabelMixStepHdr->setFixedHeight(46);
    leftLayout->addWidget(m_pLabelMixStepHdr);

    leftLayout->addWidget(createDualRow(QStringLiteral("设定压力\n(Ton)"), &m_pLabelSetPressA,
                                        &m_pLabelSetPressB, "0.0", "0.0", 100, 56));
    leftLayout->addWidget(createInfoRow(QStringLiteral("实时压力\n(Ton)"), &m_pLabelRealPressC, "0.0",
                                        100, 56));
    leftLayout->addWidget(createInfoRow(QStringLiteral("模具压强\n(MPa)"), &m_pLabelMoldPressC, "0.00",
                                        100, 56));
    leftLayout->addWidget(createDualRow(QStringLiteral("上板状态\n(°C)"), &m_pLabelUpTempA,
                                        &m_pLabelUpTempB, "0.0", "0.0", 100, 56));
    leftLayout->addWidget(createDualRow(QStringLiteral("下板状态\n(°C)"), &m_pLabelDownTempA,
                                        &m_pLabelDownTempB, "0.0", "0.0", 100, 56));
    leftLayout->addWidget(createInfoRow(QStringLiteral("计时\n(mm:ss)"), &m_pLabelMixTimer, "0:00",
                                        100, 56));
    leftLayout->addStretch(1);
    main->addWidget(left);

    // ---------------- 中间混合步骤表 / center mixed step table ----------------
    m_pTableMixSteps = createTable(QStringList()
                                   << QStringLiteral("阶段")
                                   << QStringLiteral("是否加压")
                                   << QStringLiteral("压力\n(Ton)")
                                   << QStringLiteral("补压设定\n(Ton)")
                                   << QStringLiteral("是否加热")
                                   << QStringLiteral("设定温度\n(°C)")
                                   << QStringLiteral("计时\n(mm:ss)"));
    main->addWidget(m_pTableMixSteps, 1);

    return page;
}

QWidget* UiHome::buildChartPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto* header = new QLabel(QStringLiteral("列表/曲线"));
    header->setObjectName("panelHeader");
    header->setAlignment(Qt::AlignCenter);
    header->setFixedHeight(40);
    layout->addWidget(header);

    m_pChartView = new PressChartView;
    layout->addWidget(m_pChartView, 1);
    return page;
}

QWidget* UiHome::buildProcessPage()
{
    auto* page = new QWidget;
    auto* main = new QHBoxLayout(page);
    main->setContentsMargins(12, 10, 12, 10);
    main->setSpacing(10);

    // ---------------- 阶段控制 / step control ----------------
    auto* stepGroup = new QGroupBox(QStringLiteral("阶段控制"));
    auto* stepLayout = new QVBoxLayout(stepGroup);
    stepLayout->setContentsMargins(10, 18, 10, 10);
    stepLayout->setSpacing(8);

    auto* countRow = new QHBoxLayout;
    countRow->addWidget(new QLabel(QStringLiteral("阶段数")));
    m_pSpinStepCount = new QSpinBox;
    m_pSpinStepCount->setRange(1, 30);
    m_pSpinStepCount->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pSpinStepCount->setAlignment(Qt::AlignCenter);
    countRow->addWidget(m_pSpinStepCount, 1);
    stepLayout->addLayout(countRow);

    m_pTableStepSettings = new StepTableWidget;
    stepLayout->addWidget(m_pTableStepSettings, 1);
    main->addWidget(stepGroup, 3);

    // ---------------- 模具与操作 / mold & operation ----------------
    auto* moldGroup = new QGroupBox(QStringLiteral("模具与操作"));
    auto* moldLayout = new QVBoxLayout(moldGroup);
    moldLayout->setContentsMargins(10, 18, 10, 10);
    moldLayout->setSpacing(10);

    auto* typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel(QStringLiteral("模具类型")));
    m_pComboMoldType = new QComboBox;
    m_pComboMoldType->addItems(QStringList() << QStringLiteral("异形")
                                             << QStringLiteral("圆形")
                                             << QStringLiteral("矩形")
                                             << QStringLiteral("环形"));
    typeRow->addWidget(m_pComboMoldType, 1);
    moldLayout->addLayout(typeRow);

    m_pStackMold = new QStackedWidget;

    auto* pageSpecial = new QWidget;
    auto* specialLayout = new QVBoxLayout(pageSpecial);
    specialLayout->addWidget(new QLabel(QStringLiteral("异形模具无需尺寸参数")));
    specialLayout->addStretch(1);
    m_pStackMold->addWidget(pageSpecial);

    auto* pageCircle = new QWidget;
    auto* circleLayout = new QHBoxLayout(pageCircle);
    circleLayout->setContentsMargins(0, 0, 0, 0);
    circleLayout->addWidget(new QLabel(QStringLiteral("半径")));
    m_pDBoxDiameter = new QDoubleSpinBox;
    m_pDBoxDiameter->setRange(0.0, 1000.0);
    m_pDBoxDiameter->setDecimals(1);
    m_pDBoxDiameter->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pDBoxDiameter->setAlignment(Qt::AlignCenter);
    circleLayout->addWidget(m_pDBoxDiameter, 1);
    circleLayout->addWidget(new QLabel(QStringLiteral("mm")));
    m_pStackMold->addWidget(pageCircle);

    auto* pageRect = new QWidget;
    auto* rectLayout = new QVBoxLayout(pageRect);
    rectLayout->setContentsMargins(0, 0, 0, 0);
    auto* rectARow = new QHBoxLayout;
    rectARow->addWidget(new QLabel(QStringLiteral("长")));
    m_pDBoxA = new QDoubleSpinBox;
    m_pDBoxA->setRange(0.0, 1000.0);
    m_pDBoxA->setDecimals(1);
    m_pDBoxA->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pDBoxA->setAlignment(Qt::AlignCenter);
    rectARow->addWidget(m_pDBoxA, 1);
    rectARow->addWidget(new QLabel(QStringLiteral("mm")));
    rectLayout->addLayout(rectARow);
    auto* rectBRow = new QHBoxLayout;
    rectBRow->addWidget(new QLabel(QStringLiteral("宽")));
    m_pDBoxB = new QDoubleSpinBox;
    m_pDBoxB->setRange(0.0, 1000.0);
    m_pDBoxB->setDecimals(1);
    m_pDBoxB->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pDBoxB->setAlignment(Qt::AlignCenter);
    rectBRow->addWidget(m_pDBoxB, 1);
    rectBRow->addWidget(new QLabel(QStringLiteral("mm")));
    rectLayout->addLayout(rectBRow);
    m_pStackMold->addWidget(pageRect);

    auto* pageRing = new QWidget;
    auto* ringLayout = new QVBoxLayout(pageRing);
    ringLayout->setContentsMargins(0, 0, 0, 0);
    auto* ringOutRow = new QHBoxLayout;
    ringOutRow->addWidget(new QLabel(QStringLiteral("外径")));
    m_pDBoxOuterD = new QDoubleSpinBox;
    m_pDBoxOuterD->setRange(0.0, 1000.0);
    m_pDBoxOuterD->setDecimals(1);
    m_pDBoxOuterD->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pDBoxOuterD->setAlignment(Qt::AlignCenter);
    ringOutRow->addWidget(m_pDBoxOuterD, 1);
    ringOutRow->addWidget(new QLabel(QStringLiteral("mm")));
    ringLayout->addLayout(ringOutRow);
    auto* ringInRow = new QHBoxLayout;
    ringInRow->addWidget(new QLabel(QStringLiteral("内径")));
    m_pDBoxInnerD = new QDoubleSpinBox;
    m_pDBoxInnerD->setRange(0.0, 1000.0);
    m_pDBoxInnerD->setDecimals(1);
    m_pDBoxInnerD->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pDBoxInnerD->setAlignment(Qt::AlignCenter);
    ringInRow->addWidget(m_pDBoxInnerD, 1);
    ringInRow->addWidget(new QLabel(QStringLiteral("mm")));
    ringLayout->addLayout(ringInRow);
    m_pStackMold->addWidget(pageRing);

    moldLayout->addWidget(m_pStackMold);

    auto* demoldRow = new QHBoxLayout;
    demoldRow->addWidget(new QLabel(QStringLiteral("脱模压力")));
    m_pDBoxDemold = new QDoubleSpinBox;
    m_pDBoxDemold->setRange(0.0, 1000.0);
    m_pDBoxDemold->setDecimals(1);
    m_pDBoxDemold->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pDBoxDemold->setAlignment(Qt::AlignCenter);
    demoldRow->addWidget(m_pDBoxDemold, 1);
    demoldRow->addWidget(new QLabel(QStringLiteral("Ton")));
    moldLayout->addLayout(demoldRow);

    auto* buttonRow = new QHBoxLayout;
    m_pBtnDemold = new QPushButton(QStringLiteral("脱模"));
    m_pBtnDemold->setCheckable(true);
    m_pBtnControl = new QPushButton(QStringLiteral("控制面板"));
    buttonRow->addWidget(m_pBtnDemold, 1);
    buttonRow->addWidget(m_pBtnControl, 1);
    moldLayout->addLayout(buttonRow);
    moldLayout->addStretch(1);
    main->addWidget(moldGroup, 2);

    return page;
}

QWidget* UiHome::buildSystemPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(12);

    auto* group = new QGroupBox(QStringLiteral("系统设置"));
    auto* grid = new QGridLayout(group);
    grid->setContentsMargins(16, 20, 16, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    grid->addWidget(new QLabel(QStringLiteral("屏幕亮度")), 0, 0);
    m_pSliderBrightness = new QSlider(Qt::Horizontal);
    m_pSliderBrightness->setRange(0, 100);
    grid->addWidget(m_pSliderBrightness, 0, 1);
    m_pLabelBrightnessValue = new QLabel(QStringLiteral("--"));
    m_pLabelBrightnessValue->setFixedWidth(48);
    m_pLabelBrightnessValue->setAlignment(Qt::AlignCenter);
    grid->addWidget(m_pLabelBrightnessValue, 0, 2);

    grid->addWidget(new QLabel(QStringLiteral("按键提示音")), 1, 0);
    m_pChkSound = new QCheckBox(QStringLiteral("启用"));
    grid->addWidget(m_pChkSound, 1, 1);

    grid->addWidget(new QLabel(QStringLiteral("语言")), 2, 0);
    m_pComboLanguage = new QComboBox;
    m_pComboLanguage->addItems(QStringList() << QStringLiteral("中文") << QStringLiteral("English"));
    grid->addWidget(m_pComboLanguage, 2, 1);

    grid->addWidget(new QLabel(QStringLiteral("设备时间")), 3, 0);
    m_pLabelDeviceTime = new QLabel(QStringLiteral("----/--/-- --:--:--"));
    grid->addWidget(m_pLabelDeviceTime, 3, 1, 1, 2);

    auto* buttonRow = new QHBoxLayout;
    auto* readButton = new QPushButton(QStringLiteral("读取设备时间"));
    connect(readButton, &QPushButton::clicked, this, &UiHome::slotReadDeviceTime);
    auto* writeButton = new QPushButton(QStringLiteral("写入当前时间"));
    connect(writeButton, &QPushButton::clicked, this, &UiHome::slotWriteDeviceTime);
    buttonRow->addWidget(readButton);
    buttonRow->addWidget(writeButton);
    grid->addLayout(buttonRow, 4, 1, 1, 2);

    layout->addWidget(group);

    auto* note = new QLabel(QStringLiteral("说明：设备年份使用 tm_year（1900 偏移），写入时已自动换算。"));
    note->setObjectName("hintLabel");
    layout->addWidget(note);
    layout->addStretch(1);
    return page;
}

QWidget* UiHome::buildDataPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel(QStringLiteral("数据管理"));
    title->setObjectName("panelHeader");
    title->setAlignment(Qt::AlignCenter);
    title->setFixedHeight(44);
    layout->addWidget(title);

    auto* hint = new QLabel(QStringLiteral("数据管理功能暂未实现。"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setObjectName("hintLabel");
    layout->addWidget(hint, 1);
    return page;
}

QWidget* UiHome::buildBottomBar()
{
    auto* bar = new QWidget;
    bar->setObjectName("bottomBar");
    bar->setFixedHeight(82);

    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_pBtnStartPress = createNavButton(QStringLiteral("启动加压"), "btnStartPress");
    m_pBtnStartPress->setCheckable(true);
    m_pBtnStartPress->setProperty("green", true);

    m_pBtnMode = createNavButton(QStringLiteral("单独模式"), "btnMode");
    m_pBtnData = createNavButton(QStringLiteral("数据管理"), "btnData");
    m_pBtnChart = createNavButton(QStringLiteral("列表/曲线"), "btnChart");
    m_pBtnProcess = createNavButton(QStringLiteral("流程设置"), "btnProcess");
    m_pBtnSystem = createNavButton(QStringLiteral("系统设置"), "btnSystem");

    m_pBtnStartHeat = createNavButton(QStringLiteral("启动加热"), "btnStartHeat");
    m_pBtnStartHeat->setCheckable(true);
    m_pBtnStartHeat->setProperty("green", true);

    m_pBtnStartCooperate = createNavButton(QStringLiteral("启动"), "btnStartCooperate");
    m_pBtnStartCooperate->setCheckable(true);
    m_pBtnStartCooperate->setProperty("green", true);

    layout->addWidget(m_pBtnStartPress, 1);
    layout->addWidget(m_pBtnMode, 1);
    layout->addWidget(m_pBtnData, 1);
    layout->addWidget(m_pBtnChart, 1);
    layout->addWidget(m_pBtnProcess, 1);
    layout->addWidget(m_pBtnSystem, 1);
    layout->addWidget(m_pBtnStartHeat, 1);
    layout->addWidget(m_pBtnStartCooperate, 1);

    return bar;
}

QWidget* UiHome::createInfoRow(const QString& caption, QLabel** value, const QString& initial,
                               int labelWidth, int height)
{
    auto* row = new QWidget;
    row->setFixedHeight(height);

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* captionLabel = new QLabel(caption);
    captionLabel->setObjectName("infoCaption");
    captionLabel->setFixedWidth(labelWidth);
    captionLabel->setAlignment(Qt::AlignCenter);

    auto* valueLabel = new QLabel(initial);
    valueLabel->setObjectName("infoValue");
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueLabel->setContentsMargins(0, 0, 12, 0);

    layout->addWidget(captionLabel);
    layout->addWidget(valueLabel, 1);
    *value = valueLabel;
    return row;
}

QWidget* UiHome::createDualRow(const QString& caption, QLabel** value1, QLabel** value2,
                               const QString& initial1, const QString& initial2,
                               int labelWidth, int height)
{
    auto* row = new QWidget;
    row->setFixedHeight(height);

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* captionLabel = new QLabel(caption);
    captionLabel->setObjectName("infoCaption");
    captionLabel->setFixedWidth(labelWidth);
    captionLabel->setAlignment(Qt::AlignCenter);

    auto* first = new QLabel(initial1);
    first->setObjectName("infoValue");
    first->setAlignment(Qt::AlignCenter);

    auto* divider = new QFrame;
    divider->setObjectName("dualDivider");
    divider->setFrameShape(QFrame::VLine);
    divider->setFixedWidth(2);

    auto* second = new QLabel(initial2);
    second->setObjectName("infoValue");
    second->setAlignment(Qt::AlignCenter);

    layout->addWidget(captionLabel);
    layout->addWidget(first, 1);
    layout->addWidget(divider);
    layout->addWidget(second, 1);

    *value1 = first;
    *value2 = second;
    return row;
}

QTableWidget* UiHome::createTable(const QStringList& headers)
{
    auto* table = new QTableWidget(0, headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setFixedHeight(46);
    table->horizontalHeader()->setHighlightSections(false);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(78);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(true);
    table->setWordWrap(true);
    table->setAlternatingRowColors(false);
    return table;
}

QToolButton* UiHome::createNavButton(const QString& text, const QString& name)
{
    auto* button = new QToolButton;
    button->setObjectName(name);
    button->setProperty("nav", true);
    button->setProperty("active", false);
    button->setText(text);
    button->setIcon(QIcon(navIconPixmap(name)));
    button->setIconSize(QSize(30, 30));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setCursor(Qt::PointingHandCursor);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    button->setMinimumHeight(76);
    return button;
}

// ==================== 事件与数据 / events & data ====================

void UiHome::initConnect()
{
    // 主线程时钟 / clock in top bar
    auto* clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, &UiHome::slotClockTick);
    clock->start(1000);
    slotClockTick();

    // 底部导航 / bottom navigation
    connect(m_pBtnMode, &QPushButton::clicked, this, &UiHome::slotToggleMode);
    connect(m_pBtnData, &QPushButton::clicked, this, &UiHome::slotShowData);
    connect(m_pBtnChart, &QPushButton::clicked, this, &UiHome::slotShowChart);
    connect(m_pBtnProcess, &QPushButton::clicked, this, &UiHome::slotShowProcess);
    connect(m_pBtnSystem, &QPushButton::clicked, this, &UiHome::slotShowSystem);

    // 操作按钮 / action buttons
    connect(m_pBtnStartPress, &QPushButton::clicked, this, &UiHome::slotStartPress);
    connect(m_pBtnStartHeat, &QPushButton::clicked, this, &UiHome::slotStartHeat);
    connect(m_pBtnStartCooperate, &QPushButton::clicked, this, &UiHome::slotStartCooperate);
    connect(m_pBtnDemold, &QPushButton::clicked, this, &UiHome::slotDemold);

    // 流程设置写回 / process settings write-back
    connect(m_pSpinStepCount, QOverload<int>::of(&QSpinBox::valueChanged),
            m_pTableStepSettings, &StepTableWidget::slotSetValueChanged);
    connect(m_pSpinStepCount, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &UiHome::slotStepCountChanged);
    connect(m_pComboMoldType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UiHome::slotMoldTypeChanged);
    connect(m_pDBoxDiameter, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &UiHome::slotDiameterChanged);
    connect(m_pDBoxA, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &UiHome::slotAChanged);
    connect(m_pDBoxB, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &UiHome::slotBChanged);
    connect(m_pDBoxOuterD, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &UiHome::slotOuterDChanged);
    connect(m_pDBoxInnerD, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &UiHome::slotInnerDChanged);
    connect(m_pDBoxDemold, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &UiHome::slotDemoldValueChanged);
    connect(m_pTableStepSettings, &StepTableWidget::signalCellValueChanged,
            this, &UiHome::slotCellValueChanged);
    connect(m_pBtnControl, &QPushButton::clicked, this, &UiHome::slotControlDialog);

    // 系统设置 / system settings
    connect(m_pSliderBrightness, &QSlider::sliderReleased, this, &UiHome::slotBrightnessReleased);
    connect(m_pChkSound, &QCheckBox::toggled, this, &UiHome::slotSoundToggled);
    connect(m_pComboLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UiHome::slotLanguageChanged);

    // SDK 回调 / SDK callbacks
    connect(&m_DataInterface, &DataInterface::signalRealTimeData, this, &UiHome::slotRealTimeData);
    connect(&m_DataInterface, &DataInterface::signalPressData, this, &UiHome::slotPressData);
    connect(&m_DataInterface, &DataInterface::signalReadOnlyData, this, &UiHome::slotReadOnlyData);
    connect(&m_DataInterface, &DataInterface::signalError, this, &UiHome::slotError);

    updateBottomBar(true);
}

void UiHome::loadDeviceData()
{
    std::string json;
    if (!epMachine->getReadOnlyDataJson(json))
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("获取设备参数失败!"));
    }
    else
    {
        m_ReadOnlyData = DataInterface::parseReadOnlyData(json);
        m_pChartView->setRange(0, m_ReadOnlyData.m_MaxPLimit);
    }

    if (!epMachine->getPressDataJson(json))
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("获取设备配置失败!"));
    }
    else
    {
        m_PressData = DataInterface::parsePressData(json);
    }

    // 回填流程设置控件 / fill process setting widgets
    m_pSpinStepCount->setValue(m_PressData.m_PStep == 0 ? 1 : m_PressData.m_PStep);
    m_pComboMoldType->setCurrentIndex(m_PressData.m_Type);
    m_pStackMold->setCurrentIndex(m_PressData.m_Type);
    m_pDBoxDiameter->setValue(m_PressData.m_D);
    m_pDBoxA->setValue(m_PressData.m_A);
    m_pDBoxB->setValue(m_PressData.m_B);
    m_pDBoxOuterD->setValue(m_PressData.m_OuterD);
    m_pDBoxInnerD->setValue(m_PressData.m_InnerD);
    m_pDBoxDemold->setValue(m_PressData.m_DemoldValue);

    std::vector<float> upVec(std::begin(m_PressData.m_SetPValue), std::end(m_PressData.m_SetPValue));
    m_pTableStepSettings->setUpPressValue(upVec);
    std::vector<float> downVec(std::begin(m_PressData.m_AfterValue), std::end(m_PressData.m_AfterValue));
    m_pTableStepSettings->setDownPressValue(downVec);
    std::vector<uint32_t> timeVec(std::begin(m_PressData.m_KPTime), std::end(m_PressData.m_KPTime));
    m_pTableStepSettings->setTimePressValue(timeVec);
    m_pTableStepSettings->slotSetValueChanged(m_PressData.m_PStep);

    // 系统设置初值 / initial system values
    uint8_t brightness = 0;
    if (epMachine->getBrightness(brightness))
    {
        m_pSliderBrightness->setValue(brightness);
        m_pLabelBrightnessValue->setText(QString::number(brightness));
    }
    bool sound = false;
    if (epMachine->getButtonSound(sound))
    {
        m_pChkSound->setChecked(sound);
    }
    uint8_t language = 0;
    if (epMachine->getLanguage(language))
    {
        m_pComboLanguage->setCurrentIndex(language == 0 ? 0 : 1);
    }
    slotReadDeviceTime();

    refreshStepTables();

    // 启动 1 Hz 实时采集 / start the 1 Hz polling timer
    epMachine->registerDataInterface(&m_DataInterface, 1);
}

void UiHome::refreshStepTables()
{
    // ---------------- 压力步骤 / press steps ----------------
    const int pressSteps = qBound(1, static_cast<int>(m_PressData.m_PStep), static_cast<int>(MAX_PStep));
    m_pTablePressSteps->setRowCount(pressSteps);
    m_pLabelPressStepHdr->setText(QStringLiteral("当前阶段 %1/%2")
                                      .arg(m_RealTimeData.m_CPStep)
                                      .arg(pressSteps));
    for (int i = 0; i < pressSteps; ++i)
    {
        cellItem(m_pTablePressSteps, i, 0, QString::number(i + 1), QColor("#222222"), true);
        cellItem(m_pTablePressSteps, i, 1,
                 QString::number(m_PressData.m_SetPValue[i], 'f', 1));
        cellItem(m_pTablePressSteps, i, 2, fmtMs(m_PressData.m_KPTime[i]));
    }

    // ---------------- 温度步骤 / temperature steps ----------------
    m_TempSetPoints.clear();
    m_TempKTimes.clear();
    m_TempStepCount = 0;

    std::string tempJson;
    if (epMachine->getTempDataJson(tempJson))
    {
        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(tempJson), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject())
        {
            const QJsonObject obj = doc.object();
            m_TempStepCount = obj.value(QStringLiteral("TStep")).toInt();
            const QJsonArray temps = obj.value(QStringLiteral("StartTemp")).toArray();
            const QJsonArray speeds = obj.value(QStringLiteral("Speed")).toArray();
            const QJsonArray kTimes = obj.value(QStringLiteral("KTTime")).toArray();

            m_TempStepCount = qBound(0, m_TempStepCount, static_cast<int>(MAX_PStep));
            m_pTableTempSteps->setRowCount(m_TempStepCount);
            for (int i = 0; i < m_TempStepCount; ++i)
            {
                const double temperature = temps.at(i).toDouble();
                const double speed = speeds.at(i).toDouble();
                const uint32_t kTime = static_cast<uint32_t>(kTimes.at(i).toDouble());
                m_TempSetPoints.append(temperature);
                m_TempKTimes.append(kTime);

                cellItem(m_pTableTempSteps, i, 0, QString::number(i + 1), QColor("#222222"), true);
                cellItem(m_pTableTempSteps, i, 1, QString::number(temperature, 'f', 1));
                cellItem(m_pTableTempSteps, i, 2,
                         QStringLiteral("%1 | %2").arg(speed, 0, 'f', 1).arg(fmtMs(kTime)));
            }
        }
    }
    if (m_TempStepCount == 0)
    {
        m_pTableTempSteps->setRowCount(0);
    }
    m_pLabelTempStepHdr->setText(QStringLiteral("当前阶段 %1/%2")
                                     .arg(m_RealTimeData.m_CTStep)
                                     .arg(m_TempStepCount));

    // ---------------- 混合步骤 / mixed steps ----------------
    std::string mixJson;
    if (epMachine->getMixDataJson(mixJson))
    {
        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(mixJson), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject())
        {
            const QJsonObject obj = doc.object();
            const int mixSteps = qBound(0, obj.value(QStringLiteral("Step")).toInt(),
                                        static_cast<int>(MAX_PStep));
            const QJsonArray isPress = obj.value(QStringLiteral("IsPress")).toArray();
            const QJsonArray setP = obj.value(QStringLiteral("SetPValue")).toArray();
            const QJsonArray afterP = obj.value(QStringLiteral("AfterValue")).toArray();
            const QJsonArray isUpHeat = obj.value(QStringLiteral("IsUpHeat")).toArray();
            const QJsonArray upTemp = obj.value(QStringLiteral("UpTemp")).toArray();
            const QJsonArray isDownHeat = obj.value(QStringLiteral("IsDownHeat")).toArray();
            const QJsonArray downTemp = obj.value(QStringLiteral("DownTemp")).toArray();
            const QJsonArray kTime = obj.value(QStringLiteral("KTime")).toArray();

            m_pTableMixSteps->setRowCount(mixSteps);
            for (int i = 0; i < mixSteps; ++i)
            {
                const bool pressOn = isPress.at(i).toInt() != 0;
                const bool upOn = isUpHeat.at(i).toInt() != 0;
                const bool downOn = isDownHeat.at(i).toInt() != 0;

                cellItem(m_pTableMixSteps, i, 0, QString::number(i + 1), QColor("#222222"), true);
                cellItem(m_pTableMixSteps, i, 1, pressOn ? QStringLiteral("Y") : QStringLiteral("✗"),
                         pressOn ? QColor("#1a7fd4") : QColor("#d63b3b"));
                cellItem(m_pTableMixSteps, i, 2, QString::number(setP.at(i).toDouble(), 'f', 1));
                cellItem(m_pTableMixSteps, i, 3, QString::number(afterP.at(i).toDouble(), 'f', 1));
                m_pTableMixSteps->setCellWidget(i, 4,
                    heatCellWidget(upOn ? QStringLiteral("U:Y") : QStringLiteral("U:X"), upOn,
                                   downOn ? QStringLiteral("L:Y") : QStringLiteral("L:X"), downOn));
                cellItem(m_pTableMixSteps, i, 5,
                         QStringLiteral("%1\n%2").arg(upTemp.at(i).toDouble(), 0, 'f', 1)
                             .arg(downTemp.at(i).toDouble(), 0, 'f', 1));
                cellItem(m_pTableMixSteps, i, 6,
                         fmtMs(static_cast<uint32_t>(kTime.at(i).toDouble())));
            }
        }
    }
    m_pLabelMixStepHdr->setText(QStringLiteral("当前阶段 %1/%2")
                                    .arg(m_RealTimeData.m_CPStep)
                                    .arg(m_PressData.m_PStep));
}

void UiHome::slotRealTimeData(int errorCode, const RealTimeData& realTimeData)
{
    if (errorCode != 0)
    {
        qDebug() << Q_FUNC_INFO << "errorCode=" << errorCode;
        return;
    }
    m_RealTimeData = realTimeData;

    int pressIndex = realTimeData.m_CPStep;
    if (pressIndex < 0 || pressIndex >= static_cast<int>(MAX_PStep))
    {
        pressIndex = 0;
    }
    int tempIndex = realTimeData.m_CTStep;
    if (tempIndex < 0 || tempIndex >= m_TempSetPoints.size())
    {
        tempIndex = 0;
    }

    const double setPress = m_PressData.m_SetPValue[pressIndex];
    const double afterPress = m_PressData.m_AfterValue[pressIndex];
    const double setTemp = m_TempSetPoints.isEmpty() ? 0.0 : m_TempSetPoints.value(tempIndex);
    const uint32_t tempTarget = m_TempKTimes.isEmpty() ? 0 : m_TempKTimes.value(tempIndex);

    // 单独模式 / separate mode
    m_pLabelPressStepHdr->setText(QStringLiteral("当前阶段 %1/%2")
                                      .arg(realTimeData.m_CPStep).arg(m_PressData.m_PStep));
    m_pLabelSetPress->setText(QString::number(setPress, 'f', 1));
    m_pLabelAfterPress->setText(QString::number(afterPress, 'f', 1));
    m_pLabelRealPress->setText(QString::number(realTimeData.m_PressValue, 'f', 1));
    m_pLabelMoldPress->setText(QString::number(realTimeData.m_MoldPressure, 'f', 2));
    m_pLabelPressTimer->setText(fmtMs(realTimeData.m_PTime));

    m_pLabelTempStepHdr->setText(QStringLiteral("当前阶段 %1/%2")
                                     .arg(realTimeData.m_CTStep).arg(m_TempStepCount));
    m_pLabelSetTemp->setText(QString::number(setTemp, 'f', 1));
    m_pLabelUpTemp->setText(QString::number(realTimeData.m_TUpValue, 'f', 1));
    m_pLabelDownTemp->setText(QString::number(realTimeData.m_TDownValue, 'f', 1));
    m_pLabelTempTimer->setText(QStringLiteral("%1\n%2")
                                   .arg(fmtMs(realTimeData.m_TTime))
                                   .arg(fmtMs(tempTarget)));

    // 合作模式 / cooperation mode
    m_pLabelMixStepHdr->setText(QStringLiteral("当前阶段 %1/%2")
                                    .arg(realTimeData.m_CPStep).arg(m_PressData.m_PStep));
    m_pLabelSetPressA->setText(QString::number(setPress, 'f', 1));
    m_pLabelSetPressB->setText(QString::number(afterPress, 'f', 1));
    m_pLabelRealPressC->setText(QString::number(realTimeData.m_PressValue, 'f', 1));
    m_pLabelMoldPressC->setText(QString::number(realTimeData.m_MoldPressure, 'f', 2));
    m_pLabelUpTempA->setText(QString::number(setTemp, 'f', 1));
    m_pLabelUpTempB->setText(QString::number(realTimeData.m_TUpValue, 'f', 1));
    m_pLabelDownTempA->setText(QString::number(setTemp, 'f', 1));
    m_pLabelDownTempB->setText(QString::number(realTimeData.m_TDownValue, 'f', 1));
    m_pLabelMixTimer->setText(fmtMs(realTimeData.m_PTime));

    // 曲线 / chart
    m_pChartView->appendPressData(QPointF(static_cast<qreal>(m_ChartIndex++),
                                          realTimeData.m_PressValue));

    // 按钮状态跟随设备 / sync button states with the device
    m_pBtnStartPress->setChecked(realTimeData.m_IsPStart != 0);
    m_pBtnStartHeat->setChecked(realTimeData.m_IsTStart != 0);
    m_pBtnStartCooperate->setChecked(realTimeData.m_IsPStart != 0);
    m_pBtnStartPress->setEnabled(realTimeData.m_PressState == 0);
    m_pBtnStartCooperate->setEnabled(realTimeData.m_PressState == 0);
    m_pBtnDemold->setEnabled(realTimeData.m_PressState == 0);
}

void UiHome::slotPressData(int errorCode, const PressData& pressData)
{
    if (errorCode != 0)
    {
        qDebug() << Q_FUNC_INFO << "errorCode=" << errorCode;
        return;
    }

    m_UiReady = false;
    m_PressData = pressData;

    m_pSpinStepCount->setValue(pressData.m_PStep);
    m_pComboMoldType->setCurrentIndex(pressData.m_Type);
    m_pDBoxDiameter->setValue(pressData.m_D);
    m_pDBoxA->setValue(pressData.m_A);
    m_pDBoxB->setValue(pressData.m_B);
    m_pDBoxOuterD->setValue(pressData.m_OuterD);
    m_pDBoxInnerD->setValue(pressData.m_InnerD);
    m_pDBoxDemold->setValue(pressData.m_DemoldValue);

    std::vector<float> upVec(std::begin(pressData.m_SetPValue), std::end(pressData.m_SetPValue));
    m_pTableStepSettings->setUpPressValue(upVec);
    std::vector<float> downVec(std::begin(pressData.m_AfterValue), std::end(pressData.m_AfterValue));
    m_pTableStepSettings->setDownPressValue(downVec);
    std::vector<uint32_t> timeVec(std::begin(pressData.m_KPTime), std::end(pressData.m_KPTime));
    m_pTableStepSettings->setTimePressValue(timeVec);

    refreshStepTables();
    m_UiReady = true;
}

void UiHome::slotReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData)
{
    Q_UNUSED(registerNo);
    if (errorCode != 0)
    {
        return;
    }
    m_ReadOnlyData = readOnlyData;
    m_pDBoxDemold->setRange(0.0, readOnlyData.m_MaxPLimit / 2.0);
    m_pChartView->setRange(0, readOnlyData.m_MaxPLimit);
}

void UiHome::slotError(uint16_t cmdCode, std::vector<uint8_t> response)
{
    qDebug() << Q_FUNC_INFO << "cmd=" << QString::number(cmdCode, 16)
             << "responseSize=" << response.size();
}

void UiHome::slotClockTick()
{
    m_pLabelDateTime->setText(QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss"));
}

// ==================== 导航 / navigation ====================

void UiHome::slotToggleMode()
{
    // 单个模式按钮循环切换，保持底部导航与截图一致的 7 个按钮
    // one mode button toggles between modes, keeping the 7-button bar from the mockups
    if (m_SeparateMode)
    {
        updateBottomBar(false);
        showPage(PageCooperate);
    }
    else
    {
        updateBottomBar(true);
        showPage(PageSeparate);
    }
}

void UiHome::slotShowData()
{
    showPage(PageData);
}

void UiHome::slotShowChart()
{
    showPage(PageChart);
}

void UiHome::slotShowProcess()
{
    showPage(PageProcess);
}

void UiHome::slotShowSystem()
{
    showPage(PageSystem);
}

void UiHome::updateBottomBar(bool separateMode)
{
    m_SeparateMode = separateMode;
    m_pBtnStartPress->setVisible(separateMode);
    m_pBtnStartHeat->setVisible(separateMode);
    m_pBtnStartCooperate->setVisible(!separateMode);
    m_pBtnMode->setText(separateMode ? QStringLiteral("单独模式")
                                     : QStringLiteral("合作模式"));
    setActiveNav(m_pBtnMode, true);
}

void UiHome::setActiveNav(QToolButton* button, bool active)
{
    if (button == nullptr)
    {
        return;
    }
    button->setProperty("active", active);
    repolish(button);
}

void UiHome::showPage(int index)
{
    if (m_pStackMain != nullptr && index >= 0 && index < m_pStackMain->count())
    {
        m_pStackMain->setCurrentIndex(index);
    }
}

void UiHome::repolish(QWidget* widget)
{
    if (widget == nullptr || widget->style() == nullptr)
    {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

// ==================== 操作 / actions ====================

void UiHome::slotStartPress(bool isChecked)
{
    if (!epMachine->setPressing(isChecked))
    {
        m_pBtnStartPress->setChecked(!isChecked);
        qDebug() << Q_FUNC_INFO << "setPressing failed:" << epMachine->getLastErrorInfo();
    }
}

void UiHome::slotStartHeat(bool isChecked)
{
    if (!epMachine->setStartHeat(isChecked))
    {
        m_pBtnStartHeat->setChecked(!isChecked);
        qDebug() << Q_FUNC_INFO << "setStartHeat failed:" << epMachine->getLastErrorInfo();
    }
}

void UiHome::slotStartCooperate(bool isChecked)
{
    if (!epMachine->setPressing(isChecked))
    {
        m_pBtnStartCooperate->setChecked(!isChecked);
        qDebug() << Q_FUNC_INFO << "setPressing failed:" << epMachine->getLastErrorInfo();
    }
}

void UiHome::slotDemold(bool isChecked)
{
    if (!epMachine->setDemolding(isChecked))
    {
        m_pBtnDemold->setChecked(!isChecked);
        qDebug() << Q_FUNC_INFO << "setDemolding failed:" << epMachine->getLastErrorInfo();
    }
}

// ==================== 流程设置写回 / process write-back ====================

void UiHome::slotStepCountChanged(int value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_PStep = static_cast<uint8_t>(value);
    writePressData();
}

void UiHome::slotMoldTypeChanged(int value)
{
    if (m_pStackMold != nullptr)
    {
        m_pStackMold->setCurrentIndex(value);
    }
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_Type = static_cast<uint8_t>(value);
    writePressData();
}

void UiHome::slotDiameterChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_D = static_cast<float>(value);
    writePressData();
}

void UiHome::slotAChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_A = static_cast<float>(value);
    writePressData();
}

void UiHome::slotBChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_B = static_cast<float>(value);
    writePressData();
}

void UiHome::slotOuterDChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    // 固件 PD 无对应字段，仅同步本地缓存 / no firmware field, local cache only
    m_PressData.m_OuterD = static_cast<float>(value);
}

void UiHome::slotInnerDChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    // 固件 PD 无对应字段，仅同步本地缓存 / no firmware field, local cache only
    m_PressData.m_InnerD = static_cast<float>(value);
}

void UiHome::slotDemoldValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_DemoldValue = static_cast<float>(value);
    // 固件 PD JSON 无对应字段，仅同步本地缓存
    // no firmware PD field, local cache only
}

void UiHome::slotCellValueChanged(int row, int column, double value)
{
    if (!m_UiReady)
    {
        return;
    }
    bool ok = false;
    switch (row)
    {
        case 1: // 设定压力 / target pressure
            if (column < MAX_PStep)
            {
                m_PressData.m_SetPValue[column] = static_cast<float>(value);
                ok = writePressData();
            }
            break;
        case 2: // 保压值 / after pressure
            if (column < MAX_PStep)
            {
                m_PressData.m_AfterValue[column] = static_cast<float>(value);
                ok = writePressData();
            }
            break;
        case 3: // 保压时间(毫秒) / keep time (ms)
            if (column < MAX_PStep)
            {
                m_PressData.m_KPTime[column] = static_cast<uint32_t>(value);
                ok = writePressData();
            }
            break;
        default:
            return;
    }
    if (!ok)
    {
        qDebug() << Q_FUNC_INFO << "press data write failed, row=" << row
                 << " column=" << column;
    }
    refreshStepTables();
}

bool UiHome::writePressData()
{
    if (!epMachine->setPressDataJson(DataInterface::serializePressData(m_PressData)))
    {
        qDebug() << Q_FUNC_INFO << "setPressDataJson failed:" << epMachine->getLastErrorInfo();
        return false;
    }
    return true;
}

void UiHome::slotControlDialog()
{
    ControlDialog dialog(this);
    dialog.exec();
}

// ==================== 系统设置 / system settings ====================

void UiHome::slotBrightnessReleased()
{
    const int value = m_pSliderBrightness->value();
    m_pLabelBrightnessValue->setText(QString::number(value));
    if (!epMachine->setBrightness(static_cast<uint8_t>(value)))
    {
        qDebug() << Q_FUNC_INFO << "setBrightness failed:" << epMachine->getLastErrorInfo();
    }
}

void UiHome::slotSoundToggled(bool enabled)
{
    if (!m_UiReady)
    {
        return;
    }
    if (!epMachine->setButtonSound(enabled))
    {
        qDebug() << Q_FUNC_INFO << "setButtonSound failed:" << epMachine->getLastErrorInfo();
    }
}

void UiHome::slotLanguageChanged(int index)
{
    if (!m_UiReady)
    {
        return;
    }
    if (!epMachine->setLanguage(static_cast<uint8_t>(index == 0 ? 0 : 1)))
    {
        qDebug() << Q_FUNC_INFO << "setLanguage failed:" << epMachine->getLastErrorInfo();
    }
}

void UiHome::slotReadDeviceTime()
{
    DateTimeInfo dateTime{};
    if (!epMachine->getDateTime(dateTime))
    {
        m_pLabelDeviceTime->setText(QStringLiteral("读取失败"));
        return;
    }
    // 设备年份为 tm_year（1900 偏移）/ device year is tm_year (1900 offset)
    m_pLabelDeviceTime->setText(QStringLiteral("%1/%2/%3 %4:%5:%6")
                                    .arg(1900 + dateTime.m_Year)
                                    .arg(dateTime.m_Month, 2, 10, QChar('0'))
                                    .arg(dateTime.m_Day, 2, 10, QChar('0'))
                                    .arg(dateTime.m_Hour, 2, 10, QChar('0'))
                                    .arg(dateTime.m_Minute, 2, 10, QChar('0'))
                                    .arg(dateTime.m_Second, 2, 10, QChar('0')));
}

void UiHome::slotWriteDeviceTime()
{
    const QDateTime now = QDateTime::currentDateTime();
    DateTimeInfo dateTime{};
    dateTime.m_Second = static_cast<uint8_t>(now.time().second());
    dateTime.m_Minute = static_cast<uint8_t>(now.time().minute());
    dateTime.m_Hour = static_cast<uint8_t>(now.time().hour());
    dateTime.m_Day = static_cast<uint8_t>(now.date().day());
    dateTime.m_Month = static_cast<uint8_t>(now.date().month());
    dateTime.m_Year = static_cast<uint8_t>(now.date().year() - 1900);

    if (!epMachine->setDateTime(dateTime))
    {
        qDebug() << Q_FUNC_INFO << "setDateTime failed:" << epMachine->getLastErrorInfo();
        return;
    }
    slotReadDeviceTime();
}

// ==================== 收尾 / misc ====================

void UiHome::closeEvent(QCloseEvent* event)
{
    if (m_pStackMain != nullptr && m_pStackMain->currentIndex() != PageSeparate
        && m_pStackMain->currentIndex() != PageCooperate)
    {
        showPage(m_SeparateMode ? PageSeparate : PageCooperate);
        event->ignore();
        return;
    }

    const int result = QMessageBox::warning(this, QStringLiteral("提示"),
                                            QStringLiteral("是否关闭当前软件？"),
                                            QMessageBox::Yes, QMessageBox::No);
    if (result == QMessageBox::Yes)
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

QString UiHome::fmtMs(uint32_t ms) const
{
    const uint32_t totalSeconds = ms / 1000;
    return QStringLiteral("%1:%2").arg(totalSeconds / 60)
        .arg(totalSeconds % 60, 2, 10, QChar('0'));
}

// ==================== 样式 / style sheet ====================

void UiHome::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
QMainWindow {
    background: #e9e9e9;
}
QWidget#centralwidget {
    background: #e9e9e9;
}
QWidget#topBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #41b9e9, stop:1 #1e9ed8);
}
QLabel#labelTitle {
    color: #ffffff;
    font-family: "Microsoft YaHei";
    font-size: 24px;
    font-weight: bold;
}
QLabel#labelDateTime {
    color: #ffffff;
    font-family: "Microsoft YaHei";
    font-size: 15px;
    font-weight: bold;
}
QLabel#labelVersion {
    color: #e2f4fd;
    font-family: "Microsoft YaHei";
    font-size: 11px;
}
QLabel#panelHeader {
    color: #222222;
    font-family: "Microsoft YaHei";
    font-size: 17px;
    font-weight: bold;
}
QLabel#infoCaption {
    color: #ffffff;
    background: #1b9fdc;
    border-radius: 8px;
    font-family: "Microsoft YaHei";
    font-size: 13px;
    font-weight: bold;
}
QLabel#infoValue {
    color: #1b1b1b;
    background: #ffffff;
    border: 2px solid #1b9fdc;
    border-radius: 8px;
    font-family: "Microsoft YaHei";
    font-size: 18px;
    font-weight: bold;
}
QFrame#dualDivider {
    color: #1b9fdc;
}
QTableWidget {
    background: #d8d8d8;
    alternate-background-color: #ffffff;
    color: #222222;
    gridline-color: #dcdcdc;
    border: 1px solid #cfcfcf;
    font-family: "Microsoft YaHei";
    font-size: 15px;
}
QTableWidget::item {
    background: #ffffff;
    padding: 2px;
}
QHeaderView::section {
    background: #29a7e1;
    color: #ffffff;
    border: 1px solid #63c0ea;
    padding: 2px;
    font-family: "Microsoft YaHei";
    font-size: 13px;
    font-weight: bold;
}
QWidget#bottomBar {
    background: #0f86c4;
}
QToolButton[nav="true"] {
    color: #ffffff;
    background: transparent;
    border: none;
    border-right: 1px solid rgba(255, 255, 255, 0.28);
    font-family: "Microsoft YaHei";
    font-size: 15px;
    padding-top: 6px;
}
QToolButton[nav="true"]:hover {
    background: #1a95d3;
}
QToolButton[nav="true"]:pressed {
    background: #0a6d9f;
}
QToolButton[nav="true"][active="true"] {
    background: #0a6d9f;
    font-weight: bold;
}
QToolButton[nav="true"][green="true"] {
    background: #2e9d3a;
    font-size: 16px;
    font-weight: bold;
}
QToolButton[nav="true"][green="true"]:hover {
    background: #37b044;
}
QToolButton[nav="true"][green="true"]:checked {
    background: #1f7a2a;
}
QToolButton[nav="true"][green="true"]:disabled {
    background: #7fb98a;
}
QGroupBox {
    background: #f7fafc;
    border: 1px solid #c9dce8;
    border-radius: 8px;
    margin-top: 14px;
    font-family: "Microsoft YaHei";
    font-size: 14px;
    font-weight: bold;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 6px;
    color: #176b98;
}
QSpinBox, QDoubleSpinBox, QComboBox {
    background: #ffffff;
    border: 1px solid #b9d6e6;
    border-radius: 5px;
    padding: 4px 6px;
    min-height: 22px;
    font-family: "Microsoft YaHei";
    font-size: 14px;
}
QPushButton {
    font-family: "Microsoft YaHei";
    font-size: 14px;
    color: #ffffff;
    background: #1b9fdc;
    border: none;
    border-radius: 6px;
    padding: 6px 12px;
    min-height: 24px;
}
QPushButton:hover {
    background: #2eaee6;
}
QPushButton:pressed {
    background: #1283b8;
}
QPushButton:disabled {
    background: #a8cfe2;
}
QLabel#hintLabel {
    color: #7a8a94;
    font-family: "Microsoft YaHei";
    font-size: 13px;
}
QSlider::groove:horizontal {
    height: 6px;
    background: #cfe4ef;
    border-radius: 3px;
}
QSlider::sub-page:horizontal {
    background: #1b9fdc;
    border-radius: 3px;
}
QSlider::handle:horizontal {
    width: 18px;
    margin: -6px 0;
    background: #ffffff;
    border: 2px solid #1b9fdc;
    border-radius: 9px;
}
)"));
}
