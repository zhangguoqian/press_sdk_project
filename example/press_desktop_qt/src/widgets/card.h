#pragma once

#include <QFrame>
#include "theme/icons.h"

class QHBoxLayout;
class QLabel;
class QVBoxLayout;

// 白底圆角卡片，统一标题栏（图标 + 标题 + 右侧工具区）
class Card : public QFrame
{
    Q_OBJECT
public:
    explicit Card(const QString &title = QString(),
                  AppIcon icon = AppIcon::Info,
                  QWidget *parent = nullptr);

    QVBoxLayout *contentLayout() const { return m_bodyLayout; }
    QHBoxLayout *headerLayout() const { return m_headerLayout; }

    void addHeaderWidget(QWidget *widget);
    void addHeaderSpacing(int px);
    void setHeaderVisible(bool visible);
    void setHeaderHeight(int height);
    void setHeaderBordered(bool bordered);
    void setBodyMargins(int left, int top, int right, int bottom);

private:
    QWidget *m_header = nullptr;
    QHBoxLayout *m_headerLayout = nullptr;
    QVBoxLayout *m_bodyLayout = nullptr;
};
