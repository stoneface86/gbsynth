
#include "forms/MainWindow.hpp"

#include "utils/actions.hpp"
#include "utils/connectutils.hpp"
#include "utils/IconLocator.hpp"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>

#ifdef QT_DEBUG

#include <QDesktopServices>
#include <QUrl>

#include <stdexcept>

#endif


#define TU MainWindowTU
namespace TU {

}

void MainWindow::createActions(TableActions const& instrumentActions, TableActions const& waveformActions) {

    // NOTE: For user-configurable shortcuts, set the action's data to the
    // ShortcutTable::Shortcut enum

    auto menubar = menuBar();
    QAction *act;

    // > File ================================================================
    auto menuFile = menubar->addMenu(tr("&File"));

    act = setupAction(menuFile, tr("&New"), tr("Create a new module"), Icons::fileNew, QKeySequence::New);
    mToolbarFile->addAction(act);
    connectActionToThis(act, onFileNew);

    act = setupAction(menuFile, tr("&Open"), tr("Open an existing module"), Icons::fileOpen, QKeySequence::Open);
    mToolbarFile->addAction(act);
    connectActionToThis(act, onFileOpen);
    
    act = setupAction(menuFile, tr("&Save"), tr("Save the module"), Icons::fileSave, QKeySequence::Save);
    mToolbarFile->addAction(act);
    connectActionToThis(act, onFileSave);
    
    act = setupAction(menuFile, tr("Save &As..."), tr("Save the module to a new file"), QKeySequence::SaveAs);
    connectActionToThis(act, onFileSaveAs);

    menuFile->addSeparator(); // ---------------------------------------------
    
    act = setupAction(menuFile, tr("Export to WAV..."), tr("Exports the module to a WAV file"));
    connectActionToThis(act, showExportWavDialog);

    mRecentFilesSeparator = menuFile->addSeparator(); // ---------------------
    mRecentFilesSeparator->setVisible(false);

    // recent files
    for (auto &recent : mRecentFilesActions) {
        recent = new QAction(this);
        recent->setVisible(false);
        menuFile->addAction(recent);
        connectActionToThis(recent, onFileRecent);
    }

    updateRecentFiles();

    menuFile->addSeparator(); // ---------------------------------------------
    
    act = setupAction(menuFile, tr("&Configuration..."), tr("Change application settings"), Icons::fileConfig);
    mToolbarFile->addSeparator();
    mToolbarFile->addAction(act);
    connectActionToThis(act, showConfigDialog);

    menuFile->addSeparator(); // ---------------------------------------------
    
    act = setupAction(menuFile, tr("&Quit"), tr("Exit the application"), QKeySequence::Quit);
    connectActionToThis(act, close);

    // > Edit ================================================================
    auto menuEdit = menubar->addMenu(tr("&Edit"));
    mPatternEditor->setEditMenu(menuEdit);

    auto undoGroup = mModule->undoGroup();
    act = undoGroup->createUndoAction(this);
    act->setIcon(IconLocator::get(Icons::editUndo));
    act->setShortcut(QKeySequence::Undo);
    mToolbarEdit->addAction(act);
    menuEdit->addAction(act);

    act = undoGroup->createRedoAction(this);
    act->setIcon(IconLocator::get(Icons::editRedo));
    act->setShortcut(QKeySequence::Redo);
    mToolbarEdit->addAction(act);
    menuEdit->addAction(act);

    menuEdit->addSeparator(); // ---------------------------------------------
    mToolbarEdit->addSeparator();

    act = setupAction(menuEdit, tr("C&ut"), tr("Copies and deletes selection to the clipboard"), Icons::editCut, QKeySequence::Cut);
    mToolbarEdit->addAction(act);
    connectActionTo(act, mPatternEditor, cut);

    act = setupAction(menuEdit, tr("&Copy"), tr("Copies selected rows to the clipboard"), Icons::editCopy, QKeySequence::Copy);
    mToolbarEdit->addAction(act);
    connectActionTo(act, mPatternEditor, copy);
    
    act = setupAction(menuEdit, tr("&Paste"), tr("Pastes contents at the cursor"), Icons::editPaste, QKeySequence::Paste);
    mToolbarEdit->addAction(act);
    connectActionTo(act, mPatternEditor, paste);
    
    act = setupAction(menuEdit, tr("Paste &Mix"), tr("Pastes contents at the cursor, merging with existing rows"));
    act->setData(ShortcutTable::PasteMix);
    connectActionTo(act, mPatternEditor, pasteMix);

    act = setupAction(menuEdit, tr("&Insert Row"), tr("Inserts an empty row at the cursor"));
    act->setData(ShortcutTable::InsertRow);
    connectActionTo(act, mPatternEditor, insertRow);


    act = setupAction(menuEdit, tr("&Erase"), tr("Erases selection contents"), QKeySequence::Delete);
    connectActionTo(act, mPatternEditor, erase);
    
    menuEdit->addSeparator(); // ---------------------------------------------

    act = setupAction(menuEdit, tr("&Select All"), tr("Selects entire track/pattern"), QKeySequence::SelectAll);
    connectActionTo(act, mPatternEditor, selectAll);

    menuEdit->addSeparator(); // ----------------------------------------------

    // > Edit > Transpose
    auto menuEditTranspose = menuEdit->addMenu(tr("Transpose"));
    
    act = setupAction(menuEditTranspose, tr("Decrease note"), tr("Decreases note/notes by 1 step"));
    act->setData(ShortcutTable::TransposeDecNote);
    connectActionTo(act, mPatternEditor, decreaseNote);
    
    act = setupAction(menuEditTranspose, tr("Increase note"), tr("Increases note/notes by 1 step"));
    act->setData(ShortcutTable::TransposeIncNote);
    connectActionTo(act, mPatternEditor, increaseNote);
    
    act = setupAction(menuEditTranspose, tr("Decrease octave"), tr("Decreases note/notes by 12 steps"));
    act->setData(ShortcutTable::TransposeDecOctave);
    connectActionTo(act, mPatternEditor, decreaseOctave);
    
    act = setupAction(menuEditTranspose, tr("Increase octave"), tr("Increases note/notes by 12 steps"));
    act->setData(ShortcutTable::TransposeIncOctave);
    connectActionTo(act, mPatternEditor, increaseOctave);

    act = setupAction(menuEditTranspose, tr("Custom..."), tr("Transpose by a custom amount"));
    act->setData(ShortcutTable::Transpose);
    connectActionTo(act, mPatternEditor, transpose);

    act = setupAction(menuEdit, tr("&Reverse"), tr("Reverses selected rows"));
    act->setData(ShortcutTable::Reverse);
    connectActionTo(act, mPatternEditor, reverse);

    act = setupAction(menuEdit, tr("Replace instrument"), tr("Replaces all instruments with the current one"));
    act->setData(ShortcutTable::ReplaceInstrument);
    connectActionTo(act, mPatternEditor, replaceInstrument);

    menuEdit->addSeparator(); // ----------------------------------------------

    act = setupAction(menuEdit, tr("Grow pattern"), tr("Grows a selection by adding spaces in between rows"));
    act->setData(ShortcutTable::GrowPattern);
    connectActionTo(act, mPatternEditor, growPattern);

    act = setupAction(menuEdit, tr("Shrink pattern"), tr("Shrinks a selection by removing spaces in between rows"));
    act->setData(ShortcutTable::ShrinkPattern);
    connectActionTo(act, mPatternEditor, shrinkPattern);

    menuEdit->addSeparator(); // ----------------------------------------------

    act = setupAction(menuEdit, tr("Key repetition"), tr("Toggles key repetition for pattern editor"));
    act->setCheckable(true);
    act->setChecked(true);
    mToolbarInput->addAction(act);
    lazyconnect(act, toggled, mPatternEditor, setKeyRepeat);

    // > Module ===============================================================

    auto menuModule = menubar->addMenu(tr("Module"));

    menuModule->addAction(mSidebar->previousSongAction());

    menuModule->addAction(mSidebar->nextSongAction());

    menuModule->addSeparator(); // --------------------------------------------

    act = setupAction(menuModule, tr("Comments..."), tr("Shows comments about the module"));
    connectActionToThis(act, onModuleComments);

    act = setupAction(menuModule, tr("Module properties..."), tr("Opens the module properties dialog"));
    act->setData(ShortcutTable::ModuleProperties);
    connectActionToThis(act, onModuleModuleProperties);

    // > Song =================================================================

    mActionOrderInsert = createAction(this, tr("&Insert order row"), tr("Inserts a new order at the current pattern"), Icons::itemAdd);
    connectActionTo(mActionOrderInsert, mPatternModel, insertOrder);
    
    mActionOrderRemove = createAction(this, tr("&Remove order row"), tr("Removes the order at the current pattern"), Icons::itemRemove);
    connectActionTo(mActionOrderRemove, mPatternModel, removeOrder);

    mActionOrderDuplicate = createAction(this, tr("&Duplicate order row"), tr("Duplicates the order at the current pattern"), Icons::itemDuplicate);
    connectActionTo(mActionOrderDuplicate, mPatternModel, duplicateOrder);

    mActionOrderMoveUp = createAction(this, tr("Move order &up"), tr("Moves the order up 1"), Icons::moveUp);
    connectActionTo(mActionOrderMoveUp, mPatternModel, moveOrderUp);

    mActionOrderMoveDown = createAction(this, tr("Move order dow&n"), tr("Moves the order down 1"), Icons::moveDown);
    connectActionTo(mActionOrderMoveDown, mPatternModel, moveOrderDown);

    auto menuSong = menubar->addMenu(tr("&Song"));
    setupSongMenu(menuSong);

    menuSong->addSeparator();

    act = setupAction(menuSong, tr("Tempo calculator..."), tr("Shows the tempo calculator dialog"));
    connectActionToThis(act, showTempoCalculator);

    // > Instrument ===========================================================
    auto menuInstrument = menubar->addMenu(tr("Instrument"));

    setupTableMenu(menuInstrument, instrumentActions);

    // > Waveform =============================================================
    auto menuWaveform = menubar->addMenu(tr("Waveform"));

    setupTableMenu(menuWaveform, waveformActions);

    // > Tracker ==============================================================
    auto menuTracker = menubar->addMenu(tr("&Tracker"));

    act = setupAction(menuTracker, tr("&Play"), tr("Resume playing or play the song from the current position"), Icons::trackerPlay);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::Play);
    connectActionToThis(act, onTrackerPlay);

    act = setupAction(menuTracker, tr("Play from start"), tr("Begin playback of the song from the start"), Icons::trackerRestart);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::PlayFromStart);
    connectActionToThis(act, onTrackerPlayAtStart);

    act = setupAction(menuTracker, tr("Play at cursor"), tr("Begin playback from the cursor"), Icons::trackerPlayRow);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::PlayFromCursor);
    connectActionToThis(act, onTrackerPlayFromCursor);

    act = setupAction(menuTracker, tr("Step row"), tr("Play and hold the row at the cursor"), Icons::trackerStepRow);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::Step);
    connectActionToThis(act, onTrackerStep);

    act = setupAction(menuTracker, tr("&Stop"), tr("Stop playing"), Icons::trackerStop);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::Stop);
    connectActionToThis(act, onTrackerStop);

    mToolbarTracker->addSeparator();

    act = setupAction(menuTracker, "Pattern repeat", "Toggles pattern repeat mode", Icons::trackerRepeat);
    act->setCheckable(true);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::PatternRepeat);
    lazyconnect(act, toggled, mRenderer, setPatternRepeat);

    act = setupAction(menuTracker, tr("Record"), tr("Toggles record mode"), Icons::trackerRecord);
    act->setCheckable(true);
    mToolbarTracker->addAction(act);
    act->setData(ShortcutTable::Record);
    lazyconnect(act, toggled, mPatternModel, setRecord);
    
    mActionFollowMode = setupAction(menuTracker, tr("Follow-mode"), tr("Toggles follow mode"), Qt::Key_ScrollLock);
    mActionFollowMode->setCheckable(true);
    mActionFollowMode->setChecked(true);
    lazyconnect(mActionFollowMode, toggled, mPatternModel, setFollowing);

    mToolbarTracker->addAction(mSidebar->previousSongAction());
    mToolbarTracker->addAction(mSidebar->nextSongAction());
    mToolbarTracker->addAction(mActionFollowMode);

    menuTracker->addSeparator(); // -------------------------------------------

    act = setupAction(menuTracker, tr("Toggle channel output"), tr("Enables/disables sound output for the current track"));
    act->setData(ShortcutTable::ToggleOutput);
    connectActionToThis(act, onTrackerToggleOutput);

    act = setupAction(menuTracker, tr("Solo"), tr("Solos the current track"));
    act->setData(ShortcutTable::Solo);
    connectActionToThis(act, onTrackerSolo);

    menuTracker->addSeparator(); // -------------------------------------------

    act = setupAction(menuTracker, tr("Reset volume"), tr("Resets the APU global volume setting"));
    connectActionTo(act, mRenderer, resetGlobalVolume);

    act = setupAction(menuTracker, tr("&Kill sound"), tr("Immediately stops sound output"));
    act->setData(ShortcutTable::Kill);
    connectActionToThis(act, onTrackerKill);

    

    // > View =================================================================
    auto menuView = menubar->addMenu(tr("&View"));

    mActionViewHistory = createAction(this, tr("History"), tr("Shows the undo/redo history"));
    connectActionToThis(mActionViewHistory, showHistory);

    mActionViewReset = createAction(this, tr("Reset layout"), tr("Rearranges all docks and toolbars to the default layout"));
    connectActionToThis(mActionViewReset, onViewResetLayout);

    setupViewMenu(menuView);

    // > Help =================================================================
    auto menuHelp = menubar->addMenu(tr("&Help"));

    #ifdef QT_DEBUG
    // secret debug menu

    auto menuHelpDebug = menuHelp->addMenu(tr("Debug"));
    act = setupAction(menuHelpDebug, tr("Dump QObject tree"), tr("Calls MainWindow::dumpObjectTree()"));
    connect(act, &QAction::triggered, this, [this]() {
        dumpObjectTree();
    });

    act = setupAction(menuHelpDebug, tr("Clear config"), tr("Deletes the application configuration, window state and geometry"));
    connect(act, &QAction::triggered, this, [this](){
        mSaveConfig = false;
        QSettings settings;
        settings.clear();
    });

    act = setupAction(menuHelpDebug, tr("Panic"), tr("Forces a fatal error to occur"));
    connect(act, &QAction::triggered, this, []() {
        qFatal("panic requested");
    });

    act = setupAction(menuHelpDebug, tr("Panic (exception)"), tr("Force a fatal error to occur via exception"));
    connect(act, &QAction::triggered, this, []() {
        throw std::invalid_argument("panic requested");
    });

    act = setupAction(menuHelpDebug, tr("Open configuration..."), tr("Opens the appliciation configuration file"));
    connect(act, &QAction::triggered, this, []() {
        QSettings settings;
        auto path = settings.fileName();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));

    });

    #endif

    act = setupAction(menuHelp, tr("&Effects list..."), tr("Shows a list of all available effects"));
    act->setData(ShortcutTable::ShowEffectsList);
    connectActionToThis(act, showEffectsList);

    act = setupAction(menuHelp, tr("User &manual..."), tr("Opens the online user manual"));
    act->setData(ShortcutTable::ShowUserManual);
    connectActionToThis(act, showUserManual);

    act = setupAction(menuHelp, tr("Audio &diagnostics..."), tr("Shows the audio diagnostics dialog"));
    connectActionToThis(act, showAudioDiag);

    menuHelp->addSeparator(); // ----------------------------------------------
    
    act = setupAction(menuHelp, tr("&About"), tr("About this program"));
    connectActionToThis(act, showAboutDialog);
    
    act = setupAction(menuHelp, tr("About &Qt"), tr("Shows information about Qt"));
    QApplication::connect(act, &QAction::triggered, &QApplication::aboutQt);


}

void MainWindow::setupSongMenu(QMenu *menu) {
    menu->addAction(mActionOrderInsert);
    menu->addAction(mActionOrderRemove);
    menu->addAction(mActionOrderDuplicate);
    menu->addSeparator();
    menu->addAction(mActionOrderMoveUp);
    menu->addAction(mActionOrderMoveDown); 
}

void MainWindow::setupViewMenu(QMenu *menu) {
    menu->addAction(mActionViewHistory);
    menu->addSeparator();
    auto toolbarMenu = menu->addMenu(tr("Toolbars"));
    for (auto toolbar : {
            mToolbarFile,
            mToolbarEdit, 
            mToolbarSong,
            mToolbarTracker,
            mToolbarInput
        }) {
        toolbarMenu->addAction(toolbar->toggleViewAction());
    }
    menu->addSeparator();   
    menu->addAction(mActionViewReset);
}

void MainWindow::setupTableMenu(QMenu *menu, TableActions const& actions) {

    menu->addAction(actions.add);
    menu->addAction(actions.remove);
    menu->addAction(actions.duplicate);

    menu->addSeparator();
    
    menu->addAction(actions.importFile);
    menu->addAction(actions.exportFile);

    menu->addSeparator();
    
    menu->addAction(actions.edit);

}

#undef TU
