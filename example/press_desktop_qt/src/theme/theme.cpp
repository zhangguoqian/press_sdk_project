#include "theme/theme.h"

#include <QFontInfo>

namespace Theme {

QFont ui(int pixelSize, int weight)
{
    QFont f(QStringLiteral("Microsoft YaHei UI"));
    if (!QFontInfo(f).exactMatch())
        f.setFamily(QStringLiteral("Microsoft YaHei"));
    f.setPixelSize(pixelSize);
    f.setWeight(static_cast<QFont::Weight>(weight));
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QFont num(int pixelSize, int weight)
{
    QFont f(QStringLiteral("Bahnschrift"));
    if (!QFontInfo(f).exactMatch())
        f.setFamily(QStringLiteral("Segoe UI"));
    f.setPixelSize(pixelSize);
    f.setWeight(static_cast<QFont::Weight>(weight));
    f.setStyleStrategy(QFont::PreferAntialias);
    return f;
}

QString styleSheet()
{
    return QStringLiteral(R"(
/* ============ 全局 ============ */
* {
    font-family: "Microsoft YaHei UI", "Microsoft YaHei";
    font-size: 13px;
    color: #12293D;
    outline: none;
}
QWidget#root, QWidget#contentArea, QWidget#page, QStackedWidget#stack {
    background: #EDF1F5;
}
QScrollArea#pageScroll, QScrollArea#pageScroll > QWidget > QWidget {
    background: transparent;
    border: none;
}
QToolTip {
    background: #10283E;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 6px 9px;
    font-size: 11px;
}

/* ============ 滚动条 ============ */
QScrollBar:vertical {
    width: 10px;
    background: transparent;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #C8D5DF;
    border-radius: 4px;
    min-height: 32px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover { background: #A9BCCB; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    height: 0;
    background: none;
    border: none;
}
QScrollBar:horizontal {
    height: 10px;
    background: transparent;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: #C8D5DF;
    border-radius: 4px;
    min-width: 32px;
    margin: 2px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    width: 0;
    background: none;
    border: none;
}

/* ============ 标题栏 ============ */
QWidget#titleBar {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #061A27, stop:0.55 #0B2C40, stop:1 #0E344A);
}
QLabel#titleText {
    color: #D5E7F2;
    font-size: 12px;
}
QLabel#versionChip {
    color: #6D92A9;
    font-size: 10px;
    border: 1px solid rgba(255, 255, 255, 0.13);
    border-radius: 4px;
    padding: 0 5px;
}

/* ============ 左侧导航 ============ */
QWidget#sidebar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0A2334, stop:0.45 #0B2739, stop:1 #08202F);
}
QLabel#brandName {
    color: #EAF4FA;
    font-size: 13px;
    font-weight: 700;
}
QLabel#brandSub {
    color: #5B7F97;
    font-size: 8px;
}
QLabel#railGroupLabel {
    color: #51748C;
    font-size: 10px;
    font-weight: 600;
}
QLabel#railStatText {
    color: #8FADC0;
    font-size: 11px;
}
QLabel#railStatSub {
    color: #4F7089;
    font-size: 10px;
}

/* ============ 卡片 ============ */
QFrame#card {
    background: #FFFFFF;
    border: 1px solid #E3EAF0;
    border-radius: 14px;
}
QWidget#cardHeader {
    background: transparent;
}
QWidget#cardHeaderBordered {
    background: transparent;
    border-bottom: 1px solid #EFF4F8;
}
QLabel#cardTitle {
    color: #12293D;
    font-size: 13px;
    font-weight: 700;
}

/* ============ 键值行 ============ */
QFrame#kvRow {
    background: transparent;
    border-bottom: 1px dashed #EFF4F8;
}
QFrame#kvRow[last="true"] {
    border-bottom: none;
}
QLabel#kvLabel {
    color: #3B5770;
    font-size: 12px;
}
QLabel#kvUnit {
    color: #7C90A2;
    font-size: 10px;
}
QLabel#kvValue {
    color: #12293D;
    font-size: 23px;
    font-weight: 600;
}

/* ============ 分组标题 / 提示 ============ */
QLabel#subLabel {
    color: #5F7488;
    font-size: 11px;
    font-weight: 600;
}
QLabel#tipLabel {
    color: #7C90A2;
    font-size: 11px;
}

/* ============ 分段控件 ============ */
QWidget#seg {
    background: #F1F5F9;
    border: 1px solid #E3EAF0;
    border-radius: 9px;
}
QPushButton#segBtn {
    border: none;
    border-radius: 7px;
    background: transparent;
    color: #3B5770;
    font-size: 12px;
    padding: 0 12px;
}
QPushButton#segBtn:hover { color: #0079B4; }
QPushButton#segBtn:checked {
    background: #FFFFFF;
    color: #0079B4;
    font-weight: 600;
}

/* ============ 数值输入 ============ */
QFrame#unitInput {
    background: #FBFDFE;
    border: 1px solid #E3EAF0;
    border-radius: 8px;
}
QFrame#unitInput[focused="true"] {
    border: 1px solid #00A1E9;
}
QLineEdit#plainEdit {
    border: none;
    background: transparent;
    padding: 0 10px;
    color: #12293D;
}
QLabel#unitSuffix {
    color: #7C90A2;
    font-size: 11px;
    border-left: 1px solid #EFF4F8;
    padding: 0 10px;
}

/* ============ 模具受力面积 ============ */
QFrame#moldArea {
    background: #F7FBFE;
    border: 1px dashed #A8DDF5;
    border-radius: 8px;
}
QLabel#moldAreaLabel {
    color: #3B5770;
    font-size: 11px;
}
QLabel#moldAreaValue {
    color: #0079B4;
    font-size: 15px;
    font-weight: 700;
}

/* ============ 阶段工具 ============ */
QLabel#countLabel {
    color: #5F7488;
    font-size: 11px;
}
QFrame#spin {
    background: #FFFFFF;
    border: 1px solid #E3EAF0;
    border-radius: 6px;
}
QPushButton#spinBtn {
    border: none;
    background: transparent;
    color: #3B5770;
    font-size: 13px;
}
QPushButton#spinBtn:hover {
    background: #E9F7FE;
    color: #0079B4;
}
QLineEdit#spinEdit {
    border: none;
    border-left: 1px solid #EFF4F8;
    border-right: 1px solid #EFF4F8;
    background: transparent;
    color: #12293D;
    font-size: 12px;
}
QLineEdit#spinEdit:focus { background: #E9F7FE; }
QFrame#stagePill {
    background: #E9F7FE;
    border: 1px solid #D2EEFB;
    border-radius: 12px;
}
QLabel#stagePillText {
    color: #0079B4;
    font-size: 11px;
}

/* ============ 表格 ============ */
QTableWidget#stageTable {
    background: #FFFFFF;
    border: none;
    gridline-color: transparent;
}
QTableWidget#stageTable::item { border: none; }

/* ============ 操作栏 ============ */
QWidget#actionBar {
    background: #FFFFFF;
    border-top: 1px solid #E3EAF0;
}
QLabel#actionNote {
    color: #5F7488;
    font-size: 12px;
}
QPushButton#btnGreen {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0AB233, stop:1 #008C24);
    border: none;
    border-radius: 10px;
    color: #FFFFFF;
    font-size: 14px;
    font-weight: 600;
    padding: 0 22px;
}
QPushButton#btnGreen:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #16BF3F, stop:1 #009A29);
}
QPushButton#btnGreen[running="true"] {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #DC1B33, stop:1 #A50D25);
}
QPushButton#btnGreen[running="true"]:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #E8374C, stop:1 #B0142D);
}
QPushButton#btnDangerGhost {
    background: #FFFFFF;
    border: 1px solid rgba(200, 16, 46, 90);
    border-radius: 10px;
    color: #A50D25;
    font-size: 14px;
    font-weight: 600;
    padding: 0 22px;
}
QPushButton#btnDangerGhost:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #DC1B33, stop:1 #A50D25);
    border: 1px solid #A50D25;
    color: #FFFFFF;
}
QPushButton#btnBrand {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #18ADF0, stop:1 #0090D2);
    border: none;
    border-radius: 10px;
    color: #FFFFFF;
    font-weight: 600;
}
QPushButton#btnAmber {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F58220, stop:1 #C75F05);
    border: none;
    border-radius: 10px;
    color: #FFFFFF;
    font-weight: 600;
}
QPushButton#btnRed {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #DC1B33, stop:1 #A50D25);
    border: none;
    border-radius: 10px;
    color: #FFFFFF;
    font-weight: 600;
}
QPushButton#btnGhost {
    background: #FFFFFF;
    border: 1px solid #E3EAF0;
    border-radius: 10px;
    color: #3B5770;
    font-weight: 500;
}
QPushButton#btnGhost:hover {
    background: #E9F7FE;
    border: 1px solid #A8DDF5;
    color: #0079B4;
}

/* ============ 弹窗 ============ */
QFrame#modalCard {
    background: #FFFFFF;
    border-radius: 16px;
}
QFrame#modalIcon {
    background: #E9F7FE;
    border-radius: 9px;
}
QFrame#modalIcon[tone="warn"] { background: #FFF3E6; }
QFrame#modalIcon[tone="bad"]  { background: #FDECEE; }
QLabel#modalTitle {
    color: #12293D;
    font-size: 15px;
    font-weight: 700;
}
QLabel#modalBody {
    color: #3B5770;
    font-size: 13px;
}
QLabel#timePreview {
    color: #0079B4;
    font-size: 42px;
    font-weight: 700;
}
QLabel#tpUnit {
    color: #5F7488;
    font-size: 11px;
}
QPushButton#tpStep {
    background: #F2F7FB;
    border: 1px solid #E3EAF0;
    border-radius: 9px;
    color: #0079B4;
    font-size: 17px;
    font-weight: 700;
}
QPushButton#tpStep:hover {
    background: #E4F2FA;
    border: 1px solid #A8DDF5;
}
QLineEdit#tpNum {
    background: #FFFFFF;
    border: 1px solid #E3EAF0;
    border-radius: 12px;
    color: #12293D;
    font-size: 30px;
    font-weight: 700;
}
QLineEdit#tpNum:focus {
    border: 1px solid #00A1E9;
}
QPushButton#tpPreset {
    background: #FFFFFF;
    border: 1px solid #E3EAF0;
    border-radius: 14px;
    color: #3B5770;
    font-size: 12px;
    padding: 0 13px;
}
QPushButton#tpPreset:hover {
    background: #E9F7FE;
    border: 1px solid #A8DDF5;
    color: #0079B4;
}

/* ============ 关于本机 ============ */
QFrame#aboutList {
    background: #FFFFFF;
    border: 1px solid #E3EAF0;
    border-radius: 12px;
}
QFrame#aboutRow {
    border-bottom: 1px solid #EFF4F8;
}
QFrame#aboutRow[last="true"] { border-bottom: none; }
QLabel#aboutKey {
    color: #5F7488;
    font-size: 12px;
}
QLabel#aboutVal {
    color: #12293D;
    font-size: 12px;
    font-weight: 600;
}
QLabel#heroTitle {
    color: #12293D;
    font-size: 19px;
    font-weight: 700;
}
QLabel#heroSub {
    color: #5F7488;
    font-size: 12px;
}
QLabel#heroMetaKey {
    color: #5F7488;
    font-size: 11px;
}
QLabel#heroMetaVal {
    color: #12293D;
    font-size: 13px;
    font-weight: 700;
}
QFrame#heroMeta {
    border-top: 1px solid #E3EAF0;
}
QLabel#chipBrand {
    background: #E9F7FE;
    border: 1px solid #D2EEFB;
    border-radius: 12px;
    color: #0079B4;
    font-size: 11px;
    padding: 4px 10px;
}
)");
}

} // namespace Theme
