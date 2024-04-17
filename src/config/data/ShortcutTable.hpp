
#pragma once

#include <QCoreApplication>
#include <QKeySequence>

class QSettings;

#include <array>
#include <optional>

//
// Configuration data for user configured shortcuts.
//
class ShortcutTable {

    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ShortcutTable)

public:

    enum Shortcut {
        // QShortcut
        PrevInstrument,
        NextInstrument,
        PrevPattern,
        NextPattern,
        DecOctave,
        IncOctave,
        PlayStop,
        // QAction
        PasteMix,
        InsertRow,
        TransposeDecNote,
        TransposeIncNote,
        TransposeDecOctave,
        TransposeIncOctave,
        Transpose,
        Reverse,
        ReplaceInstrument,
        Play,
        PlayFromStart,
        PlayFromCursor,
        Step,
        Stop,
        PatternRepeat,
        Record,
        ToggleOutput,
        Solo,
        Kill,
        // Misc
        EditInstrument,
        EditWaveform,
        ModuleProperties,
        ShowEffectsList,
        ShowUserManual,

        Count
    };
    Q_ENUM(Shortcut)

    explicit ShortcutTable() noexcept;

    void set(Shortcut shortcut, QKeySequence const& seq) noexcept;

    QKeySequence get(Shortcut shortcut) const noexcept;

    static QKeySequence getDefault(Shortcut shortcut) noexcept;

    void readSettings(QSettings &settings) noexcept;

    void writeSettings(QSettings &settings) noexcept;


private:

    // optional is used for deferred construction
    // QKeySequence uses PIMPL so this prevents a bunch of
    // heap allocs when mTable is initialized.
    std::array<std::optional<QKeySequence>, Count> mTable;


};
