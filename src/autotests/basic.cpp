#include <QtCore/QObject>
#include <QtTest/QTest>

class WakaTimePluginTest : public QObject {
    Q_OBJECT

public:
    WakaTimePluginTest(QObject *parent = nullptr);
    ~WakaTimePluginTest() override;

private Q_SLOTS:
    void test_PluginInitialization();
};

WakaTimePluginTest::WakaTimePluginTest(QObject *parent) : QObject(parent) {
}

WakaTimePluginTest::~WakaTimePluginTest() {
}

void WakaTimePluginTest::test_PluginInitialization() {
}

QTEST_MAIN(WakaTimePluginTest)

#include "basic.moc"
