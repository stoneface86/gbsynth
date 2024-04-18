
#include "model/PatternModel.hpp"
#include "model/commands/order.hpp"
#include "model/commands/pattern.hpp"
#include "utils/utils.hpp"

#include "trackerboy/note.hpp"

#include <QUndoCommand>
#include <QtDebug>

#include <algorithm>
#include <memory>

#define TU PatternModelTU
namespace TU {

}


PatternModel::PatternModel(Module &mod, SongModel &songModel, QObject *parent) :
    QObject(parent),
    mModule(mod),
    mCursor(),
    mCursorPattern(0),
    mRecording(false),
    mFollowing(true),
    mPlaying(false),
    mShowPreviews(true),
    mWrapCursor(true),
    mWrapPattern(true),
    mTrackerRow(0),
    mTrackerPattern(0),
    mPatternPrev(),
    mPatternCurr(mod.song()->getPattern(0)),
    mPatternNext(),
    mHasSelection(false),
    mSelection()
{
    setMaxColumns();
    connect(&songModel, &SongModel::patternSizeChanged, this,
        [this](int rows) {
            CursorChangeFlags flags = CursorUnchanged;
            if (mCursor.row >= rows) {
                mCursor.row = rows - 1;
                flags |= CursorRowChanged;
            }
            setPatterns(mCursorPattern, flags);
            emitIfChanged(flags);
        });

    connect(&mModule, &Module::songChanged, this,
        [this]() {
            mCursorPattern = -1;
            setCursorPattern(0);
            setCursor(PatternCursor(0, 0, 0));
            deselect();
            mTrackerRow = 0;
            mTrackerPattern = 0;
            setMaxColumns();
            emit effectsVisibleChanged();
            emit patternCountChanged(patterns());
        });
}

trackerboy::Pattern* PatternModel::previousPattern() {
    return mPatternPrev ? &*mPatternPrev : nullptr;
}

trackerboy::Pattern& PatternModel::currentPattern() {
    return mPatternCurr;
}

trackerboy::Pattern* PatternModel::nextPattern() {
    return mPatternNext ? &*mPatternNext : nullptr;
}

trackerboy::Order& PatternModel::order() {
    return source()->order();
}

trackerboy::Order const& PatternModel::order() const {
    return source()->order();
}

trackerboy::OrderRow PatternModel::currentOrderRow() const {
    return order()[mCursorPattern];
}

PatternCursor PatternModel::cursor() const {
    return mCursor;
}

int PatternModel::cursorRow() const {
    return mCursor.row;
}

int PatternModel::cursorColumn() const {
    return mCursor.column;
}

int PatternModel::cursorAbsoluteColumn() const {
    int absolute = std::accumulate(
        mMaxColumns.begin(),
        mMaxColumns.begin() + mCursor.track,
        0
    );
    return absolute + mCursor.column;
}

int PatternModel::cursorTrack() const {
    return mCursor.track;
}

int PatternModel::cursorPattern() const {
    return mCursorPattern;
}

int PatternModel::trackerCursorRow() const {
    return mTrackerRow;
}

int PatternModel::trackerCursorPattern() const {
    return mTrackerPattern;
}

bool PatternModel::isRecording() const {
    return mRecording;
}

bool PatternModel::isFollowing() const {
    return mFollowing;
}

bool PatternModel::isPlaying() const {
    return mPlaying;
}

int PatternModel::patterns() const {
    return source()->order().size();
}

int PatternModel::totalColumns() const {
    return std::accumulate(mMaxColumns.begin(), mMaxColumns.end(), 0);
}

trackerboy::EffectCounts PatternModel::effectsVisible() const {
    return source()->effectCounts();
}

bool PatternModel::hasSelection() const {
    return mHasSelection;
}

PatternSelection PatternModel::selection() const {
    return mSelection;
}

void PatternModel::setSelection(PatternCursor pos) {
    PatternAnchor anchor(pos);
    if (mHasSelection) {
        if (anchor != mSelection.end()) {
            mSelection.setEnd(anchor);
            emit selectionChanged();
        }
    } else {
        mHasSelection = true;
        // no selection, start with just the given point selected
        mSelection = PatternSelection(anchor);
        emit selectionChanged();
    }
}

void PatternModel::selectCursor() {
    setSelection(mCursor);
}

void PatternModel::selectAll() {
    // check if the entire track is selected
    auto const lastRow = (int)mPatternCurr.totalRows() - 1;
    if (mHasSelection) {
        auto iter = mSelection.iterator();
        if (
            iter.rowStart() == 0 && iter.rowEnd() == lastRow &&
            iter.trackStart() == iter.trackEnd() &&
            iter.columnStart() == 0 && iter.columnEnd() == PatternAnchor::MAX_SELECTS - 1
        ) {
            // yes, select all instead
            mHasSelection = true;
            mSelection.setStart({ 0, 0, 0 });
            mSelection.setEnd({ lastRow, PatternAnchor::MAX_SELECTS - 1, 3 });
            emit selectionChanged();
            return;
        }
    } else {
        mHasSelection = true;
    }

    // select track
    mSelection.setStart({ 0, 0, mCursor.track });
    mSelection.setEnd({ lastRow, PatternAnchor::MAX_SELECTS - 1, mCursor.track });
    emit selectionChanged();
}

void PatternModel::selectRow(int row) {
    if (row >= 0 && row < (int)mPatternCurr.totalRows()) {
        if (!mHasSelection) {
            mHasSelection = true;
            mSelection.setStart({row, 0, 0});
        }
        mSelection.setEnd({row, PatternAnchor::MAX_SELECTS - 1, PatternAnchor::MAX_TRACKS - 1});
        emit selectionChanged();
    }
}

void PatternModel::deselect() {
    if (mHasSelection) {
        mHasSelection = false;
        emit selectionChanged();
    }
}

PatternClip PatternModel::clip() {
    PatternClip clip;
    
    if (mHasSelection) {
        clip.save(mPatternCurr, mSelection);
    } else {
        clip.save(mPatternCurr, PatternSelection(mCursor));
    }

    return clip;
}

// slots -------------------------------

void PatternModel::moveCursorRow(int amount, SelectMode mode) {
    if (mPlaying && mFollowing) {
        return;
    }
    
    if (mode == SelectionModify) {
        selectCursor();
    } else if (mode == SelectionRemove) {
        deselect();
    }
    setCursorRow(mCursor.row + amount);
    if (mode == SelectionModify) {
        selectCursor();
    }
}

void PatternModel::moveCursorColumn(int amount, SelectMode mode) {
    if (mode == SelectionModify) {
        selectCursor();
    } else if (mode == SelectionRemove) {
        deselect();
    }
    setCursorColumn(mCursor.column + amount);
    if (mode == SelectionModify) {
        selectCursor();
    }
}

void PatternModel::moveCursorTrack(int amount) {
    setCursorTrack(mCursor.track + amount);
}

void PatternModel::setCursorRow(int row) {
    if (mPlaying && mFollowing) {
        return;
    }

    CursorChangeFlags flags = CursorUnchanged;
    setCursorRowImpl(row, flags);
    emitIfChanged(flags);
}

void PatternModel::setCursorRowImpl(int row, CursorChangeFlags &flags) {
    if (row == mCursor.row) {
        return;
    }

    int oldRow = mCursor.row;
    int newRow;
    if (row < 0) {
        // go to the previous pattern or wrap around to the last one
        if (mWrapPattern) {
            int prevPattern;
            if (mCursorPattern == 0) {
                prevPattern = patterns() - 1;
            } else {
                prevPattern = mCursorPattern - 1;
            }
            setCursorPatternImpl(prevPattern, flags);
            newRow = std::max(0, (int)mPatternCurr.totalRows() + row);
        } else {
            newRow = 0;
        }
    } else if (row >= (int)mPatternCurr.totalRows()) {
        if (mWrapPattern) {
            // go to the next pattern or wrap around to the first one
            row -= mPatternCurr.totalRows();
            auto nextPattern = mCursorPattern + 1;
            if (nextPattern == patterns()) {
                nextPattern = 0;
            }
            setCursorPatternImpl(nextPattern, flags);
            newRow = std::min((int)mPatternCurr.totalRows() - 1, row);
        } else {
            newRow = mPatternCurr.totalRows() - 1;
        }
    } else {
        newRow = row;
    }

    if (newRow != oldRow) {
        mCursor.row = newRow;
        flags |= CursorRowChanged;
    }
}

void PatternModel::setCursorColumn(int column) {
    CursorChangeFlags flags = CursorUnchanged;
    setCursorColumnImpl(column, flags);
    emitIfChanged(flags);
}

void PatternModel::setCursorAbsoluteColumn(int absolute) {
    // find the track
    int track = -1;
    for (auto max : mMaxColumns) {
        ++track;
        if (absolute < max) {
            break;
        }
        absolute -= max;
    }

    setCursorTrack(track);
    setCursorColumn(absolute);
}

void PatternModel::setCursorColumnImpl(int column, CursorChangeFlags &flags) {
    if (column == mCursor.column) {
        return;
    }

    if (column < 0) {
        auto track = mCursor.track - 1;
        setCursorTrackImpl(track, flags);
        if (flags & CursorTrackChanged) {
            column = mMaxColumns[mCursor.track] - 1;
        } else {
            column = 0;
        }
    } else if (column >= mMaxColumns[mCursor.track]) {
        setCursorTrackImpl(mCursor.track + 1, flags);
        if (flags & CursorTrackChanged) {
            column = 0;
        } else {
            column = mMaxColumns[mCursor.track] - 1;
        }
    }

    if (mCursor.column != column) {
        mCursor.column = column;
        flags |= CursorColumnChanged;
    }

}

void PatternModel::setCursorTrack(int track) {
    CursorChangeFlags flags = CursorUnchanged;
    setCursorTrackImpl(track, flags);
    emitIfChanged(flags);
}

void PatternModel::setCursorTrackImpl(int track, CursorChangeFlags &flags) {
    if (mCursor.track == track) {
        return;
    }

    if (mWrapCursor) {
        if (track < 0) {
            track = PatternCursor::MAX_TRACKS - (-track % PatternCursor::MAX_TRACKS);
        } else if (track > 3) {
            track %= PatternCursor::MAX_TRACKS;
        }
    } else {
        track = std::clamp(track, 0, PatternCursor::MAX_TRACKS - 1);
    }

    if (track != mCursor.track) {
        mCursor.track = track;
        flags |= CursorTrackChanged;
    }
}

void PatternModel::setCursor(PatternCursor const cursor) {
    CursorChangeFlags flags = CursorUnchanged;
    if (!(mPlaying && mFollowing)) {
        setCursorRowImpl(cursor.row, flags);
    }
    setCursorColumnImpl(cursor.column, flags);
    setCursorTrackImpl(cursor.track, flags);
    emitIfChanged(flags);
}

void PatternModel::setCursorPattern(int pattern) {
    if (pattern < 0 || pattern >= patterns()) {
        return;
    }

    CursorChangeFlags flags = CursorUnchanged;
    setCursorPatternImpl(pattern, flags);
    emitIfChanged(flags);
}

void PatternModel::setCursorPatternImpl(int pattern, CursorChangeFlags &flags) {
    if (mCursorPattern == pattern) {
        return;
    }

    mCursorPattern = pattern;
    setPatterns(pattern, flags);
    emit cursorPatternChanged(pattern);
    deselect();
}

void PatternModel::setTrackerCursor(int row, int pattern) {
    bool changed = false;
    if (mTrackerPattern != pattern) {
        mTrackerPattern = pattern;
        emit trackerCursorPatternChanged(pattern);
        changed = true;
    }
    if (mTrackerRow != row) {
        mTrackerRow = row;
        changed = true;
    }

    if (changed) {
        if (mFollowing) {
            CursorChangeFlags flags = CursorUnchanged;
            setCursorPatternImpl(pattern, flags);
            setCursorRowImpl(row, flags);
            emitIfChanged(flags);
        }
        emit trackerCursorChanged(row, pattern);
    }
}

void PatternModel::setFollowing(bool following) {
    mFollowing = following;
}

void PatternModel::setPlaying(bool playing) {
    if (mPlaying != playing) {
        mPlaying = playing;
        emit playingChanged(playing);
    }
}

void PatternModel::setRecord(bool record) {
    if (mRecording != record) {
        mRecording = record;
        emit recordingChanged(record);
    }
}

void PatternModel::setPreviewEnable(bool enable) {
    if (mShowPreviews != enable) {
        mShowPreviews = enable;
        if (enable) {
            setPreviewPatterns(mCursorPattern);
        } else {
            mPatternPrev.reset();
            mPatternNext.reset();
        }
        emit invalidated();
    }
}

void PatternModel::setCursorWrap(bool wrap) {
    mWrapCursor = wrap;
}

void PatternModel::setCursorWrapPattern(bool wrap) {
    mWrapPattern = wrap;
}

trackerboy::Song* PatternModel::source() const {
    return mModule.song();
}

void PatternModel::setPatterns(int pattern, CursorChangeFlags &flags) {

    auto song = source();

    if (mShowPreviews) {
        setPreviewPatterns(pattern);
    }

    // update the current pattern
    auto oldsize = mPatternCurr.totalRows();
    mPatternCurr = song->getPattern(pattern);
    auto newsize = mPatternCurr.totalRows();

    if (oldsize != newsize) {
        emit patternSizeChanged(newsize);
    }

    emit invalidated();

    if (mCursor.row >= newsize) {
        mCursor.row = newsize - 1;
        flags |= CursorRowChanged;
    }
}

void PatternModel::emitIfChanged(CursorChangeFlags flags) {
    if (flags) {
        emit cursorChanged(flags);
    }
}

void PatternModel::setPreviewPatterns(int pattern) {
    auto song = source();
    // get the previous pattern for preview
    if (pattern > 0) {
        mPatternPrev.emplace(song->getPattern(pattern - 1));
    } else {
        mPatternPrev.reset();
    }
    // get the next pattern for preview
    auto nextPattern = pattern + 1;
    if (nextPattern < patterns()) {
        mPatternNext.emplace(song->getPattern(nextPattern));
    } else {
        mPatternNext.reset();
    }
}

int PatternModel::cursorEffectNo() {
    return (mCursor.column - PatternCursor::ColumnEffect1Type) / 3;
}

void PatternModel::invalidate(int pattern, bool updatePatterns) {

    // check if the pattern being invalidated is accessible
    bool isInvalid = (mCursorPattern == pattern) ||
                     (mPatternPrev && pattern == mCursorPattern - 1) ||
                     (mPatternNext && pattern == mCursorPattern + 1);

    if (isInvalid) {
        // pattern is now invalid, either reset the pattern accessors
        // or send the invalidated signal
        if (updatePatterns) {
            // reset pattern accessors and invalidate
            CursorChangeFlags flags = CursorUnchanged;
            setPatterns(mCursorPattern, flags);
            emitIfChanged(flags);
        } else {
            // views just need to redraw
            emit invalidated();
        }
    }

}

bool PatternModel::selectionDataIsEmpty() {
    if (mHasSelection) {
        auto iter = mSelection.iterator();

        for (auto track = iter.trackStart(); track <= iter.trackEnd(); ++track) {
            auto tmeta = iter.getTrackMeta(track);
            for (auto row = iter.rowStart(); row <= iter.rowEnd(); ++row) {
                auto const& rowdata = mPatternCurr.getTrackRow(static_cast<trackerboy::ChType>(track), (uint16_t)row);
                if (tmeta.hasColumn<PatternAnchor::SelectNote>()) {
                    if (rowdata.queryNote()) {
                        return false;
                    }
                }

                if (tmeta.hasColumn<PatternAnchor::SelectInstrument>()) {
                    if (rowdata.queryInstrument()) {
                        return false;
                    }
                }

                if (tmeta.hasColumn<PatternAnchor::SelectEffect1>()) {
                    if (rowdata.queryEffect(0)) {
                        return false;
                    }
                }

                if (tmeta.hasColumn<PatternAnchor::SelectEffect2>()) {
                    if (rowdata.queryEffect(1)) {
                        return false;
                    }
                }

                if (tmeta.hasColumn<PatternAnchor::SelectEffect3>()) {
                    if (rowdata.queryEffect(2)) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

trackerboy::TrackRow const& PatternModel::cursorTrackRow() {
    return mPatternCurr.getTrackRow(
        static_cast<trackerboy::ChType>(mCursor.track),
        (uint16_t)mCursor.row
    );
}

void PatternModel::setMaxColumns() {
    auto counts = source()->effectCounts();
    int sum = 0;
    for (size_t i = 0; i < counts.size(); ++i) {
        int max = (counts[i] * 3) + 3;
        mMaxColumns[i] = max;
        sum += max;
    }
    emit totalColumnsChanged(sum);
}

// editing ====================================================================

void PatternModel::setNote(std::optional<uint8_t> note, std::optional<uint8_t> instrument) {
        
    auto &rowdata = cursorTrackRow();
    auto oldNote = rowdata.queryNote();
    auto oldInstrument = rowdata.queryInstrument();

    auto const editNote = oldNote != note;
    if (editNote && note.has_value() && *note == trackerboy::NOTE_CUT) {
        // don't set the instrument for note cuts
        instrument.reset();
    }
    // edit the instrument if the instrument has a value and the it does not equal the current instrument
    auto const editInstrument = instrument && oldInstrument != instrument;
    int editCount = editNote + editInstrument;
    if (editCount) {
        QUndoCommand *parent = nullptr;
        QUndoCommand *cmd = nullptr;

        if (editCount == 2) {
            parent = new QUndoCommand;
        }

        if (editNote) {
            cmd = new NoteEditCmd(
                *this,
                trackerboy::TrackRow::convertColumn(note),
                trackerboy::TrackRow::convertColumn(oldNote),
                parent
            );
        }

        if (editInstrument) {
            cmd = new InstrumentEditCmd(
                *this,
                trackerboy::TrackRow::convertColumn(instrument),
                trackerboy::TrackRow::convertColumn(oldInstrument),
                parent
            );
        }

        if (parent) {
            cmd = parent;
        }

        if (note) {
            cmd->setText(tr("Note entry")); // todo: put the pattern, row, and track in this text
        } else {
            cmd->setText(tr("Clear note"));
        }
        mModule.undoStack()->push(cmd);

        
    }


}

void PatternModel::setInstrument(std::optional<uint8_t> nibble) {
    auto &rowdata = cursorTrackRow();
    auto oldInstrument = rowdata.queryInstrument();
    std::optional<uint8_t> newInstrument;
    if (nibble) {
        bool const highNibble = mCursor.column == PatternCursor::ColumnInstrumentHigh;
        newInstrument = replaceNibble(oldInstrument.value_or((uint8_t)0), *nibble, highNibble);
        if (*newInstrument >= trackerboy::InstrumentTable::MAX_SIZE) {
            return;
        }
    }

    if (newInstrument != oldInstrument) {
        auto cmd = new InstrumentEditCmd(
            *this,
            trackerboy::TrackRow::convertColumn(newInstrument),
            trackerboy::TrackRow::convertColumn(oldInstrument)
        );
        if (newInstrument) {
            cmd->setText(tr("set instrument"));
        } else {
            cmd->setText(tr("clear instrument"));
        }
        mModule.undoStack()->push(cmd);
    }
}

void PatternModel::setEffectType(trackerboy::EffectType type) {
    auto effectNo = cursorEffectNo();
    auto &rowdata = cursorTrackRow();
    auto &effect = rowdata.effects[effectNo];
    if (effect.type != type) {
        auto cmd = new EffectTypeEditCmd(
            *this,
            (uint8_t)effectNo,
            static_cast<uint8_t>(type),
            static_cast<uint8_t>(effect.type)
        );

        auto stack = mModule.undoStack();
        if (type == trackerboy::EffectType::noEffect) {

            static auto CLEAR_EFFECT_STR = QT_TR_NOOP("clear effect");

            // we also need to clear the parameter
            if (effect.param != 0) {
                stack->beginMacro(tr(CLEAR_EFFECT_STR));
                stack->push(cmd);
                stack->push(new EffectParamEditCmd(
                    *this, (uint8_t)effectNo, 0, effect.param
                ));
                stack->endMacro();
            } else {
                cmd->setText(tr(CLEAR_EFFECT_STR));
                stack->push(cmd);
            }
            
        } else {
            cmd->setText(tr("set effect type"));
            stack->push(cmd);
        }
    }

}

void PatternModel::setEffectParam(uint8_t nibble) {
    auto effectNo = cursorEffectNo();
    auto &rowdata = cursorTrackRow();

    auto &oldEffect = rowdata.effects[effectNo];
    if (oldEffect.type != trackerboy::EffectType::noEffect) {
        bool isHighNibble = mCursor.column == PatternCursor::ColumnEffect1ArgHigh ||
                            mCursor.column == PatternCursor::ColumnEffect2ArgHigh ||
                            mCursor.column == PatternCursor::ColumnEffect3ArgHigh;
        auto newParam = replaceNibble(oldEffect.param, nibble, isHighNibble);
        if (newParam != oldEffect.param) {
            auto cmd = new EffectParamEditCmd(
                *this,
                (uint8_t)effectNo,
                newParam,
                oldEffect.param
            );
            cmd->setText(tr("edit effect parameter"));
            mModule.undoStack()->push(cmd);
        }
    }
        
}

void PatternModel::deleteSelection() {

    if (hasSelection()) {
        // check if the selection actually has data
        if (!selectionDataIsEmpty()) {
            auto cmd = new EraseCmd(*this);
            cmd->setText(tr("Clear selection"));
            mModule.undoStack()->push(cmd);
        }
    } else {
        switch (mCursor.column) {
            case PatternCursor::ColumnNote: {
                // four possibilities
                // 1. just the note is set          -> erase the note
                // 2. just the instrument is set    -> erase the instrument
                // 3. both are set (use a macro)    -> erase both (macro)
                // 4. neither are set               -> do nothing

                auto &rowdata = cursorTrackRow();
                auto oldNote = rowdata.queryNote();
                auto oldInstrument = rowdata.queryInstrument();

                QUndoCommand *noteClear = nullptr;
                QUndoCommand *instClear = nullptr;

                if (oldNote.has_value()) {
                    noteClear = new NoteEditCmd(*this, trackerboy::TrackRow::convertColumn({}), trackerboy::TrackRow::convertColumn(oldNote));
                }

                if (oldInstrument.has_value()) {
                    instClear = new InstrumentEditCmd(*this, trackerboy::TrackRow::convertColumn({}), trackerboy::TrackRow::convertColumn(oldInstrument));
                }

                auto getCommandText = []() { return tr("Clear note"); };

                auto undoStack = mModule.undoStack();
                if (noteClear && instClear) {
                    // 3
                    undoStack->beginMacro(getCommandText());
                    undoStack->push(noteClear);
                    undoStack->push(instClear);
                    undoStack->endMacro();
                } else if (noteClear) {
                    // 1
                    noteClear->setText(getCommandText());
                    undoStack->push(noteClear);
                } else if (instClear) {
                    // 2
                    instClear->setText(getCommandText());
                    undoStack->push(instClear);
                } // 4

                break;
            }
            case PatternCursor::ColumnInstrumentHigh:
            case PatternCursor::ColumnInstrumentLow:
                setInstrument({});
                break;
            default:
                // deleting an effect parameter or type deletes the entire effect
                setEffectType(trackerboy::EffectType::noEffect);
                break;
        }
    }
    
}

void PatternModel::transpose(int amount) {

    if (amount) { // a transpose of 0 does nothing
        if (hasSelection()) {
            auto cmd = new TransposeCmd(*this, (int8_t)amount);
            cmd->setText(tr("transpose selection"));
            mModule.undoStack()->push(cmd);
        } else {
            auto &rowdata = cursorTrackRow();
            auto rowcopy = rowdata;
            rowcopy.transpose(amount);
            // if the transpose resulted in a change, create and push an edit command
            if (rowcopy.note != rowdata.note) {
                auto cmd = new NoteEditCmd(*this, rowcopy.note, rowdata.note);
                cmd->setText(tr("transpose note"));
                mModule.undoStack()->push(cmd);
            }

        }
    }
}

void PatternModel::reverse() {
    if (mHasSelection) {
        auto iter = mSelection.iterator();

        if (iter.rows() > 1) {
            auto cmd = new ReverseCmd(*this);
            cmd->setText("reverse");
            mModule.undoStack()->push(cmd);
        }
    }
}

void PatternModel::replaceInstrument(int instrument) {
    Q_ASSERT(instrument >= 0 && instrument < 64);
    if (mHasSelection) {
        auto cmd = new ReplaceInstrumentCmd(*this, instrument);
        cmd->setText(tr("replace instrument in selection"));
        mModule.undoStack()->push(cmd);
    } else {
        auto &rowdata = cursorTrackRow();
        auto newinstrument = trackerboy::TrackRow::convertColumn((uint8_t)instrument);
        if (rowdata.queryInstrument().has_value() && newinstrument != rowdata.instrumentId) {
            auto cmd = new InstrumentEditCmd(
                *this,
                newinstrument,
                rowdata.instrumentId
                );
            cmd->setText(tr("replace instrument"));
            mModule.undoStack()->push(cmd);
        }
    }

}

void PatternModel::moveSelection(PatternCursor pos) {
    if (mHasSelection) {
        auto undoStack = mModule.undoStack();

        undoStack->beginMacro(tr("move selection"));
        auto toMove = clip();
        undoStack->push(new EraseCmd(*this));
        undoStack->push(new PasteCmd(*this, toMove, pos, false));
        undoStack->endMacro();

        mSelection.moveTo(pos);
        mSelection.clamp((int)mPatternCurr.totalRows() - 1);
        emit selectionChanged();
    }
}

void PatternModel::paste(PatternClip const& clip, bool mix) {
    auto cmd = new PasteCmd(*this, clip, mCursor, mix);
    cmd->setText(tr("paste"));
    mModule.undoStack()->push(cmd);
}

void PatternModel::backspace() {
    if (mCursor.row > 0) {
        auto nextRow = mCursor.row - 1;
        auto cmd = new BackspaceCmd(*this);
        cmd->setText(tr("backspace"));
        mModule.undoStack()->push(cmd);
        setCursorRow(nextRow);
    }
}

void PatternModel::insertRow() {
    auto cmd = new InsertRowCmd(*this);
    cmd->setText(tr("insert row"));
    mModule.undoStack()->push(cmd);

}

bool PatternModel::canGrowOrShrink() const {
    return mHasSelection && mSelection.iterator().rows() > 2;
}


void PatternModel::growPattern() {
    if (canGrowOrShrink()) {
        auto cmd = new GrowCmd(*this);
        cmd->setText(tr("grow pattern"));
        mModule.undoStack()->push(cmd);
    }
}

void PatternModel::shrinkPattern() {
    if (canGrowOrShrink()) {
        auto cmd = new ShrinkCmd(*this);
        cmd->setText(tr("shrink pattern"));
        mModule.undoStack()->push(cmd);
    }
}



void PatternModel::setOrderRow(trackerboy::OrderRow row) {
    if (order()[mCursorPattern] != row) {
        auto cmd = new OrderEditCmd(*this, row, mCursorPattern);
        cmd->setText(tr("edit order #%1").arg(mCursorPattern));
        mModule.undoStack()->push(cmd);
    }
}

void PatternModel::insertOrder() {
    auto cmd = new OrderInsertCmd(*this, mCursorPattern);
    cmd->setText(tr("insert order #%1").arg(mCursorPattern));
    mModule.undoStack()->push(cmd);
}

void PatternModel::removeOrder() {
    auto cmd = new OrderRemoveCmd(*this, mCursorPattern);
    cmd->setText(tr("remove order #%1").arg(mCursorPattern));
    mModule.undoStack()->push(cmd);
}

void PatternModel::duplicateOrder() {
    auto cmd = new OrderDuplicateCmd(*this, mCursorPattern);
    cmd->setText(tr("duplicate order #%1").arg(mCursorPattern));
    mModule.undoStack()->push(cmd);
}

void PatternModel::moveOrderUp() {
    auto cmd = new OrderSwapCmd(*this, mCursorPattern, mCursorPattern - 1);
    cmd->setText((tr("move order up")));
    mModule.undoStack()->push(cmd);
}

void PatternModel::moveOrderDown() {
    auto cmd = new OrderSwapCmd(*this, mCursorPattern, mCursorPattern + 1);
    cmd->setText((tr("move order down")));
    mModule.undoStack()->push(cmd);
}

void PatternModel::showEffect(int track) {
    addEffects(track, 1);
}

void PatternModel::hideEffect(int track) {
    auto trackCount = addEffects(track, -1);
    if (mCursor.track == track && mCursor.column >= mMaxColumns[track]) {
        // the cursor column is no longer reachable, move it the last available one
        setCursorColumn(mMaxColumns[track] - 1);
    }
    if (mHasSelection) {
        auto iter = mSelection.iterator();
        auto maxSelect = trackCount + PatternAnchor::SelectInstrument;
        if (iter.trackStart() == track
            && iter.columnStart() > maxSelect
            && iter.trackEnd() == track
            && iter.columnEnd() > maxSelect) {
                // this selection is no longer reachable, deselect it
                deselect();
            }
    }
}

int PatternModel::addEffects(int track, int effectsToAdd) {
    // assumption: effectsToAdd is never 0
    auto counts = source()->effectCounts();
    auto &trackCount = counts[track];
    trackCount += effectsToAdd;
    if (trackCount >= 1 && trackCount <= 3) {

        {
            // technically a permanent edit, but we don't want this change to
            // effect the document's modified state
            auto editor = mModule.edit();
            source()->setEffectCounts(counts);
        }
        emit effectsVisibleChanged();

        mMaxColumns[track] += effectsToAdd * 3;        
        emit totalColumnsChanged(totalColumns());
    }
    return trackCount;

}

void PatternModel::insertOrderImpl(const trackerboy::OrderRow &row, int before) {
    auto &_order = order();
    {
        auto editor = mModule.edit();
        _order.insert(before, row);
    }

    emit patternCountChanged(_order.size());
    if (mCursorPattern == before) {
        invalidate(before, true);
    } else {
        setCursorPattern(before);
    }

}

void PatternModel::removeOrderImpl(int at) {
    auto &_order = order();
    {
        auto editor = mModule.edit();
        if (at == _order.size() - 1) {
            emit aboutToRemoveLastPattern();
        }
        _order.remove(at);
    }

    auto count = _order.size();
    if (mCursorPattern >= count) {
        setCursorPattern(count - 1);
    } else {
        invalidate(at, true);
    }
    emit patternCountChanged(count);
}



#undef TU
