#include <QtTest/QtTest>
#include "store/AppStore.h"

class AppStoreTests final : public QObject {
    Q_OBJECT

private slots:
    void updatesAuthenticationState();
    void upsertsMessages();
};

void AppStoreTests::updatesAuthenticationState() {
    AppStore store;
    QVERIFY(!store.authenticated());

    store.setAuthenticated(true);
    QVERIFY(store.authenticated());
}

void AppStoreTests::upsertsMessages() {
    AppStore store;

    QVariantMap message;
    message.insert("id", "m1");
    message.insert("content", "hello");

    store.upsertMessage(message);
    QCOMPARE(store.messages().size(), 1);

    QVariantMap updatedMessage;
    updatedMessage.insert("id", "m1");
    updatedMessage.insert("content", "updated");

    store.upsertMessage(updatedMessage);
    QCOMPARE(store.messages().size(), 1);
    QCOMPARE(store.messages().first().toMap().value("content").toString(), QString("updated"));
}

QTEST_MAIN(AppStoreTests)
#include "tst_appstore.moc"
