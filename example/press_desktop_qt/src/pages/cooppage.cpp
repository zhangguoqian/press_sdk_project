#include "pages/cooppage.h"
#include "core/format.h"
#include "core/machine_state.h"
#include "dialogs/overlaydialogs.h"
#include "theme/theme.h"
#include "widgets/card.h"
#include "widgets/kvlist.h"
#include "widgets/moldpanel.h"
#include "widgets/realtimechart.h"
#include "widgets/stagetable.h"
#include "widgets/stagetools.h"

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
} // namespace

CoopPage::CoopPage(MachineState *state, QWidget *parent)
    : QWidget(parent), m_state(state)
{
    setObjectName(QStringLiteral("page"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    m_leftColumn = buildLeftColumn();
    layout->addWidget(m_leftColumn, 0);
    layout->addWidget(buildRightColumn(), 1);

    connect(m_state, &MachineState::changed, this, &CoopPage::refreshLive);

    rebuildTable();
    refreshLive();
    updateColumnWidth();
}

void CoopPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateColumnWidth();
}

void CoopPage::updateColumnWidth()
{
    if (!m_leftColumn)
        return;
    // 与原型保持一致：1500/1320/1200 三档收窄左列，允许窗口自由缩放
    const int windowWidth = window() ? window()->width() : width();
    const int column = windowWidth < 1200 ? 232
                     : windowWidth < 1320 ? 272
                     : windowWidth < 1500 ? 300
                                          : 330;
    if (m_leftColumn->width() != column)
        m_leftColumn->setFixedWidth(column);
}

QWidget *CoopPage::buildLeftColumn()
{
    auto *column = new QWidget(this);
    column->setFixedWidth(330);
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto *card = new Card(QStringLiteral("实时状态"), AppIcon::Gauge, column);

    auto *group = new QWidget(card);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(14, 12, 14, 14);
    groupLayout->setSpacing(0);

    m_kv = new KvList(group);
    m_kv->addRow(QStringLiteral("setP"), QStringLiteral("设定压力"), QStringLiteral("Ton"));
    m_kv->addRow(QStringLiteral("live"), QStringLiteral("实时压力"), QStringLiteral("Ton"),
                 Theme::Brand);
    m_kv->addRow(QStringLiteral("mold"), QStringLiteral("模具压强"), QStringLiteral("MPa"));
    m_kv->addRow(QStringLiteral("setT"), QStringLiteral("设定温度"), QStringLiteral("°C"));
    m_kv->addRow(QStringLiteral("upT"), QStringLiteral("上板温度"), QStringLiteral("°C"),
                 Theme::Red);
    m_kv->addRow(QStringLiteral("downT"), QStringLiteral("下板温度"), QStringLiteral("°C"),
                 Theme::Amber);
    m_kv->addRow(QStringLiteral("hold"), QStringLiteral("计时"), QStringLiteral("mm:ss"));
    m_kv->setValueColor(QStringLiteral("live"), Theme::Brand700);
    groupLayout->addWidget(m_kv);
    card->contentLayout()->addWidget(group);

    card->contentLayout()->addWidget(makeDivider(card));
    card->contentLayout()->addStretch(1);

    auto *moldGroup = new QWidget(card);
    auto *moldLayout = new QVBoxLayout(moldGroup);
    moldLayout->setContentsMargins(14, 12, 14, 14);
    moldLayout->setSpacing(0);
    m_mold = new MoldPanel(m_state, moldGroup);
    moldLayout->addWidget(m_mold);
    card->contentLayout()->addWidget(moldGroup);

    layout->addWidget(card, 1);
    return column;
}

QWidget *CoopPage::buildTableCard()
{
    auto *card = new Card(QStringLiteral("合作流程明细"), AppIcon::Link, this);
    m_tools = new StageTools(card);
    card->addHeaderWidget(m_tools);

    m_table = new StageTable(card);
    m_table->setColumns(
        {{QStringLiteral("是否加压"), QString(), StageColumnType::Flag},
         {QStringLiteral("压力"), QStringLiteral("Ton"), StageColumnType::Number, 1, 0.1, 30.0},
         {QStringLiteral("补压设定"), QStringLiteral("Ton"), StageColumnType::Number, 1, 0.0, 30.0},
         {QStringLiteral("是否加热上板"), QString(), StageColumnType::Flag},
         {QStringLiteral("上板温度"), QStringLiteral("°C"), StageColumnType::Number, 1, 0.0, 310.0},
         {QStringLiteral("是否加热下板"), QString(), StageColumnType::Flag},
         {QStringLiteral("下板温度"), QStringLiteral("°C"), StageColumnType::Number, 1, 0.0, 310.0},
         {QStringLiteral("计时"), QStringLiteral("mm:ss"), StageColumnType::Time, 0, 0, 35999}},
        48);
    card->contentLayout()->addWidget(m_table, 1);

    connect(m_tools, &StageTools::countChanged, this, [this](int count) {
        m_state->setCoopCount(count);
        rebuildTable();
        refreshLive();
    });
    connect(m_table, &StageTable::numberEdited, this,
            [this](int row, int column, double value) {
                if (row < 0 || row >= m_state->coopRows.size())
                    return;
                CoopStage &stage = m_state->coopRows[row];
                if (column == 2)
                    stage.p = value;
                else if (column == 3)
                    stage.boost = value;
                else if (column == 5)
                    stage.tUp = value;
                else if (column == 7)
                    stage.tDown = value;
                m_state->commitCoopStage(row);
                refreshLive();
            });
    connect(m_table, &StageTable::flagToggled, this, [this](int row, int column) {
        if (row < 0 || row >= m_state->coopRows.size())
            return;
        CoopStage &stage = m_state->coopRows[row];
        const bool value = m_table->flag(row, column);
        if (column == 1)
            stage.press = value;
        else if (column == 4)
            stage.heatUp = value;
        else if (column == 6)
            stage.heatDown = value;
        m_state->commitCoopStage(row);
        refreshLive();
    });
    connect(m_table, &StageTable::timeClicked, this, [this](int row, int column) {
        if (row < 0 || row >= m_state->coopRows.size())
            return;
        int seconds = m_state->coopRows[row].hold;
        if (Dialogs::editTiming(this, seconds, &seconds)) {
            m_state->coopRows[row].hold = seconds;
            m_state->commitCoopStage(row);
            m_table->setTime(row, column, seconds);
            refreshLive();
        }
    });

    return card;
}

QWidget *CoopPage::buildChartCard()
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
    card->setFixedHeight(300);
    return card;
}

QWidget *CoopPage::buildRightColumn()
{
    auto *column = new QWidget(this);
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    layout->addWidget(buildTableCard(), 1);
    layout->addWidget(buildChartCard(), 0);
    return column;
}

void CoopPage::rebuildTable()
{
    m_tools->setTotal(m_state->coopCount);
    m_table->setStageRowCount(m_state->coopCount);
    for (int i = 0; i < m_state->coopCount; ++i) {
        const CoopStage &stage = m_state->coopRows.at(i);
        m_table->setFlag(i, 1, stage.press);
        m_table->setNumber(i, 2, stage.p);
        m_table->setNumber(i, 3, stage.boost);
        m_table->setFlag(i, 4, stage.heatUp);
        m_table->setNumber(i, 5, stage.tUp);
        m_table->setFlag(i, 6, stage.heatDown);
        m_table->setNumber(i, 7, stage.tDown);
        m_table->setTime(i, 8, stage.hold);
    }
}

void CoopPage::refreshLive()
{
    const CoopStage &stage = m_state->coopRows.at(m_state->coopIndex());
    m_kv->setValue(QStringLiteral("setP"), fmtNumber(stage.p));
    m_kv->setValue(QStringLiteral("live"), fmtNumber(m_state->liveP));
    const double moldPressure = m_state->moldPressure(stage.p);
    m_kv->setValue(QStringLiteral("mold"), moldPressure < 0 ? QStringLiteral("—")
                                                             : fmtNumber(moldPressure, 2));
    m_kv->setValue(QStringLiteral("setT"), fmtNumber(stage.tUp));
    m_kv->setValue(QStringLiteral("upT"), fmtNumber(m_state->upT));
    m_kv->setValue(QStringLiteral("downT"), fmtNumber(m_state->downT));
    m_kv->setValue(QStringLiteral("hold"), fmtTime(stage.hold));

    const int current = m_state->coopRunning ? m_state->coopStage : 0;
    m_tools->setCurrent(current);
    m_table->setCurrentStage(current);
    updateLegend();
}

void CoopPage::onTick()
{
    m_chart->addSample(m_state->liveP, m_state->upT, m_state->downT);
    refreshLive();
}

void CoopPage::updateLegend()
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
        .arg(fmtNumber(m_state->liveP), fmtNumber(m_state->upT),
             fmtNumber(m_state->downT)));
}
