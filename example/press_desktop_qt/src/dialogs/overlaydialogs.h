#pragma once

#include <QDialog>
#include "theme/icons.h"

class QFrame;

enum class DialogTone { Info, Warn, Bad };

// 覆盖在整个窗口上的模态弹窗基类（半透明遮罩 + 白色圆角卡片）
class OverlayDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OverlayDialog(QWidget *parent);

    QFrame *card() const { return m_card; }
    QWidget *buildHeader(AppIcon icon, const QString &title, DialogTone tone);

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QFrame *m_card = nullptr;
};

namespace Dialogs {

bool confirm(QWidget *parent, const QString &title, const QString &text,
             const QString &okText = QStringLiteral("确认"),
             DialogTone tone = DialogTone::Info,
             AppIcon icon = AppIcon::Info);

bool editTiming(QWidget *parent, int seconds, int *result);

} // namespace Dialogs
