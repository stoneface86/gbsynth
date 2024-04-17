
#include "config/data/ShortcutTable.hpp"
#include "config/data/keys.hpp"

#include <QKeySequence>
#include <QMetaEnum>
#include <QSettings>

#define TU ShortcutTableTU
namespace TU {

}

ShortcutTable::ShortcutTable() noexcept :
    mTable()
{
}

void ShortcutTable::set(Shortcut shortcut, QKeySequence const& seq) noexcept {
    auto &ref = mTable[shortcut];
    if (ref) {
        *ref = seq;
    } else {
        ref = seq;
    }
}

QKeySequence ShortcutTable::get(Shortcut shortcut) const noexcept {
    auto &ref = mTable[shortcut];
    if (ref) {
        return *ref;
    } else {
        return {};
    }
}

QKeySequence ShortcutTable::getDefault(Shortcut shortcut) noexcept {
    switch (shortcut) {
        case PrevInstrument:
            return tr("Ctrl+Left");
        case NextInstrument:
            return tr("Ctrl+Right");
        case PrevPattern:
            return tr("Ctrl+Up");
        case NextPattern:
            return tr("Ctrl+Down");
        case DecOctave:
            return tr("Ctrl+[");
        case IncOctave:
            return tr("Ctrl+]");
        case PlayStop:
            return tr("Return");
        case PasteMix:
            return tr("Ctrl+M");
        case InsertRow:
            return tr("Ins");
        case TransposeDecNote:
            return tr("Ctrl+F1");
        case TransposeIncNote:
            return tr("Ctrl+F2");
        case TransposeDecOctave:
            return tr("Ctrl+F3");
        case TransposeIncOctave:
            return tr("Ctrl+F4");
        case Transpose:
            return tr("Ctrl+T");
        case Reverse:
            return tr("Ctrl+R");
        case ReplaceInstrument:
            return {};
        case Play:
            return {};
        case PlayFromStart:
            return tr("F5");
        case PlayFromCursor:
            return tr("F6");
        case Step:
            return tr("F7");
        case Stop:
            return tr("F8");
        case PatternRepeat:
            return tr("F9");
        case Record:
            return tr("Space");
        case ToggleOutput:
            return tr("F10");
        case Solo:
            return tr("F11");
        case Kill:
            return tr("F12");
        case EditInstrument:
            return tr("Ctrl+I");
        case EditWaveform:
            return tr("Ctrl+W");
        case ModuleProperties:
            return tr("Ctrl+P");
        case ShowEffectsList:
            return tr("F1");
        case ShowUserManual:
            return tr("F2");
        default:
            return {};
    }
}

void ShortcutTable::readSettings(QSettings &settings) noexcept {
    
    settings.beginGroup(Keys::Shortcuts);

    auto meta = QMetaEnum::fromType<Shortcut>();
    for (int i = 0; i < Count; ++i) {
        auto keyname = meta.key(i);
        Q_ASSERT(keyname != nullptr);
        auto seqstr = settings.value(keyname).toString();
        set((Shortcut)i,
            seqstr.isEmpty()
            ? getDefault((Shortcut)i)
            : QKeySequence(seqstr, QKeySequence::PortableText));
    }

    settings.endGroup();

}

void ShortcutTable::writeSettings(QSettings &settings) noexcept {
    settings.beginGroup(Keys::Shortcuts);

    auto meta = QMetaEnum::fromType<Shortcut>();
    for (int i = 0; i < Count; ++i) {
        auto keyname = meta.key(i);
        Q_ASSERT(keyname != nullptr);
        settings.setValue(keyname, get((Shortcut)i).toString(QKeySequence::PortableText));
    }

    settings.endGroup();
}


#undef TU
