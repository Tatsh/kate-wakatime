// SPDX-License-Identifier: MIT
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAboutData>
#include <KActionCollection>
#include <KF6/KTextEditor/ktexteditor_version.h>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcess>
#include <QtCore/QTimeZone>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtWidgets/QDialog>

#include "wakatimeplugin.h"

Q_LOGGING_CATEGORY(gLogWakaTimePlugin, "wakatime-plugin")

K_PLUGIN_FACTORY_WITH_JSON(WakaTimePluginFactory,
                           "ktexteditor_wakatime.json",
                           registerPlugin<WakaTimePlugin>();)

WakaTimePlugin::WakaTimePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent) {
    Q_UNUSED(args);
}

WakaTimePlugin::~WakaTimePlugin() {
}

void WakaTimeView::viewCreated(KTextEditor::View *view) {
    connectDocumentSignals(view->document());
}

void WakaTimeView::viewDestroyed(QObject *view) {
    disconnectDocumentSignals(static_cast<KTextEditor::View *>(view)->document());
}

WakaTimeView::WakaTimeView(KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow) {
    KXMLGUIClient::setComponentName(QStringLiteral("katewakatime"), i18n("WakaTime"));
    setXMLFile(QStringLiteral("ui.rc"));
    auto a = actionCollection()->addAction(QStringLiteral("configure_wakatime"));
    a->setText(i18n("Configure WakaTime..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("wakatime")));
    connect(a, &QAction::triggered, this, &WakaTimeView::slotConfigureWakaTime);
    mainWindow->guiFactory()->addClient(this);
    // Connections
    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, &WakaTimeView::viewCreated);
    for (auto view : m_mainWindow->views()) {
        connectDocumentSignals(view->document());
    }
}

WakaTimeView::~WakaTimeView() {
    m_mainWindow->guiFactory()->removeClient(this);
}

QObject *WakaTimePlugin::createView(KTextEditor::MainWindow *mainWindow) {
    return new WakaTimeView(mainWindow);
}

void WakaTimeView::slotConfigureWakaTime() {
    QDialog dialog(m_mainWindow->window());
    Ui::ConfigureWakaTimeDialog ui;
    ui.setupUi(&dialog);
    auto apiKey = config.apiKey();
    auto apiUrl = config.apiUrl();
    const auto hideFilenames = config.hideFilenames();
    ui.lineEdit_apiKey->setText(apiKey);
    if (apiKey.isNull() || !apiKey.isEmpty()) {
        ui.lineEdit_apiKey->setFocus();
    }
    ui.lineEdit_apiUrl->setText(apiUrl);
    ui.checkBox_hideFilenames->setChecked(hideFilenames);
    dialog.setWindowTitle(i18n("Configure WakaTime"));
    if (dialog.exec() == QDialog::Accepted) {
        auto newApiKey = ui.lineEdit_apiKey->text();
        if (newApiKey.size() >= 36 && newApiKey.size() <= 41) {
            config.setApiKey(newApiKey);
        }
        auto newApiUrl = ui.lineEdit_apiUrl->text();
        if (!newApiUrl.isEmpty() && (newApiUrl.startsWith(QStringLiteral("http://")) ||
                                     newApiUrl.startsWith(QStringLiteral("https://")))) {
            config.setApiUrl(newApiUrl);
        }
        config.setHideFilenames(ui.checkBox_hideFilenames->isChecked());
        config.save();
    }
}

void WakaTimeView::sendAction(KTextEditor::Document *doc, bool isWrite) {
    for (auto view : m_mainWindow->views()) {
        if (view->document() == doc) {
            client.send(doc->url().toLocalFile(),
                        doc->mode(),
                        view->cursorPosition().line() + 1,
                        view->cursorPosition().column() + 1,
                        view->document()->lines(),
                        isWrite);
            break;
        }
    }
}

void WakaTimeView::connectDocumentSignals(KTextEditor::Document *document) {
    if (connectedDocuments.contains(document)) {
        return;
    }
    // When document goes from saved state to changed state (not yet saved on disk).
    connect(document,
            &KTextEditor::Document::modifiedChanged,
            this,
            &WakaTimeView::slotDocumentModifiedChanged);
    // Written to disk.
    connect(document,
            &KTextEditor::Document::documentSavedOrUploaded,
            this,
            &WakaTimeView::slotDocumentWrittenToDisk);
    // Text changes (might be heavy).
    // This event unfortunately is emitted twice in separate threads for every key stroke (maybe key
    // up and down is the reason).
    connect(document,
            &KTextEditor::Document::textChanged,
            this,
            &WakaTimeView::slotDocumentModifiedChanged);
    connectedDocuments << document;
}

void WakaTimeView::disconnectDocumentSignals(KTextEditor::Document *document) {
    if (!connectedDocuments.contains(document)) {
        return;
    }
    disconnect(document, &KTextEditor::Document::modifiedChanged, this, nullptr);
    disconnect(document, &KTextEditor::Document::documentSavedOrUploaded, this, nullptr);
    disconnect(document, &KTextEditor::Document::textChanged, this, nullptr);
    connectedDocuments.removeOne(document);
}

// Slots
void WakaTimeView::slotDocumentModifiedChanged(KTextEditor::Document *doc) {
    sendAction(doc, false);
}

void WakaTimeView::slotDocumentWrittenToDisk(KTextEditor::Document *doc) {
    sendAction(doc, true);
}

#include "wakatimeplugin.moc"
