#include "widgets/moldpanel.h"
#include "core/machine_state.h"
#include "theme/theme.h"
#include "widgets/segmented.h"
#include "widgets/unitinput.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

MoldPanel::MoldPanel(MachineState *state, QWidget *parent, bool coop)
    : QWidget(parent), m_state(state), m_coop(coop)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sub = new QLabel(QStringLiteral("模具类型"), this);
    sub->setObjectName(QStringLiteral("subLabel"));
    sub->setFont(Theme::ui(11, QFont::DemiBold));
    layout->addWidget(sub);
    layout->addSpacing(6);

    m_seg = new Segmented({QStringLiteral("异形"), QStringLiteral("圆形"),
                           QStringLiteral("矩形")},
                          this);
    layout->addWidget(m_seg);
    layout->addSpacing(8);

    // 异形
    m_specialPane = new QWidget(this);
    auto *specialLayout = new QVBoxLayout(m_specialPane);
    specialLayout->setContentsMargins(0, 0, 0, 0);
    specialLayout->setSpacing(0);
    auto *specialTip = new QLabel(
        QStringLiteral("异形模具按实际受力面积换算，模具压强显示为 —。"), m_specialPane);
    specialTip->setObjectName(QStringLiteral("tipLabel"));
    specialTip->setWordWrap(true);
    specialTip->setMinimumHeight(34);
    specialTip->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    specialLayout->addWidget(specialTip);
    layout->addWidget(m_specialPane);

    // 圆形
    m_circlePane = new QWidget(this);
    auto *circleLayout = new QVBoxLayout(m_circlePane);
    circleLayout->setContentsMargins(0, 0, 0, 0);
    circleLayout->setSpacing(10);
    m_diameter = new UnitInput(QStringLiteral("mm"), m_circlePane);
    m_diameter->setRange(1.0, 9999.0, 1);
    circleLayout->addWidget(createMeasureRow(QStringLiteral("直径"), m_diameter));
    layout->addWidget(m_circlePane);

    // 矩形
    m_rectPane = new QWidget(this);
    auto *rectLayout = new QVBoxLayout(m_rectPane);
    rectLayout->setContentsMargins(0, 0, 0, 0);
    rectLayout->setSpacing(10);
    m_length = new UnitInput(QStringLiteral("mm"), m_rectPane);
    m_length->setRange(1.0, 9999.0, 1);
    m_width = new UnitInput(QStringLiteral("mm"), m_rectPane);
    m_width->setRange(1.0, 9999.0, 1);
    rectLayout->addWidget(createMeasureRow(QStringLiteral("长"), m_length));
    rectLayout->addWidget(createMeasureRow(QStringLiteral("宽"), m_width));
    layout->addWidget(m_rectPane);

    layout->addSpacing(8);

    // 受力面积
    m_area = new QFrame(this);
    m_area->setObjectName(QStringLiteral("moldArea"));
    m_area->setAttribute(Qt::WA_StyledBackground, true);
    m_area->setFixedHeight(34);
    auto *areaLayout = new QHBoxLayout(m_area);
    areaLayout->setContentsMargins(10, 0, 10, 0);
    auto *areaLabel = new QLabel(QStringLiteral("受力面积"), m_area);
    areaLabel->setObjectName(QStringLiteral("moldAreaLabel"));
    m_areaValue = new QLabel(m_area);
    m_areaValue->setObjectName(QStringLiteral("moldAreaValue"));
    m_areaValue->setTextFormat(Qt::RichText);
    m_areaValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    areaLayout->addWidget(areaLabel);
    areaLayout->addStretch(1);
    areaLayout->addWidget(m_areaValue);
    layout->addWidget(m_area);

    // 校准值与排气时间（仅单独模式，位于模具类型区块下方）
    if (!m_coop) {
        layout->addSpacing(10);
        auto *pair = new QWidget(this);
        auto *pairLayout = new QHBoxLayout(pair);
        pairLayout->setContentsMargins(0, 0, 0, 0);
        pairLayout->setSpacing(10);

        const auto makeField = [pair](const QString &label, const QString &unit,
                                      UnitInput **out) {
            auto *column = new QWidget(pair);
            auto *columnLayout = new QVBoxLayout(column);
            columnLayout->setContentsMargins(0, 0, 0, 0);
            columnLayout->setSpacing(4);
            auto *name = new QLabel(label, column);
            name->setFont(Theme::ui(12));
            name->setStyleSheet(QStringLiteral("color:#3B5770;"));
            *out = new UnitInput(unit, column);
            columnLayout->addWidget(name);
            columnLayout->addWidget(*out);
            return column;
        };

        pairLayout->addWidget(makeField(QStringLiteral("校准值"), QStringLiteral("Ton"),
                                        &m_calibration), 1);
        pairLayout->addWidget(makeField(QStringLiteral("排气时间"), QStringLiteral("秒"),
                                        &m_airTime), 1);
        m_calibration->setRange(-3.0, 3.0, 1);
        m_airTime->setRange(1.0, 5.0, 0);

        layout->addWidget(pair);
    }

    connect(m_seg, &Segmented::changed, this, [this](int index) {
        if (m_syncing)
            return;
        const MoldShape shape = index == 0 ? MoldShape::Special
                              : (index == 1 ? MoldShape::Circle : MoldShape::Rect);
        if (m_coop)
            m_state->setCoopMoldShape(shape);
        else
            m_state->setMoldShape(shape);
    });
    connect(m_diameter, &UnitInput::valueChanged, this, [this](double v) {
        if (m_syncing)
            return;
        if (m_coop)
            m_state->setCoopMoldDiameter(v);
        else
            m_state->setMoldDiameter(v);
    });
    connect(m_length, &UnitInput::valueChanged, this, [this](double v) {
        if (m_syncing)
            return;
        if (m_coop)
            m_state->setCoopMoldLength(v);
        else
            m_state->setMoldLength(v);
    });
    connect(m_width, &UnitInput::valueChanged, this, [this](double v) {
        if (m_syncing)
            return;
        if (m_coop)
            m_state->setCoopMoldWidth(v);
        else
            m_state->setMoldWidth(v);
    });
    if (m_calibration) {
        connect(m_calibration, &UnitInput::valueChanged, this, [this](double v) {
            if (m_syncing)
                return;
            m_state->setCalibration(v);
        });
        connect(m_airTime, &UnitInput::valueChanged, this, [this](double v) {
            if (m_syncing)
                return;
            m_state->setAirTime(qRound(v));
        });
    }
    connect(m_state, &MachineState::settingsChanged, this, &MoldPanel::syncFromState);

    syncFromState();
}

QWidget *MoldPanel::createMeasureRow(const QString &label, UnitInput *input)
{
    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto *name = new QLabel(label, row);
    name->setFixedWidth(34);
    name->setFont(Theme::ui(12));
    name->setStyleSheet(QStringLiteral("color:#3B5770;"));

    layout->addWidget(name);
    layout->addWidget(input, 1);
    return row;
}

void MoldPanel::syncFromState()
{
    m_syncing = true;

    const MoldShape shape = m_coop ? m_state->coopMoldShape : m_state->moldShape;
    m_seg->setCurrentIndex(shape == MoldShape::Special ? 0
                           : (shape == MoldShape::Circle ? 1 : 2));

    m_specialPane->setVisible(shape == MoldShape::Special);
    m_circlePane->setVisible(shape == MoldShape::Circle);
    m_rectPane->setVisible(shape == MoldShape::Rect);
    m_area->setVisible(shape != MoldShape::Special);

    m_diameter->setValue(m_coop ? m_state->coopMoldDia : m_state->moldDia);
    m_length->setValue(m_coop ? m_state->coopMoldLen : m_state->moldLen);
    m_width->setValue(m_coop ? m_state->coopMoldWid : m_state->moldWid);
    if (m_calibration) {
        m_calibration->setValue(m_state->calibration);
        m_airTime->setValue(m_state->airTime);
    }

    m_areaValue->setText(QStringLiteral(
        "<span style=\"font-size:15px;font-weight:700;color:#0079B4;\">%1</span>"
        "<span style=\"font-size:10px;color:#5F7488;\">&nbsp;cm²</span>")
        .arg(m_coop ? m_state->coopMoldAreaText() : m_state->moldAreaText()));

    m_syncing = false;
}
