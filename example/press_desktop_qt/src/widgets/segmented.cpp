#include "widgets/segmented.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QPushButton>

Segmented::Segmented(const QStringList &options, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("seg"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(32);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(2);

    for (int i = 0; i < options.size(); ++i) {
        auto *button = new QPushButton(options.at(i), this);
        button->setObjectName(QStringLiteral("segBtn"));
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setFixedHeight(26);
        button->setFont(Theme::ui(12));
        button->setChecked(i == 0);
        layout->addWidget(button, 1);
        m_buttons.append(button);

        connect(button, &QPushButton::clicked, this, [this, i] {
            if (m_current == i)
                return;
            m_current = i;
            emit changed(i);
        });
    }
}

void Segmented::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_buttons.size())
        return;
    m_current = index;
    m_buttons.at(index)->setChecked(true);
}

void Segmented::setFillWidth(bool fill)
{
    for (auto *button : m_buttons) {
        button->setSizePolicy(fill ? QSizePolicy::Expanding : QSizePolicy::Preferred,
                              QSizePolicy::Fixed);
    }
}
