// SPDX-License-Identifier: MIT
#pragma once

#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>

#include "wakatime.h"
#include "wakatimeconfig.h"

Q_DECLARE_LOGGING_CATEGORY(gLogWakaTimePlugin)

namespace KTextEditor {
    class Document;
    class MainWindow;
    class View;
} // namespace KTextEditor

class WakaTimeView;

/** Plugin for initialisation by KTextEditor. */
class WakaTimePlugin : public KTextEditor::Plugin {
public:
    /** Constructor. */
    explicit WakaTimePlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    /** Destructor. */
    virtual ~WakaTimePlugin();
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    QList<WakaTimeView *> m_views;
};

/** The plugin view. */
class WakaTimeView : public QObject, public KXMLGUIClient {
    Q_OBJECT

public:
    /** Constructor. */
    WakaTimeView(KTextEditor::MainWindow *);
    ~WakaTimeView() override;

private Q_SLOTS:
    void slotConfigureWakaTime();
    void slotDocumentModifiedChanged(KTextEditor::Document *);
    void slotDocumentWrittenToDisk(KTextEditor::Document *);
    void viewCreated(KTextEditor::View *);
    void viewDestroyed(QObject *);

private:
    void connectDocumentSignals(KTextEditor::Document *);
    void disconnectDocumentSignals(KTextEditor::Document *);
    void sendAction(KTextEditor::Document *, bool);

private:
    KTextEditor::MainWindow *m_mainWindow;
    WakaTime client;
    WakaTimeConfig config;
    // Initialised in constructor definition.
    QList<KTextEditor::Document *> connectedDocuments;
};
