
#pragma once

#include "clipboard/PatternClipboard.hpp"
#include "config/data/Palette.hpp"
#include "config/data/PianoInput.hpp"
#include "midi/IMidiReceiver.hpp"
#include "widgets/grid/PatternGrid.hpp"
#include "widgets/grid/PatternGridHeader.hpp"

#include <QFrame>
#include <QScrollBar>

#include <cstdint>
#include <optional>

class QMenu;


class PatternEditor : public QFrame, public IMidiReceiver {

    Q_OBJECT

public:

    PatternEditor(
        PianoInput const& input,
        PatternModel &model,
        QWidget *parent = nullptr
    );
    virtual ~PatternEditor() = default;

    PatternGrid* grid();

    PatternGridHeader* gridHeader();

    void setColors(Palette const& colors);

    void setEditMenu(QMenu *menu);

    void setRownoHex(bool hex);

    void setPageStep(int pageStep);

    virtual void midiNoteOn(int note) override;

    virtual void midiNoteOff() override;

    void setEditStep(int step);

    void setInstrument(int id);

    void setKeyRepeat(bool repeat);

    void cut();

    void copy();

    void paste();

    void pasteMix();

    void insertRow();

    void erase();

    void selectAll();

    void increaseNote();

    void decreaseNote();

    void increaseOctave();

    void decreaseOctave();

    void transpose();

    void reverse();

    void replaceInstrument();

signals:
    void previewNote(int note, int track, int instrument);

    void stopNotePreview();

protected:

    virtual void contextMenuEvent(QContextMenuEvent *evt) override;

    virtual bool event(QEvent *evt) override;

    virtual void focusInEvent(QFocusEvent *evt) override;

    virtual void focusOutEvent(QFocusEvent *evt) override;

    virtual void keyPressEvent(QKeyEvent *evt) override;

    virtual void keyReleaseEvent(QKeyEvent *evt) override;

    virtual void wheelEvent(QWheelEvent *evt) override;

private:
    Q_DISABLE_COPY(PatternEditor)

    void hscrollAction(int action);
    void vscrollAction(int action);

    void updateScrollbars(PatternModel::CursorChangeFlags flags);
    void setCursorFromHScroll(int value);

    void pasteImpl(bool mix);

    void setTempoLabel(float tempo);
    float calcActualTempo(float speed);

    void stepDown();

    PianoInput const& mPianoIn;
    PatternModel &mModel;

    PatternGridHeader *mGridHeader;
    PatternGrid *mGrid;
    QScrollBar *mHScroll;
    QScrollBar *mVScroll;

    int mWheel;
    int mPageStep;
    int mEditStep;

    int mPreviewKey;
    
    bool mIgnoreCursorChanges;

    bool mKeyRepeat;

    PatternClipboard mClipboard;

    std::optional<uint8_t> mInstrument;

    QMenu *mEditMenu;


};
