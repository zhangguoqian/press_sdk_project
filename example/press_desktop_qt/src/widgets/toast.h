#pragma once

#include <QFrame>

class QLabel;
class QTimer;
class QGraphicsOpacityEffect;

// 底部轻提示（深色胶囊，自动消失）
class Toast : public QFrame
{
    Q_OBJECT
public:
    explicit Toast(QWidget *parent = nullptr);

    void showMessage(const QString &text);

private:
    QLabel *m_text = nullptr;
    QTimer *m_timer = nullptr;
    QGraphicsOpacityEffect *m_opacity = nullptr;
};
