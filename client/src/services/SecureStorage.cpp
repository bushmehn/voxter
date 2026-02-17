#include "SecureStorage.h"

#include <QCryptographicHash>
#include <QSettings>
#include <QSysInfo>

namespace {
const char *kAccessTokenKey = "auth/accessToken";
const char *kRefreshTokenKey = "auth/refreshToken";
}

SecureStorage::SecureStorage(QObject *parent)
    : QObject(parent) {}

void SecureStorage::saveAuthTokens(const QString &accessToken, const QString &refreshToken) {
    QSettings settings("Voxter", "VoxterDesktop");
    settings.setValue(kAccessTokenKey, obfuscate(accessToken.toUtf8()).toBase64());
    settings.setValue(kRefreshTokenKey, obfuscate(refreshToken.toUtf8()).toBase64());
    settings.sync();
}

QString SecureStorage::loadAccessToken() const {
    QSettings settings("Voxter", "VoxterDesktop");
    const QByteArray encoded = settings.value(kAccessTokenKey).toByteArray();
    return QString::fromUtf8(deobfuscate(QByteArray::fromBase64(encoded)));
}

QString SecureStorage::loadRefreshToken() const {
    QSettings settings("Voxter", "VoxterDesktop");
    const QByteArray encoded = settings.value(kRefreshTokenKey).toByteArray();
    return QString::fromUtf8(deobfuscate(QByteArray::fromBase64(encoded)));
}

void SecureStorage::clear() {
    QSettings settings("Voxter", "VoxterDesktop");
    settings.remove(kAccessTokenKey);
    settings.remove(kRefreshTokenKey);
    settings.sync();
}

QByteArray SecureStorage::obfuscate(const QByteArray &input) const {
    const QByteArray secret = key();
    QByteArray output = input;
    for (int i = 0; i < output.size(); ++i) {
        output[i] = output.at(i) ^ secret.at(i % secret.size());
    }
    return output;
}

QByteArray SecureStorage::deobfuscate(const QByteArray &input) const {
    return obfuscate(input);
}

QByteArray SecureStorage::key() const {
    const QByteArray source =
        (QSysInfo::machineUniqueId() + QByteArrayLiteral("voxter-desktop-key")).trimmed();
    return QCryptographicHash::hash(source, QCryptographicHash::Sha256);
}
