#include "widgets/stagetable.h"
#include "core/format.h"
#include "theme/theme.h"

#include <QDoubleValidator>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QtMath>

namespace {
constexpr int ValueRole = Qt::UserRole + 1;
constexpr int kRowHeight = 39;
constexpr int kHeaderHeight = 36;
} // namespace

class StageHeaderView : public QHeaderView
{
public:
    explicit StageHeaderView(StageTable *table)
        : QHeaderView(Qt::Horizontal, table), m_table(table)
    {
        setFixedHeight(kHeaderHeight);
        setSectionsClickable(false);
        setHighlightSections(false);
        setDefaultAlignment(Qt::AlignCenter);
    }

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override
    {
        painter->save();
        painter->fillRect(rect, Theme::Brand050);
        painter->setPen(Theme::Brand100);
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());

        QString title = QStringLiteral("阶段");
        QString unit;
        if (logicalIndex > 0) {
            if (const StageColumn *spec = m_table->columnSpec(logicalIndex)) {
                title = spec->title;
                unit = spec->unit;
            }
        }

        const QRect content = rect.adjusted(4, 0, -4, 0);
        painter->setPen(QColor(0x0B, 0x6E, 0xA8));
        painter->setFont(Theme::ui(11, QFont::Bold));

        if (unit.isEmpty()) {
            painter->drawText(content, Qt::AlignCenter, title);
        } else {
            painter->drawText(content.adjusted(0, 4, 0, -12), Qt::AlignCenter, title);
            painter->setPen(QColor(0x6F, 0xA7, 0xC6));
            painter->setFont(Theme::ui(9));
            painter->drawText(content.adjusted(0, 18, 0, 0), Qt::AlignCenter, unit);
        }
        painter->restore();
    }

private:
    StageTable *m_table;
};

class StageDelegate : public QStyledItemDelegate
{
public:
    explicit StageDelegate(StageTable *table)
        : QStyledItemDelegate(table), m_table(table) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const QRect r = option.rect;
        const bool current = (index.row() + 1 == m_table->m_currentStage);

        painter->fillRect(r, current ? Theme::NowRow : QColor(Qt::white));
        if (current) {
            // 当前阶段只在首列左侧画 4px 指示条，
            // 否则每个单元格都会在列边界各画一条，形成列间竖线
            if (index.column() == 0)
                painter->fillRect(QRect(r.left(), r.top(), 4, r.height()), Theme::Brand);
            painter->setPen(QColor(0, 161, 233, 56));
            painter->drawLine(r.topLeft(), r.topRight());
            painter->drawLine(r.bottomLeft(), r.bottomRight());
        } else {
            painter->setPen(Theme::Line2);
            painter->drawLine(r.bottomLeft(), r.bottomRight());
        }

        if (index.column() == 0) {
            const QRect badge(r.center().x() - 12, r.center().y() - 12, 24, 24);
            painter->setPen(Qt::NoPen);
            painter->setBrush(current ? Theme::Brand : QColor(0xF1, 0xF5, 0xF9));
            painter->drawRoundedRect(badge, 7, 7);
            painter->setPen(current ? QColor(Qt::white) : Theme::Ink2);
            painter->setFont(Theme::num(13, QFont::DemiBold));
            painter->drawText(badge, Qt::AlignCenter, QString::number(index.row() + 1));
            painter->restore();
            return;
        }

        const StageColumn *spec = m_table->columnSpec(index.column());
        if (!spec) {
            painter->restore();
            return;
        }

        const QVariant value = index.data(ValueRole);
        if (spec->type == StageColumnType::Flag) {
            const bool on = value.toBool();
            const QRect badge(r.center().x() - 12, r.center().y() - 12, 24, 24);
            painter->setPen(Qt::NoPen);
            painter->setBrush(on ? Theme::Green050 : QColor(0xF1, 0xF5, 0xF9));
            painter->drawRoundedRect(badge, 7, 7);
            painter->setPen(on ? Theme::Green600 : Theme::Muted2);
            painter->setFont(Theme::num(12, QFont::Bold));
            painter->drawText(badge, Qt::AlignCenter, on ? QStringLiteral("Y")
                                                         : QStringLiteral("—"));
        } else if (spec->type == StageColumnType::Time) {
            painter->setPen(current ? Theme::Brand700 : Theme::Muted);
            painter->setFont(Theme::num(12));
            painter->drawText(r, Qt::AlignCenter, fmtTime(value.toInt()));
        } else {
            painter->setPen(current ? Theme::Brand700 : Theme::Ink);
            painter->setFont(Theme::num(15, QFont::DemiBold));
            painter->drawText(r, Qt::AlignCenter,
                              QString::number(value.toDouble(), 'f', spec->decimals));
        }

        painter->restore();
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        const StageColumn *spec = m_table->columnSpec(index.column());
        if (!spec || spec->type != StageColumnType::Number)
            return nullptr;

        auto *editor = new QLineEdit(parent);
        auto *validator = new QDoubleValidator(spec->minimum, spec->maximum,
                                               spec->decimals, editor);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setLocale(QLocale::c());
        editor->setValidator(validator);
        editor->setAlignment(Qt::AlignCenter);
        editor->setFont(Theme::num(14, QFont::DemiBold));
        editor->setFrame(false);
        editor->setStyleSheet(QStringLiteral(
            "QLineEdit { background:#FFFFFF; border:1.5px solid #00A1E9;"
            "border-radius:6px; color:#12293D; }"));
        editor->selectAll();
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        auto *line = qobject_cast<QLineEdit *>(editor);
        const StageColumn *spec = m_table->columnSpec(index.column());
        if (!line || !spec)
            return;
        line->setText(QString::number(index.data(ValueRole).toDouble(), 'f', spec->decimals));
        line->selectAll();
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        auto *line = qobject_cast<QLineEdit *>(editor);
        const StageColumn *spec = m_table->columnSpec(index.column());
        if (!line || !spec)
            return;

        bool ok = false;
        double value = line->text().toDouble(&ok);
        if (!ok)
            value = index.data(ValueRole).toDouble();
        value = qBound(spec->minimum, value, spec->maximum);
        const double factor = qPow(10.0, spec->decimals);
        value = qRound(value * factor) / factor;

        model->setData(index, value, ValueRole);
        emit m_table->numberEdited(index.row(), index.column(), value);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        editor->setGeometry(option.rect.adjusted(5, 4, -5, -4));
    }

private:
    StageTable *m_table;
};

StageTable::StageTable(QWidget *parent)
    : QTableWidget(parent)
{
    setObjectName(QStringLiteral("stageTable"));
    setFrameShape(QFrame::NoFrame);
    setShowGrid(false);
    setWordWrap(false);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    setFocusPolicy(Qt::StrongFocus);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(kRowHeight);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    setHorizontalHeader(new StageHeaderView(this));
    horizontalHeader()->setVisible(true);
    horizontalHeader()->setFixedHeight(kHeaderHeight);

    setItemDelegate(new StageDelegate(this));

    connect(this, &QTableWidget::cellClicked, this, [this](int row, int column) {
        const StageColumn *spec = columnSpec(column);
        if (!spec)
            return;
        if (spec->type == StageColumnType::Time) {
            emit timeClicked(row, column);
        } else if (spec->type == StageColumnType::Flag) {
            setFlag(row, column, !flag(row, column));
            emit flagToggled(row, column);
        }
    });
}

void StageTable::setColumns(const QVector<StageColumn> &columns, int indexWidth)
{
    m_columns = columns;
    m_indexWidth = indexWidth;
    setColumnCount(columns.size() + 1);

    QStringList labels;
    labels << QStringLiteral("阶段");
    for (const auto &spec : columns)
        labels << spec.title;
    setHorizontalHeaderLabels(labels);

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    horizontalHeader()->resizeSection(0, indexWidth);
    for (int i = 1; i <= columns.size(); ++i)
        horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);

    setStageRowCount(rowCount());
}

void StageTable::setStageRowCount(int rows)
{
    rows = qMax(0, rows);
    QTableWidget::setRowCount(rows);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columnCount(); ++c) {
            if (!item(r, c)) {
                auto *cell = new QTableWidgetItem;
                setItem(r, c, cell);
            }
            const StageColumn *spec = columnSpec(c);
            const bool editable = spec && spec->type == StageColumnType::Number;
            item(r, c)->setFlags(editable
                ? (Qt::ItemIsEnabled | Qt::ItemIsEditable)
                : Qt::ItemIsEnabled);
        }
    }
    viewport()->update();
}

void StageTable::setNumber(int row, int column, double value)
{
    QTableWidgetItem *cell = item(row, column);
    if (!cell)
        return;
    cell->setData(ValueRole, value);
    viewport()->update();
}

void StageTable::setTime(int row, int column, int seconds)
{
    QTableWidgetItem *cell = item(row, column);
    if (!cell)
        return;
    cell->setData(ValueRole, seconds);
    viewport()->update();
}

void StageTable::setFlag(int row, int column, bool value)
{
    QTableWidgetItem *cell = item(row, column);
    if (!cell)
        return;
    cell->setData(ValueRole, value);
    viewport()->update();
}

double StageTable::number(int row, int column) const
{
    QTableWidgetItem *cell = item(row, column);
    return cell ? cell->data(ValueRole).toDouble() : 0.0;
}

int StageTable::time(int row, int column) const
{
    QTableWidgetItem *cell = item(row, column);
    return cell ? cell->data(ValueRole).toInt() : 0;
}

bool StageTable::flag(int row, int column) const
{
    QTableWidgetItem *cell = item(row, column);
    return cell ? cell->data(ValueRole).toBool() : false;
}

void StageTable::setCurrentStage(int stage)
{
    if (m_currentStage == stage)
        return;
    m_currentStage = stage;
    viewport()->update();
    if (stage > 0 && stage <= rowCount()) {
        QTableWidgetItem *cell = item(stage - 1, 0);
        if (cell)
            scrollToItem(cell, QAbstractItemView::EnsureVisible);
    }
}

void StageTable::setColumnVisible(int column, bool visible)
{
    setColumnHidden(column, !visible);
}

const StageColumn *StageTable::columnSpec(int column) const
{
    if (column <= 0 || column > m_columns.size())
        return nullptr;
    return &m_columns.at(column - 1);
}
