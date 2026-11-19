#include "app/mainwindow.h"
#include "dialogs/overlaydialogs.h"
#include "theme/theme.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFontMetrics>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QSet>
#include <QStyleFactory>
#include <QTimer>

#include <algorithm>

namespace {

/** 页面上的模块化控件类名（用于控件截图标注） */
const QSet<QString> &controlClasses()
{
    static const QSet<QString> classes = {
        QStringLiteral("TitleBar"),      QStringLiteral("Sidebar"),
        QStringLiteral("ActionBar"),     QStringLiteral("Card"),
        QStringLiteral("KvList"),        QStringLiteral("MoldPanel"),
        QStringLiteral("RealtimeChart"), QStringLiteral("Segmented"),
        QStringLiteral("StageTable"),    QStringLiteral("StageTools"),
        QStringLiteral("UnitInput"),
    };
    return classes;
}

/** 弹窗内的控件类名 */
const QSet<QString> &dialogClasses()
{
    static const QSet<QString> classes = {
        QStringLiteral("OverlayDialog"), QStringLiteral("QPushButton"),
        QStringLiteral("QSpinBox"),      QStringLiteral("QLineEdit"),
    };
    return classes;
}

void waitMs(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

struct ControlEntry
{
    QString className;
    QRect rect;
};

/**
 * 对 root 内的目标控件逐个裁剪截图，并在每张图顶部标注类名；
 * 同时输出一张带框选标注的整页截图。
 */
void annotateWidgets(QWidget *root, const QSet<QString> &classes, const QString &outDir,
                     const QString &title)
{
    QDir().mkpath(outDir);
    const QPixmap base = root->grab();
    const qreal dpr = base.devicePixelRatio();

    QList<ControlEntry> entries;
    QList<QWidget *> widgets = root->findChildren<QWidget *>();
    widgets.prepend(root);
    for (QWidget *widget : widgets) {
        if (!widget->isVisible() || widget->width() < 2 || widget->height() < 2)
            continue;
        const QString className = QString::fromLatin1(widget->metaObject()->className());
        if (!classes.contains(className))
            continue;
        const QRect rect = QRect(widget->mapTo(root, QPoint(0, 0)), widget->size())
                               .intersected(root->rect());
        if (rect.width() < 4 || rect.height() < 4)
            continue;
        entries.append({className, rect});
    }
    if (entries.isEmpty())
        return;

    std::sort(entries.begin(), entries.end(), [](const ControlEntry &a, const ControlEntry &b) {
        if (a.rect.top() != b.rect.top())
            return a.rect.top() < b.rect.top();
        return a.rect.left() < b.rect.left();
    });

    QFont labelFont = QApplication::font();
    labelFont.setPointSize(8);
    labelFont.setBold(true);

    QHash<QString, int> counters;
    QStringList legend;
    QPixmap marked = base;
    QPainter painter(&marked);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(labelFont);

    QFont badgeFont = labelFont;
    badgeFont.setPointSize(7);
    int number = 0;

    for (const ControlEntry &entry : entries) {
        const int index = ++counters[entry.className];

        // ---- 单个控件截图：顶部深色标题条 + 控件画面 ----
        const QRect pixelRect(qRound(entry.rect.x() * dpr), qRound(entry.rect.y() * dpr),
                              qRound(entry.rect.width() * dpr),
                              qRound(entry.rect.height() * dpr));
        QPixmap crop = base.copy(pixelRect);
        crop.setDevicePixelRatio(1);
        const int header = 26;
        QPixmap shot(crop.width(), crop.height() + header);
        shot.fill(QColor(0xF4, 0xF8, 0xFB));
        QPainter shotPainter(&shot);
        shotPainter.fillRect(0, 0, shot.width(), header, QColor(0x0B, 0x2C, 0x40));
        QFont shotFont = labelFont;
        shotFont.setPointSize(9);
        shotPainter.setFont(shotFont);
        shotPainter.setPen(Qt::white);
        shotPainter.drawText(QRect(8, 0, shot.width() - 16, header),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             entry.className
                                 + (counters.value(entry.className) > 1
                                        ? QStringLiteral("  #%1").arg(index)
                                        : QString()));
        shotPainter.drawPixmap(0, header, crop);
        shotPainter.setPen(QColor(0xD7, 0xE2, 0xEB));
        shotPainter.drawRect(0, 0, shot.width() - 1, shot.height() - 1);
        shotPainter.end();
        shot.save(QStringLiteral("%1/%2_%3.png").arg(outDir, entry.className).arg(index));

        // ---- 整页标注：选框 + 序号徽标（类名见图例） ----
        ++number;
        const int badgeSize = 18;
        QRect badge(entry.rect.left() - badgeSize / 2, entry.rect.top() - badgeSize / 2,
                    badgeSize, badgeSize);
        badge.moveLeft(qMax(0, badge.left()));
        badge.moveTop(qMax(0, badge.top()));

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0x00, 0xA1, 0xE9, 0xDD), 2));
        painter.drawRect(entry.rect.adjusted(0, 0, -1, -1));

        painter.setBrush(QColor(0x00, 0x6E, 0xA8));
        painter.setPen(QPen(Qt::white, 1));
        painter.drawEllipse(badge.adjusted(1, 1, -1, -1));
        painter.setFont(badgeFont);
        painter.setPen(Qt::white);
        painter.drawText(badge, Qt::AlignCenter, QString::number(number));
        painter.setFont(labelFont);

        legend << (entry.className
                   + (counters.value(entry.className) > 1
                          ? QStringLiteral("  #%1").arg(index)
                          : QString()));
    }
    painter.end();

    // ---- 右侧图例：序号 → 类名 ----
    const int legendWidth = 280;
    const int rowHeight = 22;
    const int legendTop = 64;
    const int canvasHeight =
        qMax(marked.height(), legendTop + entries.size() * rowHeight + 20);
    QPixmap annotated(marked.width() + legendWidth, canvasHeight);
    annotated.fill(QColor(0xF4, 0xF7, 0xFA));
    QPainter canvas(&annotated);
    canvas.drawPixmap(0, 0, marked);
    canvas.fillRect(marked.width(), 0, legendWidth, canvasHeight, QColor(0x0B, 0x2C, 0x40));

    QFont titleFont = labelFont;
    titleFont.setPointSize(12);
    canvas.setFont(titleFont);
    canvas.setPen(Qt::white);
    canvas.drawText(QRect(18, 14, legendWidth - 36, 26), Qt::AlignLeft | Qt::AlignVCenter,
                    QStringLiteral("控件类名标注"));

    QFont subFont = labelFont;
    subFont.setPointSize(9);
    subFont.setBold(false);
    canvas.setFont(subFont);
    canvas.setPen(QColor(0x8F, 0xB3, 0xC8));
    canvas.drawText(QRect(18, 40, legendWidth - 36, 20), Qt::AlignLeft | Qt::AlignVCenter,
                    title);

    for (int i = 0; i < entries.size(); ++i) {
        const int y = legendTop + i * rowHeight;
        const QRect badge(marked.width() + 18, y + 2, 18, 18);
        canvas.setBrush(QColor(0x00, 0x6E, 0xA8));
        canvas.setPen(QPen(Qt::white, 1));
        canvas.drawEllipse(badge.adjusted(1, 1, -1, -1));
        canvas.setFont(badgeFont);
        canvas.setPen(Qt::white);
        canvas.drawText(badge, Qt::AlignCenter, QString::number(i + 1));

        canvas.setFont(subFont);
        canvas.setPen(QColor(0xE3, 0xF1, 0xF8));
        canvas.drawText(QRect(badge.right() + 10, y, legendWidth - 90, rowHeight),
                        Qt::AlignLeft | Qt::AlignVCenter, legend.at(i));
    }
    canvas.end();
    annotated.save(outDir + QStringLiteral("/annotated.png"));
}

/** 在模态弹窗打开期间完成截图，然后自动关闭弹窗 */
void scheduleDialogCapture(const QString &outDir, const QSet<QString> &classes,
                           const QString &title)
{
    QTimer::singleShot(350, [outDir, classes, title] {
        if (QWidget *dialog = QApplication::activeModalWidget()) {
            annotateWidgets(dialog, classes, outDir, title);
            dialog->close();
        }
    });
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("自动热压机 · 上位机"));
    app.setOrganizationName(QStringLiteral("AUTO HOT PRESS"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setFont(Theme::ui(13));
    app.setStyleSheet(Theme::styleSheet());

    MainWindow window;

    // 便于自动化截图校验：--screenshot out.png [--page single|coop|settings]
    const QStringList arguments = app.arguments();
    const int pageIndex = arguments.indexOf(QStringLiteral("--page"));
    if (pageIndex >= 0 && pageIndex + 1 < arguments.size())
        window.showPage(arguments.at(pageIndex + 1));

    const int shotIndex = arguments.indexOf(QStringLiteral("--screenshot"));
    const QString shotPath = (shotIndex >= 0 && shotIndex + 1 < arguments.size())
        ? arguments.at(shotIndex + 1)
        : QString();

    // 控件截图模式：--annotate 输出目录
    const int annotateIndex = arguments.indexOf(QStringLiteral("--annotate"));
    const QString annotateDir = (annotateIndex >= 0 && annotateIndex + 1 < arguments.size())
        ? arguments.at(annotateIndex + 1)
        : QString();

    window.show();

    if (!annotateDir.isEmpty()) {
        QTimer::singleShot(700, &window, [&window, annotateDir] {
            // 加高窗口，保证滚动区域内的完整卡片也能一次截全
            window.resize(1440, 1080);
            waitMs(250);

            const QStringList pages = {QStringLiteral("single"), QStringLiteral("coop"),
                                       QStringLiteral("settings")};
            for (const QString &page : pages) {
                window.showPage(page);
                waitMs(220);
                const QString pageTitle = page == QStringLiteral("single")
                    ? QStringLiteral("单独模式")
                    : page == QStringLiteral("coop") ? QStringLiteral("合作模式")
                                                     : QStringLiteral("系统设置");
                annotateWidgets(&window, controlClasses(),
                                annotateDir + QLatin1Char('/') + page, pageTitle);
            }

            // 两个弹窗也纳入控件截图
            window.showPage(QStringLiteral("single"));
            waitMs(120);
            scheduleDialogCapture(annotateDir + QStringLiteral("/dialog_confirm"),
                                  dialogClasses(), QStringLiteral("确认弹窗 · 启动加压"));
            Dialogs::confirm(&window, QStringLiteral("启动加压"),
                             QStringLiteral("将按压力流程执行 30 段加压，首段 1.0 Ton / 保压 10:00。"
                                            "请确认人员已离开压机工作区。"),
                             QStringLiteral("确认启动"), DialogTone::Info, AppIcon::Power);

            int seconds = 600;
            scheduleDialogCapture(annotateDir + QStringLiteral("/dialog_timing"),
                                  dialogClasses(), QStringLiteral("计时设置弹窗"));
            Dialogs::editTiming(&window, 600, &seconds);

            QApplication::quit();
        });
    } else if (!shotPath.isEmpty()) {
        QTimer::singleShot(900, &window, [&window, shotPath] {
            window.grab().save(shotPath);
            QApplication::quit();
        });
    }

    return app.exec();
}
