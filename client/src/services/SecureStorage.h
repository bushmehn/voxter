#pragma once

#include <QObject>

class SecureStorage final : public QObject {
    Q_OBJECT

public:
    explicit SecureStorage(QObject *parent = nullptr);

    void saveAuthTokens(const QString &accessToken, const QString &refreshToken);
    QString loadAccessToken() const;
    QString loadRefreshToken() const;
    void clear();

private:
    QByteArray obfuscate(const QByteArray &input) const;
    QByteArray deobfuscate(const QByteArray &input) const;
    QByteArray key() const;
};
