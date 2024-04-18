
#include "config/tabs/KeyboardConfigTab.hpp"

#include "config/data/PianoInput.hpp"
#include "config/data/ShortcutTable.hpp"
#include "core/NoteStrings.hpp"
#include "utils/connectutils.hpp"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTreeView>
#include <QVBoxLayout>

#include <QtDebug>

#include <algorithm>


#define TU KeyboardConfigTabTU
namespace TU {

// human readable names for each shortcut in ShortcutTable
static std::array const SHORTCUT_NAMES = {
    QT_TR_NOOP("Previous instrument"),
    QT_TR_NOOP("Next instrument"),
    QT_TR_NOOP("Previous pattern"),
    QT_TR_NOOP("Next pattern"),
    QT_TR_NOOP("Increment octave"),
    QT_TR_NOOP("Decrement octave"),
    QT_TR_NOOP("Play / Stop"),
    QT_TR_NOOP("Mix paste"),
    QT_TR_NOOP("Insert row"),
    QT_TR_NOOP("Transpose, decrease note"),
    QT_TR_NOOP("Transpose, increase note"),
    QT_TR_NOOP("Transpose, decrease octave"),
    QT_TR_NOOP("Transpose, increase octave"),
    QT_TR_NOOP("Transpose..."),
    QT_TR_NOOP("Reverse"),
    QT_TR_NOOP("Replace instrument"),
    QT_TR_NOOP("Grow pattern"),
    QT_TR_NOOP("Shrink pattern"),
    QT_TR_NOOP("Play"),
    QT_TR_NOOP("Play from start"),
    QT_TR_NOOP("Play from cursor"),
    QT_TR_NOOP("Step from cursor"),
    QT_TR_NOOP("Stop"),
    QT_TR_NOOP("Pattern repeat"),
    QT_TR_NOOP("Record"),
    QT_TR_NOOP("Toggle channel output"),
    QT_TR_NOOP("Solo channel"),
    QT_TR_NOOP("Kill sound"),
    QT_TR_NOOP("Edit instrument"),
    QT_TR_NOOP("Edit waveform"),
    QT_TR_NOOP("Module properties..."),
    QT_TR_NOOP("Show Effects List..."),
    QT_TR_NOOP("Show User Manual...")
};
static_assert(SHORTCUT_NAMES.size() == ShortcutTable::Count, "table size mismatch");
}

//
// QWidget subclass for editing PianoInput::Bindings
//
class BindingEdit : public QFrame {

    Q_OBJECT

    static constexpr int GRID_COLUMNS = 13;
    static constexpr int GRID_ROWS = 6;
    static constexpr int GRID_PADDING = 2;

    // specials (just note cut) is located at row 4, column 11
    // if more are added we will probably arrange them in a 2 column grid
    static constexpr int SPECIAL_ROW_START = 4;


public:

    explicit BindingEdit(QWidget *parent = nullptr) :
        QFrame(parent),
        mCursorRow(0),
        mCursorColumn(0)
    {
        setFocusPolicy(Qt::StrongFocus);
        setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
        setLineWidth(1);

        auto pal = palette();
        auto const bgrole = backgroundRole();
        auto const fgrole = foregroundRole();
        pal.setColor(QPalette::Active, fgrole, Qt::black);
        pal.setColor(QPalette::Disabled, fgrole, Qt::gray);
        pal.setColor(QPalette::Active, bgrole, Qt::white);
        pal.setColor(QPalette::Disabled, bgrole, Qt::white);
        setPalette(pal);


        determineMinHeight();
    }

    PianoInput::Bindings const& bindings() const {
        return mBindings;
    }

    void setBindings(PianoInput::Bindings const& bindings) {
        mBindings = bindings;
        update();
    }

signals:
    void bindingsChanged();

protected:

    virtual void changeEvent(QEvent *evt) override {
        if (evt->type() == QEvent::FontChange) {
            determineMinHeight();
        }
    }

    virtual void keyPressEvent(QKeyEvent *evt) override {
        auto key = evt->key();
        switch (key) {
            case Qt::Key_Left:
                moveCursorColumn(-1);
                return;
            case Qt::Key_Right:
                moveCursorColumn(1);
                return;
            case Qt::Key_Up:
                moveCursorRow(-1);
                return;
            case Qt::Key_Down:
                moveCursorRow(1);
                return;
            case Qt::Key_Delete:
                if (evt->modifiers() & Qt::ShiftModifier) {
                    key = 0;
                    break;
                }
                [[fallthrough]];
            case Qt::Key_Shift:
            case Qt::Key_Control:
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
            case Qt::Key_Alt:
            case Qt::Key_AltGr:
            case Qt::Key_Meta:
                evt->ignore();
                return;
        }

        if (evt->isAutoRepeat()) {
            return;
        }

        setKey(key);

    }

    virtual void mousePressEvent(QMouseEvent *evt) override {
        if (evt->button() == Qt::LeftButton) {
            auto const pos = evt->position();
            auto const row = qRound( pos.y() / mRowHeight );
            auto const col = qRound( pos.x() / mColumnWidth );

            if ((row >= 1 && row <= 4 && col >= 1 && col <= 12) || (row == 5 && col == 12)) {
                mCursorRow = row - 1;
                mCursorColumn = col - 1;
                update();
            }
        }
    }

    virtual void paintEvent(QPaintEvent *evt) override {

        QPainter painter(this);

        QRect cell(GRID_PADDING, GRID_PADDING, mColumnWidth - GRID_PADDING * 2, mRowHeight - GRID_PADDING * 2);

        auto mergeCells = [this](int row, int col, int rows, int cols) {
            return QRect(
                col * mColumnWidth + GRID_PADDING,
                row * mRowHeight + GRID_PADDING,
                cols * mColumnWidth - GRID_PADDING * 2,
                rows * mRowHeight - GRID_PADDING * 2
            );
        };

        auto invertRect = [&painter](QRect const& rect) {
            auto mode = painter.compositionMode();
            painter.setCompositionMode(QPainter::RasterOp_NotDestination);
            painter.fillRect(rect, QColor(0, 0, 0, 0));
            painter.setCompositionMode(mode);
        };

        auto drawKeybind = [&painter, &cell](Qt::Key key) {
            painter.drawText(cell, Qt::AlignCenter, QKeySequence(key).toString());
        };

        // lines
        painter.drawLine(0, mRowHeight, width(), mRowHeight);

        // note header
        for (int i = 0; i < 12; ++i) {
            cell.translate(mColumnWidth, 0);

            auto notestr = NoteStrings::Sharps[i];
            painter.drawText(cell, Qt::AlignCenter, notestr);

            if (isEnabled() && notestr[1] == '#') {
                // invert the cell so it has a black background
                invertRect(cell);
            }
        }

        // octaves
        cell.moveTo(GRID_PADDING, mRowHeight + GRID_PADDING);
        for (auto octave : { 0, 1, 1, 2}) {
            painter.drawText(cell, Qt::AlignCenter, QStringLiteral("+%1").arg(octave));
            cell.translate(0, mRowHeight);
        }

        // note binds        
        cell.moveTo(mColumnWidth + GRID_PADDING, mRowHeight + GRID_PADDING);
        auto bindingsIter = mBindings.cbegin();
        for (auto r = 0; r < 4; ++r) {
            for (auto c = 0; c < 12; ++c) {
                drawKeybind(*bindingsIter++);
                cell.translate(mColumnWidth, 0);
            }
            cell.moveLeft(mColumnWidth + GRID_PADDING);
            cell.translate(0, mRowHeight);
        }


        // special binds
        cell.moveTo(mColumnWidth * 12 + GRID_PADDING, mRowHeight * 5 + GRID_PADDING);
        painter.drawText(mergeCells(5, 1, 1, 11), Qt::AlignVCenter | Qt::AlignRight, tr("Note cut: "));
        drawKeybind(mBindings[PianoInput::BindingNoteCut]);

        // cursor
        cell.moveTo(
            mColumnWidth * (mCursorColumn + 1) + GRID_PADDING,
            mRowHeight * (mCursorRow + 1) + GRID_PADDING
        );

        if (hasFocus()) {
            invertRect(cell);
        } else {
            painter.drawRect(cell);
        }



        QFrame::paintEvent(evt);
    }

    virtual void resizeEvent(QResizeEvent *evt) override {
        auto size = evt->size();

        mColumnWidth = size.width() / GRID_COLUMNS;
        mRowHeight = size.height() / GRID_ROWS;
    }

private:

    Qt::Key& cursorKey() {
        switch (mCursorRow) {
            case 0:
            case 1:
            case 2:
            case 3:
                return mBindings[mCursorRow * 12 + mCursorColumn];
            default:
                // specials (just note cut)
                return mBindings[PianoInput::BindingNoteCut];
        }
    }

    void determineMinHeight() {
        auto metrics = fontMetrics();
        setMinimumHeight((metrics.height() + GRID_PADDING * 2) * GRID_ROWS);
    }

    void moveCursorColumn(int amount) {
        if (mCursorRow < 4) {
            mCursorColumn = std::clamp(mCursorColumn + amount, 0, 11);
            update();
        }
    }

    void moveCursorRow(int amount) {
        mCursorRow = std::clamp(mCursorRow + amount, 0, 4);
        if (mCursorRow == 4) {
            mCursorColumn = 11;
        }

        update();
    }

    void setKey(int key) {
        auto &dest = cursorKey();
        if (dest != key) {

            if (key) {
                // check if this key is already bound
                auto existing = std::find(mBindings.begin(), mBindings.end(), (Qt::Key)key);
                if (existing != mBindings.end()) {
                    // key is bound, overwrite with the cursor's old value
                    *existing = dest;
                }
            }
            dest = (Qt::Key)key;

            emit bindingsChanged();
            update();
        }
    }

    PianoInput::Bindings mBindings;

    int mColumnWidth;
    int mRowHeight;

    int mCursorRow;
    int mCursorColumn;

};

//
// QAbstractItemModel subclass for ShortcutTable
//
class ShortcutTableModel : public QAbstractItemModel {

    Q_OBJECT

public:
    explicit ShortcutTableModel(ShortcutTable &src, QObject *parent = nullptr) :
        QAbstractItemModel(parent),
        mSrc(src)
    {
    }

    virtual QModelIndex index(int row, int column, QModelIndex const& index = QModelIndex()) const override {
        if (!index.isValid()) {
            return createIndex(row, column, nullptr);
        } else {
            return {};
        }
    }

    virtual Qt::ItemFlags flags(QModelIndex const& index) const override {
        if (index.isValid()) {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
        }

        return Qt::NoItemFlags;
    }

    virtual QModelIndex parent(QModelIndex const& index) const override {
        Q_UNUSED(index)
        return {};
    }

    virtual int rowCount(QModelIndex const& parent = QModelIndex()) const override {
        if (parent.isValid()) {
            return 0;
        } else {
            return ShortcutTable::Count;
        }
    }

    virtual int columnCount(QModelIndex const& parent = QModelIndex()) const override {
        if (parent.isValid()) {
            return 0;
        } else {
            return 2;
        }
    }

    virtual QVariant data(QModelIndex const& index, int role = Qt::DisplayRole) const override {
        if (index.isValid() && role == Qt::DisplayRole) {
            auto const row = index.row();
            if (row >= 0 && row < ShortcutTable::Count) {
                switch (index.column()) {
                    case 0:
                        return tr(TU::SHORTCUT_NAMES[row]);
                    case 1:
                        return mSrc.get((ShortcutTable::Shortcut)row).toString(QKeySequence::NativeText);
                    default:
                        break;
                }
            
            }
        }

        return {};
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
                case 0:
                    return tr("Action");
                case 1:
                    return tr("Key sequence");
                default:
                    break;
            }
        }

        return {};
    }

    void setShortcut(int row, QKeySequence const& seq) {
        mSrc.set((ShortcutTable::Shortcut)row, seq);
        emit dataChanged(
            createIndex(row, 1, nullptr),
            createIndex(row, 1, nullptr),
            { Qt::DisplayRole }
        );
    }

private:
    Q_DISABLE_COPY(ShortcutTableModel)

    ShortcutTable &mSrc;

};




KeyboardConfigTab::KeyboardConfigTab(PianoInput &input, ShortcutTable &shortcuts, QWidget *parent) :
    ConfigTab(parent),
    mInput(input),
    mShortcuts(shortcuts),
    mSelectedShortcut(-1)
{

    auto layout = new QVBoxLayout;

    auto keyboardLayoutLayout = new QHBoxLayout;
    keyboardLayoutLayout->addWidget(new QLabel(tr("Layout:")));

    mLayoutCombo = new QComboBox;
    keyboardLayoutLayout->addWidget(mLayoutCombo, 1);

    mCustomGroup = new QGroupBox(tr("Custom layout"));
    auto customLayout = new QVBoxLayout;
    customLayout->addWidget(new QLabel(tr("Assign keys to each note. Press Shift+Del to unbind.")));
    mBindingEdit = new BindingEdit;
    customLayout->addWidget(mBindingEdit);
    mCustomGroup->setLayout(customLayout);

    auto shortcutGroup = new QGroupBox(tr("Shortcuts"));
    auto shortcutLayout = new QVBoxLayout;
    auto shortcutView = new QTreeView;
    mModel = new ShortcutTableModel(shortcuts, this);
    shortcutView->setModel(mModel);

    auto shortcutEditLayout = new QHBoxLayout;
    mKeyEdit = new QKeySequenceEdit;
    mClearButton = new QPushButton(tr("Clear"));
    mDefaultButton = new QPushButton(tr("Default"));
    shortcutEditLayout->addWidget(new QLabel(tr("Keys:")));
    shortcutEditLayout->addWidget(mKeyEdit, 1);
    shortcutEditLayout->addWidget(mClearButton);
    shortcutEditLayout->addWidget(mDefaultButton);

    shortcutLayout->addWidget(shortcutView, 1);
    shortcutLayout->addLayout(shortcutEditLayout);
    shortcutGroup->setLayout(shortcutLayout);

    layout->addLayout(keyboardLayoutLayout);
    layout->addWidget(mCustomGroup);
    layout->addWidget(shortcutGroup);
    layout->addStretch(1);
    setLayout(layout);

    // I'd like to have both columns 50/50 and user resizeable but
    // couldn't get it working so just do this for now
    shortcutView->resizeColumnToContents(0);

    for (int kblayout = PianoInput::LayoutQwerty; kblayout < PianoInput::LayoutCount; ++kblayout) {
        mLayoutCombo->addItem(PianoInput::layoutName((PianoInput::KeyboardLayout)kblayout));
    }
    mLayoutCombo->setCurrentIndex(input.layout());

    mCustomGroup->setEnabled(input.layout() == PianoInput::LayoutCustom);
    mBindingEdit->setBindings(input.bindings());

    lazyconnect(mBindingEdit, bindingsChanged, this, setDirty<Config::CategoryKeyboard>);

    lazyconnect(mClearButton, clicked, mKeyEdit, clear);
    connect(mDefaultButton, &QPushButton::clicked, this,
        [this]() {
            // sets the default shortcut for the current selection
            mKeyEdit->setKeySequence(ShortcutTable::getDefault((ShortcutTable::Shortcut)mSelectedShortcut));
        });

    connect(shortcutView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [this](QItemSelection const& selected) {
            QSignalBlocker blocker(mKeyEdit); // ignore changed signals for the key edit

            bool hasSelection = !selected.isEmpty();
            setKeyEditEnable(hasSelection);
            if (hasSelection) {
                // get the row of the selection
                mSelectedShortcut = selected.indexes().first().row();
                // update the key edit with this row's key sequence
                mKeyEdit->setKeySequence(mShortcuts.get((ShortcutTable::Shortcut)mSelectedShortcut));
            } else {
                // no selection, clear the edit
                mSelectedShortcut = -1;
                mKeyEdit->clear();
            }
        });

    connect(mKeyEdit, &QKeySequenceEdit::keySequenceChanged, this,
        [this](QKeySequence const& seq) {
            // update the shortcut's sequence in the model
            mModel->setShortcut(mSelectedShortcut, seq);
        });

    // any change made to the model dirties the config
    lazyconnect(mModel, dataChanged, this, setDirty<Config::CategoryKeyboard>);

    connect(mLayoutCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this](int index) {
            if (index != -1) {
                setDirty<Config::CategoryKeyboard>();
                auto const layout = (PianoInput::KeyboardLayout)index;
                mInput.setLayout(layout);
                mBindingEdit->setBindings(mInput.bindings());
                mCustomGroup->setEnabled(layout == PianoInput::LayoutCustom);
            }
        });

    setKeyEditEnable(false);
}

void KeyboardConfigTab::apply(PianoInput &input) {

    // we only need to copy the bindings if the user set a custom layout
    if (mLayoutCombo->currentIndex() == PianoInput::LayoutCustom) {
        input.setBindings(mBindingEdit->bindings());
    }

    clean();
}

void KeyboardConfigTab::setKeyEditEnable(bool enable) {
    mKeyEdit->setEnabled(enable);
    mClearButton->setEnabled(enable);
    mDefaultButton->setEnabled(enable);
}

#include "KeyboardConfigTab.moc"

#undef TU
