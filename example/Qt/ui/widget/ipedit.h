//
// Created by 11518 on 2026/10/7.
//

#ifndef PRESS_SDK_PROJECT_IPEDIT_H
#define PRESS_SDK_PROJECT_IPEDIT_H

#include <QLineEdit>
#include <QValidator>

// 仅允许合法的 IPv4 地址输入，正确处理中间态（如 "192."、"192.168."）。
class IpValidator : public QValidator
{
    Q_OBJECT
public:
    explicit IpValidator(QObject *parent = nullptr);

    State validate(QString &input, int &pos) const override;
};

class IpEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit IpEdit(QWidget *parent = nullptr);
    ~IpEdit() override;

    // 设置 / 获取 IPv4 地址文本。传入非法格式时 setIp 不生效。
    bool setIp(const QString &ip);
    QString ip() const;

    // 当前内容是否为完整合法的 IPv4 地址（0.0.0.0 - 255.255.255.255）。
    bool isValid() const;
};

#endif //PRESS_SDK_PROJECT_IPEDIT_H
