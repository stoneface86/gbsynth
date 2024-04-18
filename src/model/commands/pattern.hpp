
#pragma once

class PatternModel;

#include "clipboard/PatternClip.hpp"

#include "trackerboy/data/TrackRow.hpp"

#include <QUndoCommand>

#include <cstdint>


//
// Base class for commands that operate on a PatternSelection
//
class SelectionCmd : public QUndoCommand {

protected:
    PatternModel &mModel;
    uint8_t mPattern;
    PatternClip mClip;

    //
    // initializes the command by saving a clip of the current selection
    //
    explicit SelectionCmd(PatternModel &model);

    //
    // Restores the saved clip
    //
    void restore(bool update);

};

//
// Command for erasing the contents of a selection
//
class EraseCmd : public SelectionCmd {

public:

    EraseCmd(PatternModel &model);

    virtual void redo() override;

    virtual void undo() override;

};

//
// Command for pasting pattern data
//
class PasteCmd : public QUndoCommand {

    PatternModel &mModel;
    PatternClip mSrc;
    PatternClip mPast;
    PatternCursor mPos;
    uint8_t mPattern;
    bool mMix;

public:
    PasteCmd(
        PatternModel &model,
        PatternClip const& clip,
        PatternCursor pos,
        bool mix
    );

    virtual void redo() override;

    virtual void undo() override;

};

//
// Command for reversing the contents of a selection. While reverse applies to
// a selection, we do not inherit from SelectionCmd, as its
// redo/undo actions are the same (reversing is an involutory function). So
// there is no need to save a chunk of the selection for undo'ing
//
class ReverseCmd : public QUndoCommand {

    PatternModel &mModel;
    PatternSelection mSelection;
    uint8_t mPattern;

public:

    explicit ReverseCmd(PatternModel &model);

    virtual void redo() override;

    virtual void undo() override;

private:

    void reverse();

};

//
// Command for replacing all selected instrument columns with the given instrument
// Empty instrument columns are ignored.
//
class ReplaceInstrumentCmd : public SelectionCmd {

    int const mInstrument;

public:

    explicit ReplaceInstrumentCmd(PatternModel &model, int instrument);

    virtual void redo() override;
    
    virtual void undo() override;
    

};

//
// Command to grow a pattern
//
class GrowCmd : public SelectionCmd {

public:
    explicit GrowCmd(PatternModel &model);

    virtual void redo() override;

    virtual void undo() override;

};

//
// Command to shrink a pattern
//
class ShrinkCmd : public SelectionCmd {

public:
    explicit ShrinkCmd(PatternModel &model);

    virtual void redo() override;

    virtual void undo() override;

};


//
// Base command class for editing a column in a track row
//
class TrackEditCmd : public QUndoCommand {

protected:
    PatternModel &mModel;
    uint8_t const mTrack;
    uint8_t const mPattern;
    uint8_t const mRow;
    uint8_t const mNewData;
    uint8_t const mOldData;

public:
    explicit TrackEditCmd(
        PatternModel &model,
        uint8_t dataNew,
        uint8_t dataOld,
        QUndoCommand *parent = nullptr
    );

    virtual void redo() override;

    virtual void undo() override;

protected:
    trackerboy::TrackRow& getRow();

    virtual bool edit(trackerboy::TrackRow &rowdata, uint8_t data) = 0;

private:
    void setData(uint8_t data);

};

//
// Command for editing the note column in a TrackRow
//
class NoteEditCmd : public TrackEditCmd {

    using TrackEditCmd::TrackEditCmd;

protected:
    virtual bool edit(trackerboy::TrackRow &rowdata, uint8_t data) override;
};

//
// Command for editing the instrument column in a TrackRow
//
class InstrumentEditCmd : public TrackEditCmd {

    using TrackEditCmd::TrackEditCmd;

protected:
    virtual bool edit(trackerboy::TrackRow &rowdata, uint8_t data);

};

//
// Base command class for editing an effect column
//
class EffectEditCmd : public TrackEditCmd {

public:
    explicit EffectEditCmd(
        PatternModel &model,
        uint8_t effectNo,
        uint8_t newData,
        uint8_t oldData,
        QUndoCommand *parent = nullptr
    );

protected:
    uint8_t const mEffectNo;
};

//
// Command class for editing an effect type in a TrackRow
//
class EffectTypeEditCmd : public EffectEditCmd {

    using EffectEditCmd::EffectEditCmd;

protected:
    virtual bool edit(trackerboy::TrackRow &rowdata, uint8_t data) override;

};

//
// Command class for editing an effect parameter in a TrackRow
//
class EffectParamEditCmd : public EffectEditCmd {

    using EffectEditCmd::EffectEditCmd;

protected:
    virtual bool edit(trackerboy::TrackRow &rowdata, uint8_t data) override;

};

//
// Command for transposing all notes in a selection by a given semitone offset.
//
class TransposeCmd : public SelectionCmd {

    int8_t mTransposeAmount; // semitones to transpose all notes by

public:

    explicit TransposeCmd(PatternModel &model, int8_t transposeAmount);

    virtual void redo() override;

    virtual void undo() override;

};

//
// Backspace command. Deletes the previous row in the track and shifts all rows
// below it up 1.
//
class BackspaceCmd : public QUndoCommand {

    PatternModel &mModel;
    int const mPattern;
    int const mTrack;
    int const mRow;
    trackerboy::TrackRow const mDeleted;

public:

    explicit BackspaceCmd(PatternModel &model, QUndoCommand *parent = nullptr);

    virtual void redo() override;

    virtual void undo() override;

};

class InsertRowCmd : public QUndoCommand {

    PatternModel &mModel;
    int const mPattern;
    int const mTrack;
    int const mRow;
    int const mLastRow;
    trackerboy::TrackRow const mTruncated;

public:
    explicit InsertRowCmd(PatternModel &model, QUndoCommand *parent = nullptr);

    virtual void redo() override;

    virtual void undo() override;
};
