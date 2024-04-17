
#include "MainWindow.hpp"

#include "utils/connectutils.hpp"
#include "utils/IconLocator.hpp"
#include "utils/utils.hpp"
#include "widgets/TableView.hpp"
#include "version.hpp"

#include <QApplication>
#include <QFileInfo>
#include <QSettings>
#include <QShortcut>
#include <QScreen>
#include <QMenu>
#include <QStatusBar>
#include <QtDebug>
#include <QUndoView>
#include <QSplitter>
#include <QTimerEvent>

#define TU MainWindowTU

namespace TU {

static auto const KEY_MAIN_WINDOW = QStringLiteral("MainWindow");
static auto const KEY_WINDOW_STATE = QStringLiteral("windowState");
static auto const KEY_GEOMETRY = QStringLiteral("geometry");
static auto const KEY_HSPLITTER = QStringLiteral("hsplitter");
static auto const KEY_VSPLITTER = QStringLiteral("vsplitter");


static auto const KEY_RECENT_FILES = QStringLiteral("recentFiles");

static auto const DEFAULT_HSPLITTER_RATIO = 0.625f;
static auto const DEFAULT_VSPLITTER_RATIO = 0.625f;

//
// increment this constant when adding new docks or toolbars
// v2 - removed all dock widgets (no longer using QDockWidget)
// v1 - initial version
//
static constexpr int WINDOW_STATE_VERSION = 2;

}

MainWindow::MainWindow() :
    QMainWindow(),
    mAudioEnumerator(),
    mUntitledString(tr("Untitled")),
    mPianoInput(),
    mMidi(),
    mModule(),
    mModuleFile(),
    mErrorSinceLastConfig(false),
    mLastEngineFrame(),
    mFrameSkip(0),
    mAutosave(false),
    mAutosaveIntervalMs(30000),
    mAudioDiag(nullptr),
    mTempoCalc(nullptr),
    mCommentsDialog(nullptr),
    mInstrumentEditor(nullptr),
    mWaveEditor(nullptr),
    mHistoryDialog(nullptr),
    mEffectsListDialog(nullptr)
{

    // create models
    mModule = new Module(this);
    mInstrumentModel = new InstrumentListModel(*mModule, this);
    mSongListModel = new SongListModel(*mModule, this);
    mSongModel = new SongModel(*mModule, this);
    mPatternModel = new PatternModel(*mModule, *mSongModel, this);
    mWaveModel = new WaveListModel(*mModule, this);

    mRenderer = new Renderer(*mModule, this);

    setupUi();

    // read in application configuration
    //mConfig.readSettings(mAudioEnumerator, mMidiEnumerator);
    
    setWindowIcon(IconLocator::getAppIcon());

    QSettings settings;
    settings.beginGroup(TU::KEY_MAIN_WINDOW);

    // restore geomtry from the last session
    if (!restoreGeometry(settings.value(TU::KEY_GEOMETRY, QByteArray()).toByteArray())) {
        // no saved geometry, initialize it
        // we will take 3/4 of the primary screen's width and height, but we
        // will take no more than 1280x720

        // maximum initial resolution
        constexpr int MAX_WIDTH = 1280;
        constexpr int MAX_HEIGHT = 720;

        // get the available geometry for the primary screen
        auto const availableGeometry = QApplication::primaryScreen()->availableGeometry();

        QRect newGeometry(
            0,
            0, 
            std::min(MAX_WIDTH, availableGeometry.width() * 3 / 4),
            std::min(MAX_HEIGHT, availableGeometry.height() * 3 / 4)
        );

        newGeometry.moveTo(availableGeometry.center() - newGeometry.center());
        setGeometry(newGeometry);
    }

    // restore window state if it exists
    auto const windowState = settings.value(TU::KEY_WINDOW_STATE).toByteArray();
    if (windowState.isEmpty()) {
        // default layout
        initState();
    } else {
        addToolBar(mToolbarFile);
        addToolBar(mToolbarEdit);
        addToolBar(mToolbarSong);
        addToolBar(mToolbarTracker);
        addToolBar(mToolbarInput);
        if (!restoreState(windowState, TU::WINDOW_STATE_VERSION)) {
            initState();
        }
    }

    // restore splitter sizes
    auto restoreSplitter = [](QSplitter &splitter, QByteArray const& state, float defaultRatio) {
        if (!splitter.restoreState(state)) {
            rationSplitter(splitter, defaultRatio);
        }
    };
    restoreSplitter(*mVSplitter, settings.value(TU::KEY_VSPLITTER).toByteArray(), TU::DEFAULT_VSPLITTER_RATIO);
    restoreSplitter(*mHSplitter, settings.value(TU::KEY_HSPLITTER).toByteArray(), TU::DEFAULT_HSPLITTER_RATIO);

    mModuleFile.setName(mUntitledString);
    updateWindowTitle();

    // apply the read in configuration
    Config config;
    config.readSettings(mAudioEnumerator, mMidiEnumerator);
    applyConfig(config, Config::CategoryAll);
    config.writeSettings(mAudioEnumerator, mMidiEnumerator);

    setAcceptDrops(true);

    setStyleSheet(QStringLiteral(R"stylesheet(
QToolBar QLabel {
    padding-left: 3px;
    padding-right: 3px;
}
    )stylesheet"));
}

QMenu* MainWindow::createPopupMenu() {
    auto menu = new QMenu(this);
    setupViewMenu(menu);
    return menu;
}

void MainWindow::panic(QString const& msg) {
    // this function is called from our custom message handler for fatal
    // messages. Shows the fatal error to the user and attempts to save the
    // current module if it was modified.

    QString moduleSaveResult;
    if (mModule->isModified()) {
        auto path = mModuleFile.crashSave(*mModule);
        if (path.isEmpty()) {
            moduleSaveResult = tr("Unable to save a copy of the module");
        } else {
            moduleSaveResult = tr("A copy of the module has been saved to: %1").arg(path);
        }
    }

    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::Critical);
    msgbox.setText(tr("An unexpected error has occurred."));
    msgbox.setDetailedText(msg);
    if (!moduleSaveResult.isEmpty()) {
        msgbox.setInformativeText(moduleSaveResult);
    }
    msgbox.exec();
}

void MainWindow::closeEvent(QCloseEvent *evt) {
    if (maybeSave()) {
        // user saved or discarded changes, close the window
        #ifdef QT_DEBUG
        if (mSaveConfig) {
        #endif
            QSettings settings;
            settings.beginGroup(TU::KEY_MAIN_WINDOW);
            settings.setValue(TU::KEY_GEOMETRY, saveGeometry());
            settings.setValue(TU::KEY_WINDOW_STATE, saveState(TU::WINDOW_STATE_VERSION));
            // splitters
            settings.setValue(TU::KEY_HSPLITTER, mHSplitter->saveState());
            settings.setValue(TU::KEY_VSPLITTER, mVSplitter->saveState());
        #ifdef QT_DEBUG
        }
        #endif
        evt->accept();
    } else {
        // user aborted closing, ignore this event
        evt->ignore();
    }
    
}

void MainWindow::showEvent(QShowEvent *evt) {
    Q_UNUSED(evt)
    mPatternEditor->setFocus();
}

void MainWindow::timerEvent(QTimerEvent *evt) {
    if (evt->timerId() == mAutosaveTimer.timerId()) {
        if (mModuleFile.hasFile()) {
            qDebug() << "[MainWindow] Auto-saving...";
            onFileSave();
            mAutosaveTimer.stop();
        }
    } else {
        QMainWindow::timerEvent(evt);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* evt) {
    // only one URL, being the location of the module to open, is accepted
    auto const mime = evt->mimeData();
    if (mime->hasUrls() && mime->urls().length() == 1) {
        evt->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* evt) {
    if (maybeSave()) {
        openFile(evt->mimeData()->urls().at(0).toLocalFile());
    }

}



// PRIVATE METHODS -----------------------------------------------------------

bool MainWindow::maybeSave() {
    if (mModule->isModified()) {
        // prompt the user if they want to save any changes
        auto const result = QMessageBox::warning(
            this,
            tr("Trackerboy"),
            tr("Save changes to %1?").arg(mModuleFile.name()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        switch (result) {
            case QMessageBox::Save:
                if (!onFileSave()) {
                    // save failed, do not close document
                    return false;
                }
                break;
            case QMessageBox::Cancel:
                // user cancelled, do not close document
                return false;
            default:
                break;
        }
    }

    return true;
}

QToolBar* MainWindow::makeToolbar(QString const& title, QString const& objname) {
    auto toolbar = new QToolBar(title, this);
    toolbar->setObjectName(objname);
    toolbar->setIconSize(IconLocator::size());
    return toolbar;
}

void MainWindow::setupUi() {

    // CENTRAL WIDGET ========================================================

    auto centralWidget = new QWidget(this);
    auto layout = new QHBoxLayout;
    mSidebar = new Sidebar(*mModule, *mPatternModel, *mSongListModel, *mSongModel);
    mPatternEditor = new PatternEditor(mPianoInput, *mPatternModel);
    layout->addWidget(mSidebar);
    centralWidget->setLayout(layout);

    mHSplitter = new QSplitter(Qt::Horizontal);
    
    mVSplitter = new QSplitter(Qt::Vertical);
    auto setupTableEdit = [](QString const& table, TableView *&dock, BaseTableModel &model, QString const& str) {
        auto group = new QGroupBox(table);
        auto layout = new QVBoxLayout;
        dock = new TableView(model, str);
        layout->addWidget(dock);
        layout->setContentsMargins(0, 0, 0, 0);
        group->setLayout(layout);
        return group;
    };
    mVSplitter->addWidget(
        setupTableEdit(
            tr("Instruments"),
            mInstruments,
            *mInstrumentModel,
            tr("instrument")
        ));
    mVSplitter->addWidget(
        setupTableEdit(
            tr("Waveforms"),
            mWaveforms,
            *mWaveModel,
            tr("waveform")
        ));

    mHSplitter->addWidget(mPatternEditor);
    mHSplitter->addWidget(mVSplitter);
    layout->addWidget(mHSplitter, 1);

    setCentralWidget(centralWidget);
    mMidi.setReceiver(mPatternEditor);

    {
        auto grid = mPatternEditor->grid();
        grid->setFirstHighlight(mSongModel->rowsPerBeat());
        grid->setSecondHighlight(mSongModel->rowsPerMeasure());
        lazyconnect(mSongModel, rowsPerBeatChanged, grid, setFirstHighlight);
        lazyconnect(mSongModel, rowsPerMeasureChanged, grid, setSecondHighlight);
        connect(mSongModel, &SongModel::reloaded, this,
            [this]() {
                mPatternEditor->grid()->setFirstHighlight(mSongModel->rowsPerBeat());
                mPatternEditor->grid()->setSecondHighlight(mSongModel->rowsPerMeasure());
            });
    }
    connect(mInstruments, &TableView::selectedItemChanged, this,
        [this](int index) {
            int id = -1;
            if (index != -1) {
                id = mInstrumentModel->id(index);
                if (mInstrumentEditor && mInstrumentEditor->isVisible()) {
                    mInstrumentEditor->openItem(index);
                }
            }
            mPatternEditor->setInstrument(id);
        });

    connect(mWaveforms, &TableView::selectedItemChanged, this,
        [this](int index) {
            if (index != -1 && mWaveEditor && mWaveEditor->isVisible()) {
                mWaveEditor->openItem(index);
            }
        });

    // TOOLBARS ==============================================================

    mToolbarFile = makeToolbar(tr("File"), QStringLiteral("ToolbarFile"));
    mToolbarEdit = makeToolbar(tr("Edit"), QStringLiteral("ToolbarEdit"));
    mToolbarSong = makeToolbar(tr("Song"), QStringLiteral("ToolbarSong"));
    mToolbarTracker = makeToolbar(tr("Tracker"), QStringLiteral("ToolbarTracker"));
    mToolbarInput = makeToolbar(tr("Input"), QStringLiteral("ToolbarInput"));

    mOctaveSpin = new QSpinBox(mToolbarInput);
    mOctaveSpin->setRange(2, 8);
    mOctaveSpin->setValue(mPianoInput.octave());
    connect(mOctaveSpin, qOverload<int>(&QSpinBox::valueChanged), this,
        [this](int octave) {
            mPianoInput.setOctave(octave);
        });
    auto editStepSpin = new QSpinBox(mToolbarInput);
    editStepSpin->setRange(0, 255);
    editStepSpin->setValue(1);
    connect(editStepSpin, qOverload<int>(&QSpinBox::valueChanged), mPatternEditor, &PatternEditor::setEditStep);
    mToolbarInput->addWidget(new QLabel(tr("Octave"), mToolbarInput));
    mToolbarInput->addWidget(mOctaveSpin);
    mToolbarInput->addWidget(new QLabel(tr("Edit step"), mToolbarInput));
    mToolbarInput->addWidget(editStepSpin);
    mToolbarInput->addSeparator();

    // ACTIONS ===============================================================

    createActions(mInstruments->tableActions(), mWaveforms->tableActions());

    mSidebar->orderEditor()->addActionsToToolbar({
        mActionOrderInsert,
        mActionOrderRemove,
        mActionOrderDuplicate,
        mActionOrderMoveUp,
        mActionOrderMoveDown
        });

    mToolbarSong->addAction(mActionOrderInsert);
    mToolbarSong->addAction(mActionOrderRemove);
    mToolbarSong->addAction(mActionOrderDuplicate);
    mToolbarSong->addAction(mActionOrderMoveUp);
    mToolbarSong->addAction(mActionOrderMoveDown);

    // SHORTCUTS =============================================================

    mShortcutPrevInst = new QShortcut(this);
    lazyconnect(mShortcutPrevInst, activated, this, previousInstrument);

    mShortcutNextInst = new QShortcut(this);
    lazyconnect(mShortcutNextInst, activated, this, nextInstrument);

    mShortcutPrevPatt = new QShortcut(this);
    lazyconnect(mShortcutPrevPatt, activated, this, previousPattern);
    
    mShortcutNextPatt = new QShortcut(this);
    lazyconnect(mShortcutNextPatt, activated, this, nextPattern);

    mShortcutIncOct = new QShortcut(this);
    lazyconnect(mShortcutIncOct, activated, this, increaseOctave);
    
    mShortcutDecOct = new QShortcut(this);
    lazyconnect(mShortcutDecOct, activated, this, decreaseOctave);

    mShortcutPlayStop = new QShortcut(mPatternEditor);
    mShortcutPlayStop->setContext(Qt::WidgetWithChildrenShortcut);
    lazyconnect(mShortcutPlayStop, activated, this, playOrStop);

    // STATUSBAR ==============================================================

    auto statusbar = statusBar();

    mStatusRenderer = new QLabel(statusbar);
    mStatusSpeed = new QLabel(statusbar);
    mStatusTempo = new QLabel(statusbar);
    mStatusElapsed = new QLabel(statusbar);
    mStatusPos = new QLabel(statusbar);
    mStatusSamplerate = new QLabel(statusbar);

    mStatusRenderer->setMinimumWidth(60);
    mStatusSpeed->setMinimumWidth(60);
    mStatusTempo->setMinimumWidth(60);
    mStatusElapsed->setMinimumWidth(40);
    mStatusPos->setMinimumWidth(40);
    mStatusSamplerate->setMinimumWidth(60);

    for (auto label : { 
            mStatusRenderer,
            (QLabel*)mStatusSpeed,
            (QLabel*)mStatusTempo,
            mStatusElapsed,
            mStatusPos,
            mStatusSamplerate
            }) {
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        statusbar->addPermanentWidget(label);
    }

    statusbar->showMessage(tr("Trackerboy v%1").arg(QString::fromLatin1(VERSION_STR)));

    // default statuses
    setPlayingStatus(PlayingStatusText::ready);
    mStatusElapsed->setText(QStringLiteral("00:00"));
    mStatusPos->setText(QStringLiteral("00 / 00"));
    mStatusSpeed->setText(tr("-- FPR"));
    mStatusTempo->setText(tr("-- BPM"));
    // no need to set samplerate, it is done so in onConfigApplied

    // CONNECTIONS ============================================================

    lazyconnect(mSidebar->orderEditor()->grid(), patternJump, mRenderer, jumpToPattern);

    lazyconnect(mPatternEditor, previewNote, mRenderer, instrumentPreview);
    lazyconnect(mPatternEditor, stopNotePreview, mRenderer, stopPreview);

    lazyconnect(&mMidi, error, this, onMidiError);

    connect(mModule, &Module::modifiedChanged, this,
        [this](bool modified) {
            if (modified) {
                if (mAutosave && mModuleFile.hasFile()) {
                    qDebug() << "[MainWindow] Auto-save scheduled";
                    mAutosaveTimer.start(mAutosaveIntervalMs, this);
                }
            } else {
                mAutosaveTimer.stop();
            }
            setWindowModified(modified);
        });

    connect(mRenderer, &Renderer::audioStarted, this, &MainWindow::onAudioStart);
    connect(mRenderer, &Renderer::audioStopped, this, &MainWindow::onAudioStop);
    connect(mRenderer, &Renderer::audioError, this, &MainWindow::onAudioError);
    connect(mRenderer, &Renderer::frameSync, this, &MainWindow::onFrameSync);
    
    auto scope = mSidebar->scope();
    scope->setBuffer(&mRenderer->visualizerBuffer());
    connect(mRenderer, &Renderer::updateVisualizers, scope, qOverload<>(&AudioScope::update));

    lazyconnect(mRenderer, isPlayingChanged, mPatternModel, setPlaying);

    lazyconnect(mInstruments, edit, this, editInstrument);
    lazyconnect(mWaveforms, edit, this, editWaveform);

    connect(mPatternEditor->gridHeader(), &PatternGridHeader::outputChanged, mRenderer, &Renderer::setChannelOutput);

    lazyconnect(mPatternModel, patternCountChanged, this, onPatternCountChanged);
    lazyconnect(mPatternModel, cursorPatternChanged, this, onPatternCursorChanged);
    connect(mPatternModel, &PatternModel::aboutToRemoveLastPattern, this,
        [this]() {
            mRenderer->jumpToPattern(0);
        });

    auto app = static_cast<QApplication*>(QApplication::instance());
    connect(app, &QApplication::focusChanged, this, &MainWindow::handleFocusChange);

    onPatternCountChanged(mPatternModel->patterns());
    onPatternCursorChanged(mPatternModel->cursorPattern());
}

void MainWindow::initState() {
    // setup default layout
    addToolBar(Qt::TopToolBarArea, mToolbarFile);
    mToolbarFile->show();

    addToolBar(Qt::TopToolBarArea, mToolbarEdit);
    mToolbarEdit->show();

    addToolBar(Qt::TopToolBarArea, mToolbarSong);
    mToolbarSong->show();

    addToolBar(Qt::TopToolBarArea, mToolbarTracker);
    mToolbarTracker->show();

    addToolBar(Qt::TopToolBarArea, mToolbarInput);
    mToolbarInput->show();
}

void MainWindow::initSplitters() {
    rationSplitter(*mHSplitter, TU::DEFAULT_HSPLITTER_RATIO);
    rationSplitter(*mVSplitter, TU::DEFAULT_VSPLITTER_RATIO);
}

void MainWindow::settingsMessageBox(QMessageBox &msgbox) {
    auto settingsBtn = msgbox.addButton(tr("Change settings"), QMessageBox::ActionRole);
    msgbox.addButton(QMessageBox::Close);
    msgbox.setDefaultButton(settingsBtn);
    msgbox.exec();

    if (msgbox.clickedButton() == settingsBtn) {
        showConfigDialog();
    }
}

void MainWindow::updateWindowTitle() {
    setWindowTitle(QStringLiteral("%1[*] - Trackerboy").arg(mModuleFile.name()));
}

void MainWindow::setPlayingStatus(PlayingStatusText type) {
    static const char *PLAYING_STATUSES[] = {
        QT_TR_NOOP("Ready"),
        QT_TR_NOOP("Playing"),
        QT_TR_NOOP("Device error")
    };

    mStatusRenderer->setText(tr(PLAYING_STATUSES[(int)type]));
}

void MainWindow::handleFocusChange(QWidget *oldWidget, QWidget *newWidget) {
    Q_UNUSED(oldWidget)

    // this handler is for determining where MIDI events will go, based on
    // who has focus. If MIDI is disabled we don't need to do anything

    // performance notes
    // searching is required by walking the newWidget's parents until either
    // nullptr or the editor dock widgets are found. Performance depends on
    // how deep newWidget is nested in the hierarchy.

    // alternative solution:
    // BaseEditor could have an eventFilter installed on all of its child widgets,
    // that check for FocusIn and FocusOut events, which we could emit a
    // signal for these and the MainWindow would handle them appropriately.

    if (mMidi.isOpen()) {
        if (QApplication::activeModalWidget()) {
            return; // ignore if a dialog is open
        }

        // MIDI events by default go to the pattern editor
        IMidiReceiver *receiver = mPatternEditor;

        QWidget *widget = newWidget;
        while (widget) {
            // search if this widget's parent is the instrument or waveform editor dock
            // if it is, midi events will go to the editor's piano widget
            if (widget == mWaveEditor) {
                receiver = mWaveEditor->piano();
                break;
            } else if (widget == mInstrumentEditor) {
                receiver = mInstrumentEditor->piano();
                break;
            }
            widget = widget->parentWidget();
        }
        mMidi.setReceiver(receiver);
    }

}

namespace TU {

//
// Makes the size of the given list no larger than max
//
static void minimizeList(QStringList &list, int max) {
    while (list.size() > max) {
        list.takeLast();
    }
}

}

void MainWindow::pushRecentFile(const QString &file) {
    QSettings settings;
    settings.beginGroup(TU::KEY_MAIN_WINDOW);

    auto list = settings.value(TU::KEY_RECENT_FILES).toStringList();
    // remove duplicate(s)
    list.removeAll(file);
    list.prepend(file);
    TU::minimizeList(list, (int)mRecentFilesActions.size());

    settings.setValue(TU::KEY_RECENT_FILES, list);
    updateRecentFiles(list);

}

void MainWindow::updateRecentFiles() {
    QSettings settings;
    settings.beginGroup(TU::KEY_MAIN_WINDOW);
    auto list = settings.value(TU::KEY_RECENT_FILES).toStringList();
    TU::minimizeList(list, (int)mRecentFilesActions.size());
    updateRecentFiles(list);
}

void MainWindow::updateRecentFiles(const QStringList &list) {

    int const size = list.size();
    mRecentFilesSeparator->setVisible(size > 0);

    int i = 0;
    for (auto const& filename : list) {
        QFileInfo info(filename);
        QString text = tr("&%1 %2").arg(QString::number(i + 1), info.fileName());
        auto act = mRecentFilesActions[i];
        act->setText(text);
        act->setStatusTip(filename); // put the full filename in the statusbar in case of duplicates
        act->setVisible(true);

        ++i;
    }


    for (auto iter = mRecentFilesActions.begin() + size; iter != mRecentFilesActions.end(); ++iter) {
        (*iter)->setVisible(false);
    }
}

void MainWindow::configureActions(QWidget &widget, ShortcutTable const& shortcuts) {
    for (auto act : widget.actions()) {
        if (act->isSeparator()) {
            continue;
        } else if (auto menu = act->menu(); menu) {
            // recurse into this menu
            configureActions(*menu, shortcuts);
        } else {
            // check if this action has a shortcut
            auto const _data = act->data();
            if (_data.canConvert<ShortcutTable::Shortcut>()) {
                // yes, set the action's shortcut from the configuration
                act->setShortcut(shortcuts.get(_data.value<ShortcutTable::Shortcut>()));
            }
        }
    }
}

#undef TU
