#include "pages/singlepage.h"
#include "core/format.h"
#include "core/machine_state.h"
#include "dialogs/overlaydialogs.h"
#include "theme/theme.h"
#include "widgets/card.h"
#include "widgets/kvlist.h"
#include "widgets/moldpanel.h"
#include "widgets/realtimechart.h"
#include "widgets/segmented.h"
#include "widgets/stagetable.h"
#include "widgets/stagetools.h"
#include "widgets/unitinput.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace {

QFrame *makeDivider(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background:#E3EAF0;"));
    return line;
}

QWidget *makeGroup(QWidget *parent, QVBoxLayout **outLayout,
                   int left = 14, int top = 12, int right = 14, int bottom = 14)
{
    auto *group = new QWidget(parent);
    auto *layout = new QVBoxLayout(group);
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(0);
    *outLayout = layout;
    return group;
}

} // namespace

SinglePage::SinglePage(MachineState *state, QWidget *parent)
    : QWidget(parent), m_state(state)
{
    setObjectName(QStringLiteral("page"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    m_leftColumn = buildLeftColumn();
    m_rightColumn = buildRightColumn();
    layout->addWidget(m_leftColumn, 0);
    layout->addWidget(buildMiddleColumn(), 1);
    layout->addWidget(m_rightColumn, 0);

    connect(m_state, &MachineState::settingsChanged, this, [this] {
        m_pressureTable->setColumnDecimals(1, m_state->pDecimals);
        m_pressureTable->setColumnDecimals(2, m_state->pDecimals);
        refreshSettings();
        refreshLive();
    });
    connect(m_state, &MachineState::pressProgramChanged, this, [this] {
        rebuildPressureTable();
        m_pressureTable->setColumnDecimals(1, m_state->pDecimals);
        m_pressureTable->setColumnDecimals(2, m_state->pDecimals);
        refreshLive();
    });
    connect(m_state, &MachineState::tempProgramChanged, this, [this] {
        rebuildTemperatureTable();
        refreshLive();
    });
    connect(m_state, &MachineState::changed, this, &SinglePage::refreshLive);

    rebuildPressureTable();
    rebuildTemperatureTable();
    m_pressureTable->setColumnDecimals(1, m_state->pDecimals);
    m_pressureTable->setColumnDecimals(2, m_state->pDecimals);
    refreshSettings();
    refreshLive();

    // 中间列固定内容高度：两张阶段卡 275 + 间距 14 + 曲线卡 230 + 间距 14
    // 明确最小高度，保证窗口较矮时页面可滚动，曲线卡片不会被永久裁掉
    setMinimumHeight(275 + 14 + 275 + 14 + 230);
    updateColumnWidth();
}

void SinglePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateColumnWidth();
}

void SinglePage::updateColumnWidth()
{
    if (!m_leftColumn || !m_rightColumn)
        return;
    // 与原型保持一致：1500/1320/1200 三档收窄左右列，允许窗口自由缩放
    const int windowWidth = window() ? window()->width() : width();
    const int column = windowWidth < 1200 ? 232
                     : windowWidth < 1320 ? 272
                     : windowWidth < 1500 ? 300
                                          : 330;
    if (m_leftColumn->width() != column)
        m_leftColumn->setFixedWidth(column);
    if (m_rightColumn->width() != column)
        m_rightColumn->setFixedWidth(column);
}

QWidget *SinglePage::buildLeftColumn()
{
    auto *column = new QWidget(this);
    column->setFixedWidth(330);
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto *card = new Card(QStringLiteral("压力参数"), AppIcon::Gauge, column);
    QVBoxLayout *body = card->contentLayout();

    // 参数
    QVBoxLayout *groupLayout = nullptr;
    QWidget *group = makeGroup(card, &groupLayout);
    m_pressureKv = new KvList(group);
    m_pressureKv->setRowHeight(35);
    m_pressureKv->addRow(QStringLiteral("setP"), QStringLiteral("设定压力"),
                         QStringLiteral("Ton"));
    m_pressureKv->addRow(QStringLiteral("boost"), QStringLiteral("补压设定"),
                         QStringLiteral("Ton"));
    m_pressureKv->addRow(QStringLiteral("live"), QStringLiteral("实时压力"),
                         QStringLiteral("Ton"), Theme::Brand);
    m_pressureKv->addRow(QStringLiteral("mold"), QStringLiteral("模具压强"),
                         QStringLiteral("MPa"));
    m_pressureKv->addRow(QStringLiteral("hold"), QStringLiteral("保压计时"),
                         QStringLiteral("mm:ss"));
    m_pressureKv->setValueColor(QStringLiteral("live"), Theme::Brand700);
    groupLayout->addWidget(m_pressureKv);
    body->addWidget(group);

    body->addWidget(makeDivider(card));

    // 模具类型（贴底）
    QVBoxLayout *moldLayout = nullptr;
    QWidget *moldGroup = makeGroup(card, &moldLayout);
    m_mold = new MoldPanel(m_state, moldGroup);
    moldLayout->addWidget(m_mold);
    body->addWidget(moldGroup);

    layout->addWidget(card, 1);
    return column;
}

QWidget *SinglePage::buildPressureCard()
{
    auto *card = new Card(QStringLiteral("压力阶段"), AppIcon::Press, this);
    m_pressureTools = new StageTools(card);
    card->addHeaderWidget(m_pressureTools);

    m_pressureTable = new StageTable(card);
    m_pressureTable->setColumns(
        {{QStringLiteral("压力"), QStringLiteral("Ton"), StageColumnType::Number, 1, 0.1, 30.0},
         {QStringLiteral("补压设定"), QStringLiteral("Ton"), StageColumnType::Number, 1, 0.0, 30.0},
         {QStringLiteral("计时"), QStringLiteral("mm:ss"), StageColumnType::Time, 0, 0, 35999}},
        64);
    m_pressureTable->setFixedHeight(36 + 5 * 39);
    card->contentLayout()->addWidget(m_pressureTable);
    card->setFixedHeight(44 + 36 + 5 * 39);

    connect(m_pressureTools, &StageTools::countChanged, this, [this](int count) {
        m_state->setPCount(count);
        rebuildPressureTable();
        refreshLive();
    });
    connect(m_pressureTable, &StageTable::numberEdited, this,
            [this](int row, int column, double value) {
                if (row < 0 || row >= m_state->pStages.size())
                    return;
                PressureStage &stage = m_state->pStages[row];
                if (column == 1) {
                    stage.p = value;
                    // 自动补压：补压设定 = 压力 - Max_Min
                    stage.boost = qMax(0.0, value - m_state->autoBoost);
                    m_pressureTable->setNumber(row, 2, stage.boost);
                } else if (column == 2) {
                    stage.boost = value;
                }
                m_state->commitPressureStage(row);
                refreshLive();
            });
    connect(m_pressureTable, &StageTable::timeClicked, this, [this](int row, int column) {
        if (row < 0 || row >= m_state->pStages.size())
            return;
        int seconds = m_state->pStages[row].hold;
        if (Dialogs::editTiming(this, seconds, &seconds)) {
            m_state->pStages[row].hold = seconds;
            m_state->commitPressureStage(row);
            m_pressureTable->setTime(row, column, seconds);
            refreshLive();
        }
    });

    return card;
}

QWidget *SinglePage::buildTemperatureCard()
{
    auto *card = new Card(QStringLiteral("温度阶段"), AppIcon::Flame, this);
    m_temperatureTools = new StageTools(card);
    card->addHeaderWidget(m_temperatureTools);

    m_temperatureTable = new StageTable(card);
    m_temperatureTable->setColumns(
        {{QStringLiteral("温度"), QStringLiteral("°C"), StageColumnType::Number, 1, 0.0, 310.0},
         {QStringLiteral("速率"), QStringLiteral("°C/min"), StageColumnType::Number, 1, 0.1, 50.0},
         {QStringLiteral("计时"), QStringLiteral("mm:ss"), StageColumnType::Time, 0, 0, 35999}},
        64);
    m_temperatureTable->setFixedHeight(36 + 5 * 39);
    card->contentLayout()->addWidget(m_temperatureTable);
    card->setFixedHeight(44 + 36 + 5 * 39);

    connect(m_temperatureTools, &StageTools::countChanged, this, [this](int count) {
        m_state->setTCount(count);
        rebuildTemperatureTable();
        refreshLive();
    });
    connect(m_temperatureTable, &StageTable::numberEdited, this,
            [this](int row, int column, double value) {
                if (row < 0 || row >= m_state->tStages.size())
                    return;
                TemperatureStage &stage = m_state->tStages[row];
                if (column == 1)
                    stage.t = value;
                else if (column == 2)
                    stage.rate = value;
                m_state->commitTemperatureStage(row);
                refreshLive();
            });
    connect(m_temperatureTable, &StageTable::timeClicked, this,
            [this](int row, int column) {
                if (row < 0 || row >= m_state->tStages.size())
                    return;
                int seconds = m_state->tStages[row].hold;
                if (Dialogs::editTiming(this, seconds, &seconds)) {
                    m_state->tStages[row].hold = seconds;
                    m_state->commitTemperatureStage(row);
                    m_temperatureTable->setTime(row, column, seconds);
                    refreshLive();
                }
            });

    return card;
}

QWidget *SinglePage::buildChartCard()
{
    auto *card = new Card(QStringLiteral("实时曲线"), AppIcon::Chart, this);
    card->setHeaderBordered(false);

    m_legend = new QLabel(card);
    m_legend->setTextFormat(Qt::RichText);
    m_legend->setFont(Theme::ui(11));
    m_legend->setContentsMargins(0, 0, 4, 0);
    card->addHeaderWidget(m_legend);

    auto *windowLabel = new QLabel(QStringLiteral("窗口 180 s"), card);
    windowLabel->setFont(Theme::num(10));
    windowLabel->setStyleSheet(QStringLiteral("color:#7C90A2;"));
    card->addHeaderWidget(windowLabel);

    m_chart = new RealtimeChart(card);
    card->contentLayout()->addWidget(m_chart, 1);
    // 高度随窗口伸展：窗口放大（全屏）时曲线卡片填满剩余空间，而不是留白
    card->setMinimumHeight(230);
    updateLegend();
    return card;
}

QWidget *SinglePage::buildMiddleColumn()
{
    auto *column = new QWidget(this);
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    layout->addWidget(buildPressureCard());
    layout->addWidget(buildTemperatureCard());
    layout->addWidget(buildChartCard(), 1);
    return column;
}

QWidget *SinglePage::buildTemperatureControlCard()
{
    auto *card = new Card(QStringLiteral("温控设置"), AppIcon::Temp, this);

    // 温度状态
    QVBoxLayout *groupLayout = nullptr;
    QWidget *group = makeGroup(card, &groupLayout);
    auto *sub = new QLabel(QStringLiteral("温度状态"), group);
    sub->setObjectName(QStringLiteral("subLabel"));
    sub->setFont(Theme::ui(11, QFont::DemiBold));
    sub->setContentsMargins(0, 2, 0, 8);
    groupLayout->addWidget(sub);

    m_temperatureKv = new KvList(group);
    m_temperatureKv->addRow(QStringLiteral("setT"), QStringLiteral("设定温度"),
                            QStringLiteral("°C"));
    m_temperatureKv->addRow(QStringLiteral("upT"), QStringLiteral("上板温度"),
                            QStringLiteral("°C"), Theme::Red);
    m_temperatureKv->addRow(QStringLiteral("downT"), QStringLiteral("下板温度"),
                            QStringLiteral("°C"), Theme::Amber);
    m_temperatureKv->addRow(QStringLiteral("holdT"), QStringLiteral("保温计时"),
                            QStringLiteral("mm:ss"));
    groupLayout->addWidget(m_temperatureKv);
    card->contentLayout()->addWidget(group);

    auto *divider = makeDivider(card);
    m_tempDivider = divider;
    card->contentLayout()->addWidget(divider);

    // 升温速率（设备不支持速率模式时隐藏）
    QVBoxLayout *rateLayout = nullptr;
    QWidget *rateGroup = makeGroup(card, &rateLayout);
    m_rateGroup = rateGroup;
    auto *rateSub = new QLabel(QStringLiteral("升温速率"), rateGroup);
    rateSub->setObjectName(QStringLiteral("subLabel"));
    rateSub->setFont(Theme::ui(11, QFont::DemiBold));
    rateSub->setContentsMargins(0, 2, 0, 8);
    rateLayout->addWidget(rateSub);

    m_rateSegmented = new Segmented({QStringLiteral("普通模式"),
                                     QStringLiteral("速率模式")},
                                    rateGroup);
    m_rateSegmented->setFillWidth(true);
    rateLayout->addWidget(m_rateSegmented);
    rateLayout->addSpacing(12);

    m_rateTip = new QLabel(rateGroup);
    m_rateTip->setObjectName(QStringLiteral("tipLabel"));
    m_rateTip->setWordWrap(true);
    m_rateTip->setFont(Theme::ui(11));
    rateLayout->addWidget(m_rateTip);
    card->contentLayout()->addWidget(rateGroup);

    card->contentLayout()->addStretch(1);

    // 水冷回路（贴底，设备不支持水冷时隐藏）
    QVBoxLayout *waterLayout = nullptr;
    QWidget *waterGroup = makeGroup(card, &waterLayout, 14, 12, 14, 14);
    m_waterGroup = waterGroup;
    auto *waterSub = new QLabel(QStringLiteral("水冷回路"), waterGroup);
    waterSub->setObjectName(QStringLiteral("subLabel"));
    waterSub->setFont(Theme::ui(11, QFont::DemiBold));
    waterSub->setContentsMargins(0, 2, 0, 8);
    waterLayout->addWidget(waterSub);

    auto makeWaterRow = [&](const QString &label, double value, double *target,
                            UnitInput **inputOut) {
        auto *row = new QWidget(waterGroup);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        auto *name = new QLabel(label, row);
        name->setFixedWidth(62);
        name->setFont(Theme::ui(12));
        name->setStyleSheet(QStringLiteral("color:#3B5770;"));

        auto *input = new UnitInput(QStringLiteral("°C"), row);
        input->setRange(0.0, 200.0, 1);
        input->setValue(value);
        connect(input, &UnitInput::valueChanged, this, [target](double v) {
            *target = v;
        });

        rowLayout->addWidget(name);
        rowLayout->addWidget(input, 1);
        *inputOut = input;
        return row;
    };
    waterLayout->addWidget(makeWaterRow(QStringLiteral("降温水温"), m_state->coolT,
                                        &m_state->coolT, &m_coolInput));
    waterLayout->addSpacing(10);
    waterLayout->addWidget(makeWaterRow(QStringLiteral("升温水温"), m_state->warmT,
                                        &m_state->warmT, &m_warmInput));
    waterLayout->addSpacing(10);

    auto *waterTip = new QLabel(
        QStringLiteral("水冷回路进水温度设定，用于降温与升温阶段的循环水温控制。"), waterGroup);
    waterTip->setObjectName(QStringLiteral("tipLabel"));
    waterTip->setWordWrap(true);
    waterTip->setFont(Theme::ui(11));
    waterLayout->addWidget(waterTip);

    card->contentLayout()->addWidget(waterGroup);

    connect(m_rateSegmented, &Segmented::changed, this, [this](int index) {
        m_state->setRateMode(index == 0 ? RateMode::Normal : RateMode::Fast);
    });

    return card;
}

QWidget *SinglePage::buildRightColumn()
{
    auto *column = new QWidget(this);
    column->setFixedWidth(330);
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    layout->addWidget(buildTemperatureControlCard(), 1);
    return column;
}

void SinglePage::rebuildPressureTable()
{
    m_pressureTools->setTotal(m_state->pCount);
    m_pressureTable->setStageRowCount(m_state->pCount);
    for (int i = 0; i < m_state->pCount; ++i) {
        const PressureStage &stage = m_state->pStages.at(i);
        m_pressureTable->setNumber(i, 1, stage.p);
        m_pressureTable->setNumber(i, 2, stage.boost);
        m_pressureTable->setTime(i, 3, stage.hold);
    }
}

void SinglePage::rebuildTemperatureTable()
{
    m_temperatureTools->setTotal(m_state->tCount);
    m_temperatureTable->setStageRowCount(m_state->tCount);
    for (int i = 0; i < m_state->tCount; ++i) {
        const TemperatureStage &stage = m_state->tStages.at(i);
        m_temperatureTable->setNumber(i, 1, stage.t);
        m_temperatureTable->setNumber(i, 2, stage.rate);
        m_temperatureTable->setTime(i, 3, stage.hold);
    }
}

void SinglePage::refreshSettings()
{
    const bool speed = m_state->hasSpeed;
    const bool fast = speed && m_state->rateMode == RateMode::Fast;

    // 按设备能力显隐：不支持速率则隐藏升温速率区并固定普通模式，不支持水冷则隐藏水冷回路
    if (m_rateGroup)
        m_rateGroup->setVisible(speed);
    if (m_waterGroup)
        m_waterGroup->setVisible(m_state->hasWater);
    if (m_tempDivider)
        m_tempDivider->setVisible(speed || m_state->hasWater);

    m_rateSegmented->setCurrentIndex(fast ? 1 : 0);
    m_temperatureTable->setColumnVisible(2, fast);
    m_rateTip->setText(fast
        ? QStringLiteral("速率模式：按各段速率（°C/min）线性升温，自动校核升温时长。")
        : QStringLiteral("普通模式：按温度段设定直接升温，不做速率限制。"));

    // 水温来自 TempData：MinWater 降温水温、MaxWater 升温水温
    if (m_coolInput)
        m_coolInput->setValue(m_state->coolT);
    if (m_warmInput)
        m_warmInput->setValue(m_state->warmT);
}

void SinglePage::refreshLive()
{
    const PressureStage &pressure = m_state->pStages.at(m_state->pressureIndex());
    m_pressureKv->setValue(QStringLiteral("setP"),
                           fmtNumber(pressure.p, m_state->pDecimals));
    m_pressureKv->setValue(QStringLiteral("boost"),
                           fmtNumber(pressure.boost, m_state->pDecimals));
    m_pressureKv->setValue(QStringLiteral("live"),
                           fmtNumber(m_state->liveP, m_state->pDecimals));
    const double moldPressure = m_state->moldPressure(m_state->liveP);
    m_pressureKv->setValue(QStringLiteral("mold"),
                           moldPressure < 0 ? QStringLiteral("—")
                                            : fmtNumber(moldPressure,
                                                        m_state->moldDecimals));
    m_pressureKv->setValue(QStringLiteral("hold"),
                           fmtTime(m_state->press ? m_state->pRemain : pressure.hold));

    const TemperatureStage &temperature = m_state->tStages.at(m_state->temperatureIndex());
    m_temperatureKv->setValue(QStringLiteral("setT"), fmtNumber(temperature.t));
    // 负值表示传感器无效/断线，显示占位符
    const auto tempText = [](double value) {
        return value < 0.0 ? QStringLiteral("—") : fmtNumber(value);
    };
    m_temperatureKv->setValue(QStringLiteral("upT"), tempText(m_state->upT));
    m_temperatureKv->setValue(QStringLiteral("downT"), tempText(m_state->downT));
    m_temperatureKv->setValue(QStringLiteral("holdT"),
                              fmtTime(m_state->heat ? m_state->tRemain : temperature.hold));

    const int pressureCurrent = m_state->press ? m_state->pStage : 0;
    const int temperatureCurrent = m_state->heat ? m_state->tStage : 0;
    m_pressureTools->setCurrent(pressureCurrent);
    m_temperatureTools->setCurrent(temperatureCurrent);
    m_pressureTable->setCurrentStage(pressureCurrent);
    m_temperatureTable->setCurrentStage(temperatureCurrent);

    updateLegend();
}

void SinglePage::resetChart()
{
    if (m_chart)
        m_chart->clear();
}

void SinglePage::onTick()
{
    m_chart->addSample(m_state->liveP, m_state->upT, m_state->downT);
    refreshLive();
}

void SinglePage::updateLegend()
{
    if (!m_legend)
        return;
    m_legend->setText(QStringLiteral(
        "<span style=\"color:#00A1E9;\">●</span>&nbsp;<b>%1</b>&nbsp;"
        "<span style=\"color:#5F7488;\">Ton</span>&nbsp;&nbsp;&nbsp;"
        "<span style=\"color:#C8102E;\">●</span>&nbsp;<b>%2</b>&nbsp;"
        "<span style=\"color:#5F7488;\">°C</span>&nbsp;&nbsp;&nbsp;"
        "<span style=\"color:#E8730C;\">●</span>&nbsp;<b>%3</b>&nbsp;"
        "<span style=\"color:#5F7488;\">°C</span>")
        .arg(fmtNumber(m_state->liveP, m_state->pDecimals),
             m_state->upT < 0.0 ? QStringLiteral("—") : fmtNumber(m_state->upT),
             m_state->downT < 0.0 ? QStringLiteral("—") : fmtNumber(m_state->downT)));
}
