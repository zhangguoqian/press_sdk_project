#pragma once

#include <QHash>
#include <QMainWindow>
#include <QString>

class ActionBar;
class CoopPage;
class MachineState;
class PressService;
class QScrollArea;
class QStackedWidget;
class SettingsPage;
class Sidebar;
class SinglePage;
class Toast;
class TitleBar;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void showPage(const QString &key);
    QString currentPage() const { return m_current; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    QWidget *createContent();
    QScrollArea *wrapPage(QWidget *page);
    void navigate(const QString &key);
    void updateActionBar();
    void syncModeAvailability();
    void showToast(const QString &text);
    void positionToast();

    void handlePressRequest();
    void handleHeatRequest();
    void handleVentRequest();
    void handleCoopRequest();

    MachineState *m_state = nullptr;
    PressService *m_service = nullptr;
    TitleBar *m_titleBar = nullptr;
    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    ActionBar *m_actionBar = nullptr;
    Toast *m_toast = nullptr;
    QWidget *m_root = nullptr;

    SinglePage *m_singlePage = nullptr;
    CoopPage *m_coopPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;

    QHash<QString, QWidget *> m_pageWidgets;
    QString m_current = QStringLiteral("single");
    bool m_didInitialScroll = false;
};
