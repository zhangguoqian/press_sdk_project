//
// Created by 11518 on 2026/9/15.
//

#include "startstopbutton.h"

StartStopButton::StartStopButton(QWidget* parent) :
    QPushButton(parent)
{
    this->setCheckable(true);
}

StartStopButton::~StartStopButton()
{
}

void StartStopButton::setStartStopStateText(const QString& startText, const QString& stopText)
{
    m_StopText = stopText;
    m_StartText = startText;
    if (isChecked())
    {
        setText(m_StopText);
    }
    else
    {
        setText(m_StartText);
    }
}

void StartStopButton::setState(ButtonState state)
{
    if (state == BUTTON_CAN_START)
    {
        setEnabled(true);
        setChecked(false);
        setText(m_StartText);
    }
    else if (state == BUTTON_CAN_STOP)
    {
        setEnabled(true);
        setChecked(true);
        setText(m_StopText);
    }
}


