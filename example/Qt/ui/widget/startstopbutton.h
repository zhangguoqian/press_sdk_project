//
// Created by 11518 on 2026/9/15.
//

#ifndef PRESS_SDK_PROJECT_STARTSTOPBUTTON_H
#define PRESS_SDK_PROJECT_STARTSTOPBUTTON_H

#include <QPushButton>

enum ButtonState
{
    BUTTON_DEFAULT = 0,
    BUTTON_CAN_START = 1,
    BUTTON_CAN_STOP = 2,
};

class StartStopButton : public QPushButton
{
    Q_OBJECT
public:
    explicit StartStopButton(QWidget *parent = nullptr);
    ~StartStopButton() override;
    void setStartStopStateText(const QString &startText, const QString &stopText);
    void setState(ButtonState state);

private:
    QString m_StartText;
    QString m_StopText;
};



#endif //PRESS_SDK_PROJECT_STARTSTOPBUTTON_H