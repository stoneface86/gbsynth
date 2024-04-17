
#include "widgets/PatternEditor.hpp"
#include "utils/utils.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtDebug>

#define TU PatternEditorTU
namespace TU {

static std::optional<trackerboy::EffectType> keyToEffectType(int const key) {
    switch (key) {
        case Qt::Key_B:
            return trackerboy::EffectType::patternGoto;
        case Qt::Key_C:
            return trackerboy::EffectType::patternHalt;
        case Qt::Key_D:
            return trackerboy::EffectType::patternSkip;
        case Qt::Key_F:
            return trackerboy::EffectType::setTempo;
        case Qt::Key_T:
            return trackerboy::EffectType::sfx;
        case Qt::Key_E:
            return trackerboy::EffectType::setEnvelope;
        case Qt::Key_V:
            return trackerboy::EffectType::setTimbre;
        case Qt::Key_I:
            return trackerboy::EffectType::setPanning;
        case Qt::Key_J:
            return trackerboy::EffectType::setGlobalVolume;
        case Qt::Key_H:
            return trackerboy::EffectType::setSweep;
        case Qt::Key_S:
            return trackerboy::EffectType::delayedCut;
        case Qt::Key_G:
            return trackerboy::EffectType::delayedNote;
        case Qt::Key_L:
            return trackerboy::EffectType::lock;
        case Qt::Key_0:
            return trackerboy::EffectType::arpeggio;
        case Qt::Key_1:
            return trackerboy::EffectType::pitchUp;
        case Qt::Key_2:
            return trackerboy::EffectType::pitchDown;
        case Qt::Key_3:
            return trackerboy::EffectType::autoPortamento;
        case Qt::Key_4:
            return trackerboy::EffectType::vibrato;
        case Qt::Key_5:
            return trackerboy::EffectType::vibratoDelay;
        case Qt::Key_P:
            return trackerboy::EffectType::tuning;
        case Qt::Key_Q:
            return trackerboy::EffectType::noteSlideUp;
        case Qt::Key_R:
            return trackerboy::EffectType::noteSlideDown;
        case Qt::Key_Delete:
            return trackerboy::EffectType::noEffect;
        default:
            return std::nullopt;
    }
}

}

// PatternEditor is just a composite widget containing the grid, header and
// two scrollbars. (PatternGrid does most of the work)
//
// +-------------------------------------+
// | PatternGridHeader                   |
// |                                     |
// +----------------------------------+--+
// |                                  |  |
// |                                  |  |
// |                                  |  |
// |                                  |  |
// | PatternGrid                      |  | <--- VScrollBar
// |                                  |  |
// |                                  |  |
// |                                  |  |
// |                                  |  |
// +----------------------------------+--+
// | HScrollBar                       |XX|
// +----------------------------------+--+


PatternEditor::PatternEditor(
    PianoInput const& input,
    PatternModel &model,
    QWidget *parent
) :
    QFrame(parent),
    mPianoIn(input),
    mModel(model),
    mWheel(0),
    mPageStep(4),
    mEditStep(1),
    mIgnoreCursorChanges(false),
    mKeyRepeat(true),
    mClipboard(),
    mInstrument(),
    mEditMenu(nullptr)
{
    setFrameStyle(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);

    auto layout = new QGridLayout;
    mGridHeader = new PatternGridHeader(model, this);
    mGrid = new PatternGrid(*mGridHeader, model, this);
    mHScroll = new QScrollBar(Qt::Horizontal);
    mVScroll = new QScrollBar(Qt::Vertical);


    mHScroll->setMinimum(0);
    mHScroll->setMaximum(model.totalColumns() - 1);
    mHScroll->setPageStep(1);

    layout->addWidget(mGridHeader,     0, 0);
    layout->addWidget(mGrid,           1, 0);
    layout->addWidget(mVScroll,        0, 1, 2, 1);
    layout->addWidget(mHScroll,        2, 0);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // connections

    // scrollbars
    mVScroll->setMaximum((int)model.currentPattern().totalRows() - 1);
    updateScrollbars(PatternModel::CursorRowChanged | PatternModel::CursorColumnChanged);
    connect(mVScroll, &QScrollBar::valueChanged, &model, &PatternModel::setCursorRow);
    connect(mHScroll, &QScrollBar::valueChanged, this, &PatternEditor::setCursorFromHScroll);
    connect(&model, &PatternModel::cursorChanged, this, &PatternEditor::updateScrollbars);

    connect(&model, &PatternModel::patternSizeChanged, this,
        [this](int rows) {
            mVScroll->setMaximum(rows - 1);
        });
    connect(&model, &PatternModel::totalColumnsChanged, this,
        [this](int columns) {
            mHScroll->setMaximum(columns - 1);
        });

    connect(mVScroll, &QScrollBar::actionTriggered, this, &PatternEditor::vscrollAction);
    connect(mHScroll, &QScrollBar::actionTriggered, this, &PatternEditor::hscrollAction);
}

PatternGrid* PatternEditor::grid() {
    return mGrid;
}

PatternGridHeader* PatternEditor::gridHeader() {
    return mGridHeader;
}

void PatternEditor::setColors(Palette const& colors) {
    mGridHeader->setColors(colors);
    mGrid->setColors(colors);
}

void PatternEditor::setEditMenu(QMenu *menu) {
    mEditMenu = menu;
}

void PatternEditor::setPageStep(int pageStep) {
    mPageStep = pageStep;
}

void PatternEditor::setRownoHex(bool hex) {
    mGrid->setRownoHex(hex);
    mGridHeader->update();
}

void PatternEditor::contextMenuEvent(QContextMenuEvent *evt) {
    // only show the menu if the user right-clicked on the grid
    if (childAt(evt->pos()) == mGrid) {
        auto menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        // add the actions of the Edit menu to this menu
        if (mEditMenu) {
            menu->addActions(mEditMenu->actions());
        }
        
        // check if the mouse is under a track
        auto track = mGrid->trackFromMouse(evt->pos());
        if (track >= 0 && track <= 3) {
            // yes, so add toggle/solo track actions (same as PatternGridHeader)
            menu->addSeparator();
            QAction *act;
            act = menu->addAction(PatternGridHeader::toggleChannelString(track + 1));
            connect(act, &QAction::triggered, this,
                [this, track]() {
                    mGridHeader->toggleTrack(track);
                });
            act = menu->addAction(PatternGridHeader::soloChannelString(track + 1));
            connect(act, &QAction::triggered, this,
                [this, track]() {
                    mGridHeader->soloTrack(track);
                });
        }

        menu->popup(evt->globalPos());
    }
}

bool PatternEditor::event(QEvent *evt)  {
    if (evt->type() == QEvent::KeyPress) {
        auto keyEvt = static_cast<QKeyEvent*>(evt);
        auto key = keyEvt->key();

        // intercept tabs for pattern navigation
        if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
            // intercept tab presses
            int amount = key == Qt::Key_Backtab ? -1 : 1;
            mModel.moveCursorTrack(amount);
            return true;
        }
    }

    return QWidget::event(evt);
}

void PatternEditor::focusInEvent(QFocusEvent *evt) {
    Q_UNUSED(evt)
    mGrid->setEditorFocus(true);
}

void PatternEditor::focusOutEvent(QFocusEvent *evt) {
    Q_UNUSED(evt)
    mGrid->setEditorFocus(false);
}

void PatternEditor::keyPressEvent(QKeyEvent *evt) {

    auto const modifiers = evt->modifiers();
    int const key = evt->key();


    // ignore this event if CTRL is present (conflicts with shortcuts)
    if (modifiers.testFlag(Qt::ControlModifier)) {
        QWidget::keyPressEvent(evt);
        return;
    }

    auto selectionMode = modifiers.testFlag(Qt::ShiftModifier) ?
                         PatternModel::SelectionModify : // shift is down, modify the selection
                         PatternModel::SelectionRemove;  // no shift, remove the selection

    // navigation keys
    // Up/Down/Left/Right - move cursor by 1 (also selects if shift is down)
    // PgUp/PgDn - move cursor by page step (also selects if shift is down)
    // Tab/Shift+Tab - move cursor to next/previous track (not handled here)
    
    // navigation keys / non-edit keys
    // these keys also ignore the key repetition setting (they always repeat)
    switch (key) {
        case Qt::Key_Left:
            mModel.moveCursorColumn(-1, selectionMode);
            return;
        case Qt::Key_Right:
            mModel.moveCursorColumn(1, selectionMode);
            return;
        case Qt::Key_Up:
            mModel.moveCursorRow(-1, selectionMode);
            return;
        case Qt::Key_Down:
            mModel.moveCursorRow(1, selectionMode);
            return;
        case Qt::Key_PageDown:
            mModel.moveCursorRow(mPageStep, selectionMode);
            return;
        case Qt::Key_PageUp:
            mModel.moveCursorRow(-mPageStep, selectionMode);
            return;
    }

    if (evt->isAutoRepeat() && !mKeyRepeat) {
        QWidget::keyPressEvent(evt);
        return; // key repetition disabled, ignore this event
    }

    auto const recording = mModel.isRecording();
    bool validKey = false;

    if (key == Qt::Key_Backspace) {
        if (recording) {
            mModel.backspace();
        }
        return;
    }

    switch (mModel.cursorColumn()) {
        case PatternCursor::ColumnNote: {
            auto note = mPianoIn.keyToNote(key);
            
            if (note) {
                if (*note != trackerboy::NOTE_CUT) {
                    if (mPreviewKey != key) {
                        mPreviewKey = key;
                        emit previewNote(*note, mModel.cursorTrack(), mInstrument.value_or(-1));
                    }
                }
                if (recording) {
                    mModel.setNote(note, mInstrument);
                }
                validKey = true;
            }

            break;
        }
        case PatternCursor::ColumnEffect1Type:
        case PatternCursor::ColumnEffect2Type:
        case PatternCursor::ColumnEffect3Type:
        {
            // check if the key pressed is a valid effect type
            auto effectType = TU::keyToEffectType(key);
            if (effectType) {
                if (recording) {
                    mModel.setEffectType(*effectType);
                }
                validKey = true;
            }
            break;
        }
        case PatternCursor::ColumnInstrumentHigh:
        case PatternCursor::ColumnInstrumentLow: {
            auto hex = keyToHex(key);
            if (hex) {
                if (recording) {
                    mModel.setInstrument(hex);
                }
                validKey = true;
            }
            break;
        }
        case PatternCursor::ColumnEffect1ArgHigh:
        case PatternCursor::ColumnEffect1ArgLow:
        case PatternCursor::ColumnEffect2ArgHigh:
        case PatternCursor::ColumnEffect2ArgLow:
        case PatternCursor::ColumnEffect3ArgHigh:
        case PatternCursor::ColumnEffect3ArgLow:
        {
            // check if the key pressed is a hex number
            auto hex = keyToHex(key);
            if (hex) {
                if (recording) {
                    mModel.setEffectParam(*hex);
                }
                validKey = true;
            }
            
        }
        break;
    }


    if (validKey) {
        if (recording) {
            stepDown();
        }
    } else {
        // invalid key or edit mode is off, let QWidget handle it
        QWidget::keyPressEvent(evt);
    }
    
}

void PatternEditor::keyReleaseEvent(QKeyEvent *evt) {
    if (!evt->isAutoRepeat() && evt->key() == mPreviewKey) {
        mPreviewKey = Qt::Key_unknown;
        emit stopNotePreview();
    }
}

void PatternEditor::wheelEvent(QWheelEvent *evt) {
    mWheel += evt->angleDelta().y();
    int amount = 0;
    // 120 / 8 = 15 degrees
    if (mWheel >= 120) {
        mWheel -= 120;
        amount = -mPageStep;
    } else if (mWheel <= -120) {
        mWheel += 120;
        amount = mPageStep;
    }

    if (amount) {
        mModel.moveCursorRow(amount);
    }

    evt->accept();
}

void PatternEditor::hscrollAction(int action) {
    switch (action) {
        case QAbstractSlider::SliderSingleStepAdd:
            mModel.moveCursorColumn(1);
            break;
        case QAbstractSlider::SliderSingleStepSub:
            mModel.moveCursorColumn(-1);
            break;
        default:
            break;
    }
}

void PatternEditor::vscrollAction(int action) {

    switch (action) {
        case QAbstractSlider::SliderSingleStepAdd:
            mModel.moveCursorRow(1);
            break;
        case QAbstractSlider::SliderSingleStepSub:
            mModel.moveCursorRow(-1);
            break;
        default:
            break;
    }
}

void PatternEditor::setEditStep(int step) {
    mEditStep = step;
}

void PatternEditor::setInstrument(int id) {
    if (id == -1) {
        mInstrument.reset();
    } else {
        mInstrument = (uint8_t)id;
    }
}

void PatternEditor::setKeyRepeat(bool repeat) {
    mKeyRepeat = repeat;
}

void PatternEditor::cut() {
    copy();
    mModel.deleteSelection();
}

void PatternEditor::copy() {
    auto clip = mModel.clip();
    mClipboard.setClip(clip);
}

void PatternEditor::paste() {
    pasteImpl(false);
}

void PatternEditor::pasteMix() {
    pasteImpl(true);
}

void PatternEditor::insertRow() {
    mModel.insertRow();
}

void PatternEditor::pasteImpl(bool mix) {
    if (mClipboard.hasClip()) {
        auto &clip = mClipboard.clip();
        mModel.paste(clip, mix);
    }
}

void PatternEditor::erase() {

    mModel.deleteSelection();
    if (!mModel.hasSelection()) {
        stepDown();
    }
}

void PatternEditor::selectAll() {
    mModel.selectAll();
}

void PatternEditor::increaseNote() {
    mModel.transpose(1);
}

void PatternEditor::decreaseNote() {
    mModel.transpose(-1);
}

void PatternEditor::increaseOctave() {
    mModel.transpose(12);
}

void PatternEditor::decreaseOctave() {
    mModel.transpose(-12);
}

void PatternEditor::transpose() {
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(tr("Transpose"));

    QVBoxLayout layout;
        QLabel label(tr("Enter transpose amount (semitones):"));
        QSpinBox transposeSpin;
        QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    
    layout.addWidget(&label);
    layout.addWidget(&transposeSpin);
    layout.addWidget(&buttons);
    layout.setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
    dialog.setLayout(&layout);

    transposeSpin.setRange(-trackerboy::NOTE_LAST, trackerboy::NOTE_LAST);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        mModel.transpose(transposeSpin.value());
    }
}

void PatternEditor::reverse() {
    mModel.reverse();
}

void PatternEditor::replaceInstrument() {
    if (mInstrument.has_value()) {
        mModel.replaceInstrument(*mInstrument);
    }
}

void PatternEditor::stepDown() {
    mModel.moveCursorRow(mEditStep);
}

void PatternEditor::updateScrollbars(PatternModel::CursorChangeFlags flags) {
    if (mIgnoreCursorChanges) {
        // we are currently changing the cursor, ignore updates from the model
        // as it is being changed.
        return;
    }

    if (flags.testFlag(PatternModel::CursorRowChanged)) {
        QSignalBlocker blocker(mVScroll);
        mVScroll->setValue(mModel.cursorRow());
    }

    if (flags & (PatternModel::CursorTrackChanged | PatternModel::CursorColumnChanged)) {
        QSignalBlocker blocker(mHScroll);
        mHScroll->setValue(mModel.cursorAbsoluteColumn());
    }

}

void PatternEditor::setCursorFromHScroll(int value) {

    mIgnoreCursorChanges = true;
    mModel.setCursorAbsoluteColumn(value);
    mIgnoreCursorChanges = false;

}

void PatternEditor::midiNoteOn(int note) {
    if (mModel.isRecording()) {
        mModel.setNote((uint8_t)note, mInstrument);
        stepDown();
    }

    emit previewNote(note, mModel.cursorTrack(), mInstrument.value_or(-1));

}

void PatternEditor::midiNoteOff() {
    emit stopNotePreview();
}

#undef TU
