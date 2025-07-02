#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QtCore/QObject>
#include <QtTest/QTest>

#include "wakatimeplugin.h"

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
    KTextEditor::Editor *editor = KTextEditor::Editor::instance();
    QVERIFY(editor != nullptr);
}

QTEST_MAIN(WakaTimePluginTest)

#include "basic.moc"
