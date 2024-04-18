
#pragma once

#include "clipboard/PatternClip.hpp"
#include "model/SongModel.hpp"
#include "core/Module.hpp"
#include "core/PatternCursor.hpp"
#include "core/PatternSelection.hpp"

#include "trackerboy/data/Pattern.hpp"
#include "trackerboy/data/Order.hpp"

#include <QObject>
#include <QRect>

#include <array>
#include <optional>


//
// Model class for accessing/modifying pattern data for a song.
//
class PatternModel : public QObject {

    Q_OBJECT

public:

    enum CursorChangeFlag {
        CursorRowChanged = 0x1,
        CursorColumnChanged = 0x2,
        CursorTrackChanged = 0x4,
        CursorUnchanged = 0x0
    };
    Q_DECLARE_FLAGS(CursorChangeFlags, CursorChangeFlag)

    enum SelectMode {
        SelectionKeep,      // the current selection will be kept
        SelectionModify,    // the current selection will be modified
        SelectionRemove     // the current selection will be deselected
    };


    explicit PatternModel(Module &mod, SongModel &songModel, QObject *parent = nullptr);
    virtual ~PatternModel() = default;

    // Data Access ============================================================

    // TODO: make these accessors private (for command classes), add public const versions

    trackerboy::Pattern* previousPattern();

    trackerboy::Pattern& currentPattern();

    trackerboy::Pattern* nextPattern();

    trackerboy::Order& order();
    trackerboy::Order const& order() const;

    trackerboy::OrderRow currentOrderRow() const;

    // Properties =============================================================

    int cursorRow() const;

    int cursorColumn() const;

    //
    // Absolute column, includes the sum of the columns in the tracks
    // before it, used exclusively by the PatternEditor horizontal scrollbar
    //
    int cursorAbsoluteColumn() const;

    int cursorTrack() const;

    int cursorPattern() const;

    PatternCursor cursor() const;

    int trackerCursorRow() const;
    int trackerCursorPattern() const;

    bool isRecording() const;

    bool isFollowing() const;

    bool isPlaying() const;

    void setTrackerCursor(int row, int pattern);
    void setPlaying(bool playing);

    void setCursorWrap(bool wrap);

    void setCursorWrapPattern(bool wrap);

    int patterns() const;

    int totalColumns() const;

    trackerboy::EffectCounts effectsVisible() const;

    // Selection ==============================================================

    //
    // Returns true if there is a selection set, false otherwise.
    //
    bool hasSelection() const;

    //
    // Gets the current selection.
    //
    PatternSelection selection() const;

    //
    // Sets the selection. If nothing is selected, then the item at the given
    // point is selected. If there is a selection, then the end point is set
    // to the given point.
    //
    void setSelection(PatternCursor pos);

    //
    // Calls setSelection with the current cursor position.
    //
    void selectCursor();

    //
    // Selects entire the entire track or the entire pattern. The track is
    // selected first. If the function is called again with the track selected
    // then the entire pattern is selected. Therefore, repeatedly calling this
    // function will alternate between selecting the track and pattern.
    //
    void selectAll();

    //
    // Selects the entire row
    //
    void selectRow(int row);

    //
    // Removes the current selection
    //
    void deselect();

    //
    // Gets a clip of the current selection or the cursor if there is no selection
    //
    PatternClip clip();

    // Editing ================================================================

    // unless specified, these functions use the current cursor position for
    // editing.

    //
    // sets the note for the current track at the cursor row. An empty optional
    // deletes the note set. If an instrument is provided, it will be set along
    // with the note
    //
    void setNote(std::optional<uint8_t> note, std::optional<uint8_t> instrument);
    void setInstrument(std::optional<uint8_t> nibble);
    void setEffectType(trackerboy::EffectType type);
    void setEffectParam(uint8_t nibble);

    // deletes selection or the cursor if no selection is present
    void deleteSelection();

    // transpose the current note or selection by the given number of semitones
    void transpose(int amount);

    // reverses rows in a selection
    void reverse();

    // replaces all instrument colums in the selection with the given instrument
    void replaceInstrument(int instrument);

    // moves the selected data to a new position
    void moveSelection(PatternCursor pos);

    void paste(PatternClip const& clip, bool mix);

    void backspace();

    void insertRow();

    void growPattern();

    void shrinkPattern();

    // order

    //
    // Replaces the current order row with the given one
    //
    void setOrderRow(trackerboy::OrderRow row);

    //
    // Removes the current order
    //
    void removeOrder();

    //
    // Inserts an empty order at the cursor position
    //
    void insertOrder();

    //
    // Inserts a copy of the current order
    //
    void duplicateOrder();

    //
    // Moves the current order up in the list by swapping it with the previous one
    //
    void moveOrderUp();

    //
    // Moves the current order down in the list by swapping it with the next one
    //
    void moveOrderDown();

    //
    // Makes an effect column visible for the track
    //
    void showEffect(int track);

    //
    // Hides the last effect column for the track
    //
    void hideEffect(int track);

    void moveCursorRow(int amount, PatternModel::SelectMode mode = SelectionKeep);
    void moveCursorColumn(int amount, PatternModel::SelectMode mode = SelectionKeep);
    void moveCursorTrack(int amount);

    void setCursorRow(int row);
    void setCursorColumn(int column);
    void setCursorAbsoluteColumn(int column);
    void setCursorTrack(int track);
    void setCursorPattern(int pattern);

    void setCursor(PatternCursor const cursor);

    void setRecord(bool recording);

    void setFollowing(bool following);

    void setPreviewEnable(bool previews);

signals:
    void cursorChanged(PatternModel::CursorChangeFlags flags);
    void cursorPatternChanged(int pattern);

    void patternSizeChanged(int rows);
    void patternCountChanged(int patterns);
    void trackerCursorChanged(int row, int pattern);
    void trackerCursorPatternChanged(int pattern);
    void playingChanged(bool playing);
    void recordingChanged(bool recording);
    void aboutToRemoveLastPattern();

    void selectionChanged();

    //
    // emitted when a change has been made to the current pattern and should
    // be redrawn.
    //
    void invalidated();

    void effectsVisibleChanged();

    void totalColumnsChanged(int columns);

private:


    // QUndoCommand command classes
    friend class TrackEditCmd;
    friend class SelectionCmd;
    friend class EraseCmd;
    friend class PasteCmd;
    friend class TransposeCmd;
    friend class ReverseCmd;
    friend class ReplaceInstrumentCmd;
    friend class BackspaceCmd;
    friend class OrderEditCmd;
    friend class OrderInsertCmd;
    friend class OrderRemoveCmd;
    friend class OrderDuplicateCmd;
    friend class OrderSwapCmd;
    friend class InsertRowCmd;
    friend class GrowCmd;
    friend class ShrinkCmd;

    Q_DISABLE_COPY(PatternModel)

    // get the source of the model's data (the current song)
    trackerboy::Song* source() const;
    
    void setCursorRowImpl(int row, CursorChangeFlags &flags);
    void setCursorColumnImpl(int col, CursorChangeFlags &flags);
    void setCursorTrackImpl(int track, CursorChangeFlags &flags);
    void setCursorPatternImpl(int pattern, CursorChangeFlags &flags);

    void setPatterns(int pattern, CursorChangeFlags &flags);
    void setPreviewPatterns(int pattern);

    void emitIfChanged(CursorChangeFlags flags);

    int cursorEffectNo();

    void setMaxColumns();

    //
    // Shows/hides a given number of effects for the track. The new count of
    // effects visible is returned
    //
    int addEffects(int track, int effectsToAdd);

    trackerboy::TrackRow const& cursorTrackRow();

    void invalidate(int pattern, bool updatePatterns);

    bool selectionDataIsEmpty();

    // called by insert, remove and duplicate commands
    void insertOrderImpl(trackerboy::OrderRow const& row, int before);
    void removeOrderImpl(int at);


    bool canGrowOrShrink() const;

    Module &mModule;

    PatternCursor mCursor;
    int mCursorPattern;

    bool mRecording;
    bool mFollowing;
    bool mPlaying;
    bool mShowPreviews;
    bool mWrapCursor;
    bool mWrapPattern;

    int mTrackerRow;
    int mTrackerPattern;

    std::optional<trackerboy::Pattern> mPatternPrev;
    trackerboy::Pattern mPatternCurr;
    std::optional<trackerboy::Pattern> mPatternNext;

    bool mHasSelection;
    PatternSelection mSelection;

    std::array<int, 4> mMaxColumns;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(PatternModel::CursorChangeFlags)
