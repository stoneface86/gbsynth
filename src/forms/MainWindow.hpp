
#pragma once

#include "audio/AudioEnumerator.hpp"
#include "audio/Renderer.hpp"
#include "config/Config.hpp"
#include "config/ConfigDialog.hpp"
#include "utils/TableActions.hpp"
#include "model/PatternModel.hpp"
#include "model/SongModel.hpp"
#include "model/SongListModel.hpp"
#include "model/TableModel.hpp"
#include "core/Module.hpp"
#include "core/ModuleFile.hpp"
#include "config/data/PianoInput.hpp"
#include "forms/editors/InstrumentEditor.hpp"
#include "forms/editors/WaveEditor.hpp"
#include "forms/AudioDiagDialog.hpp"
#include "forms/TempoCalculator.hpp"
#include "forms/CommentsDialog.hpp"
#include "forms/EffectsListDialog.hpp"
#include "midi/Midi.hpp"
#include "widgets/PatternEditor.hpp"
#include "widgets/Sidebar.hpp"
#include "widgets/TableView.hpp"

#include "trackerboy/engine/Frame.hpp"

#include <QBasicTimer>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QToolBar>
#include <QSpinBox>
#include <QSplitter>
#include <QShortcut>

//
// Main form for the application
//
class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    explicit MainWindow();

    virtual QMenu* createPopupMenu() override;

    //
    // A crash has occurred, attempt to save a copy of the current module
    //
    void panic(QString const& msg);

    void openFile(QString const& filepath);

protected:

    virtual void closeEvent(QCloseEvent *evt) override;

    virtual void showEvent(QShowEvent *evt) override;

    virtual void timerEvent(QTimerEvent *evt) override;

    virtual void dragEnterEvent(QDragEnterEvent *evt) override;

    virtual void dropEvent(QDropEvent *evt) override;

private:

    // implementation in MainWindow/slots.cpp - BEGIN -------------------------

    // actions
    void onFileNew();
    void onFileOpen();
    bool onFileSave();
    bool onFileSaveAs();
    void onFileRecent();

    void onModuleComments();
    void onModuleModuleProperties();

    void onTrackerPlay();
    void onTrackerPlayAtStart();
    void onTrackerPlayFromCursor();
    void onTrackerStep();
    void onTrackerStop();
    void onTrackerSolo();
    void onTrackerToggleOutput();
    void onTrackerKill();
    
    void onViewResetLayout();

    void onMidiError();

    //
    // Applies the current configuration for the given categories. An optional list of problems
    // can be given and will be populated with an error string for each error.
    //
    Config::Categories applyConfig(Config const& config, Config::Categories categories, QString *problems = nullptr);

    // dialog show slots (lazy loading)
    void showAboutDialog();
    void showAudioDiag();
    void showConfigDialog();
    void showUserManual();
    void showEffectsList();
    void showExportWavDialog();
    void showTempoCalculator();
    void showInstrumentEditor();
    void showWaveEditor();
    void showHistory();

    void onAudioStart();
    void onAudioError();
    void onAudioStop();
    void onFrameSync();

    // shortcut slots
    void previousInstrument();
    void nextInstrument();
    void previousPattern();
    void nextPattern();
    void increaseOctave();
    void decreaseOctave();
    void playOrStop();

    // misc slots

    // these slots update order actions
    void onPatternCountChanged(int count);
    void onPatternCursorChanged(int pattern);

    void editInstrument(int item);
    void editWaveform(int item);

    // implementation in MainWindow/slots.cpp - END ---------------------------

private:
    Q_DISABLE_COPY(MainWindow)

    enum class PlayingStatusText {
        ready,
        playing,
        error
    };

    //
    // Creates a new toolbar with the given title and object name. This class
    // takes ownership of the toolbar.
    //
    QToolBar* makeToolbar(QString const& title, QString const& objname);

    //
    // Utility function for tracker play slots. Checks if the renderer is
    // stepping and if so, steps out. Does nothing otherwise. true is returned
    // if the renderer was stepped out, false otherwise.
    //
    bool checkAndStepOut();

    // implementation in MainWindow/actions.cpp - BEGIN -----------------------

    //
    // Creates all actions used by the application
    //
    void createActions(TableActions const& instrumentActions, TableActions const& waveformActions);

    //
    // Setups the View menu in the given QMenu instance.
    //
    void setupViewMenu(QMenu *menu);

    //
    // Adds order actions to the given menu
    //
    void setupSongMenu(QMenu *menu);

    //
    // setups the menu with actions for table operations using the given model
    //
    void setupTableMenu(QMenu *menu, TableActions const& actions);

    // implementation in MainWindow/actions.cpp - END -------------------------

    //
    // Prompts the user to save if the module is modified, do nothing otherwise
    // To be called before saving, loading and when closing the application. If
    // this function returns false, do not continue with the save/load or close.
    // true is returned if:
    //  * The user was prompted to save, saved, and the save was successful
    //  * The user was prompted to save but chose to discard changes
    //  * The document was not modified, so the user was not prompted to save
    // false is returned for all other cases
    //
    bool maybeSave();

    //
    // Setups the UI, should only be called once and by the constructor
    //
    void setupUi();

    //
    // Resets all toolbars and docks to the initial state.
    //
    void initState();

    //
    // initialize the size of each splitter to default
    //
    void initSplitters();

    //
    // Shows and adds a "Change settings" button that opens the configuration
    // to the given QMessageBox
    //
    void settingsMessageBox(QMessageBox &msgbox);

    //
    // Sets the window title using the current filename
    //
    void updateWindowTitle();

    //
    // Sets the playing status text in statusbar
    //  PlayingStatusText::playing - "Playing"
    //  PlayingStatusText::ready - "Ready"
    //  PlayingStatusText::error - "Device error"
    //
    void setPlayingStatus(PlayingStatusText type);

    //
    // Updates the current midi receiver based on the newWidget that
    // recieved focus.
    //
    void handleFocusChange(QWidget *oldWidget, QWidget *newWidget);

    //
    // Pushes the given filename to the recent files list. Each file that is
    // successfully opened and newly saved files should get added to this list
    //
    void pushRecentFile(QString const& file);

    //
    // Updates the recent files actions with the list stored in the config
    //
    void updateRecentFiles();

    //
    // Updates the recent files actions using the given list of filenames
    //
    void updateRecentFiles(QStringList const& list);

    //
    // Configures key sequences for all actions in the given widget. A shortcut is
    // assigned to the action if the action's data property contains a ShortcutTable::Shortcut
    // value. This function applies shortcuts to all of the widget's actions and all actions
    // in each submenu.
    //
    void configureActions(QWidget &widget, ShortcutTable const& shortcuts);

    AudioEnumerator mAudioEnumerator;
    MidiEnumerator mMidiEnumerator;
    QString const mUntitledString;

    #ifdef QT_DEBUG
    bool mSaveConfig = true;
    #endif

    //Config mConfig;
    PianoInput mPianoInput;
    Palette mPalette;

    Midi mMidi;

    Module *mModule;
    ModuleFile mModuleFile;

    InstrumentListModel *mInstrumentModel;
    SongListModel *mSongListModel;
    SongModel *mSongModel;
    PatternModel *mPatternModel;
    WaveListModel *mWaveModel;

    Renderer *mRenderer;

    bool mErrorSinceLastConfig;
    trackerboy::Frame mLastEngineFrame;
    int mFrameSkip;

    bool mAutosave;
    int mAutosaveIntervalMs;
    QBasicTimer mAutosaveTimer;

    // dialogs
    AudioDiagDialog *mAudioDiag;
    TempoCalculator *mTempoCalc;
    CommentsDialog *mCommentsDialog;
    InstrumentEditor *mInstrumentEditor;
    WaveEditor *mWaveEditor;
    PersistantDialog *mHistoryDialog;
    EffectsListDialog *mEffectsListDialog;

    // toolbars
    QToolBar *mToolbarFile;
    QToolBar *mToolbarEdit;
    QToolBar *mToolbarSong;
    QToolBar *mToolbarTracker;
    QToolBar *mToolbarInput;
        QSpinBox *mOctaveSpin;

    // actions shared by multiple menus (parented by MainWindow)
    QAction *mActionOrderInsert;
    QAction *mActionOrderRemove;
    QAction *mActionOrderDuplicate;
    QAction *mActionOrderMoveUp;
    QAction *mActionOrderMoveDown;

    QAction *mActionViewHistory;
    QAction *mActionViewReset;

    QAction *mActionFollowMode;

    QMenu *mSongOrderContextMenu;

    // recent file actions
    std::array<QAction*, 9> mRecentFilesActions;
    QAction* mRecentFilesSeparator;

    // widgets
    Sidebar *mSidebar;
    PatternEditor *mPatternEditor;
    TableView *mInstruments;
    TableView *mWaveforms;
    QSplitter *mHSplitter;
    QSplitter *mVSplitter;

    // statusbar widgets
    QLabel *mStatusRenderer;
    QLabel *mStatusSpeed;
    QLabel *mStatusTempo;
    QLabel *mStatusElapsed;
    QLabel *mStatusPos;
    QLabel *mStatusSamplerate;

    // shortcuts
    QShortcut *mShortcutPrevInst;
    QShortcut *mShortcutNextInst;
    QShortcut *mShortcutPrevPatt;
    QShortcut *mShortcutNextPatt;
    QShortcut *mShortcutIncOct;
    QShortcut *mShortcutDecOct;
    QShortcut *mShortcutPlayStop;


};
