
#include "Config.hpp"
#include "samplerates.hpp"

#include <QSettings>


namespace {

constexpr int DEFAULT_SAMPLERATE_INDEX = 4; // 44100 Hz
constexpr unsigned DEFAULT_BUFFERSIZE = 40;
constexpr unsigned DEFAULT_VOLUME = 100;


}


Config::Config(ColorTable &colorTable, Miniaudio &miniaudio) :
    mColorTable(colorTable),
    mMiniaudio(miniaudio)
{
}

Config::Appearance const& Config::appearance() {
    return mAppearance;
}

Config::Sound const& Config::sound() {
    return mSound;
}

void Config::readSettings() {
    QSettings settings;


    settings.beginGroup("config");

    mAppearance.font.setFamily(settings.value("fontFamily", "Cascadia Mono").toString());
    mAppearance.font.setPixelSize(settings.value("fontSize", 12).toInt());

    settings.beginReadArray("colors");
    readColor(settings, Color::background,              QColor(0x18, 0x18, 0x18));
    readColor(settings, Color::backgroundHighlight1,    QColor(0x20, 0x20, 0x20));
    readColor(settings, Color::backgroundHighlight2,    QColor(0x30, 0x30, 0x30));
    readColor(settings, Color::foreground,              QColor(0xC0, 0xC0, 0xC0));
    readColor(settings, Color::foregroundHighlight1,    QColor(0xF0, 0xF0, 0xF0));
    readColor(settings, Color::foregroundHighlight2,    QColor(0xFF, 0xFF, 0xFF));
    readColor(settings, Color::row,                     QColor(20, 20, 80));
    readColor(settings, Color::rowEdit,                 QColor(0, 0, 0));
    readColor(settings, Color::rowPlayer,               QColor(0, 0, 0));
    readColor(settings, Color::effectType,              QColor(0x80, 0x80, 0xFF));
    readColor(settings, Color::instrument,              QColor(0x80, 0xFF, 0x80));
    readColor(settings, Color::selection,               QColor(0x45, 0x45, 0x50));
    readColor(settings, Color::cursor,                  QColor(192, 192, 192));
    readColor(settings, Color::line,                    QColor(64, 64, 64));
    readColor(settings, Color::headerBackground,        QColor(224, 248, 208));
    readColor(settings, Color::headerForeground,        QColor(8, 24, 32));
    readColor(settings, Color::headerHover,             QColor(136, 192, 112));
    readColor(settings, Color::headerDisabled,          QColor(52, 104, 86));
    settings.endArray();
   
    mAppearance.showFlats = settings.value("showFlats", false).toBool();
    mAppearance.showPreviews = settings.value("showPreviews", true).toBool();

    
    QByteArray id = settings.value("deviceId").toByteArray();
    if (id.size() == sizeof(ma_device_id)) {

        int index = mMiniaudio.lookupDevice(reinterpret_cast<ma_device_id*>(id.data()));
        setDevice(index + 1);

        // if we don't find the device we will go back to the default device
        // TODO: let the user know via messagebox if this occurs
    } else {
        setDevice(0);
    }

    
    mSound.context = mMiniaudio.context();

    setSamplerate(settings.value("samplerateIndex", DEFAULT_SAMPLERATE_INDEX).toUInt());
    mSound.buffersize = settings.value("buffersize", DEFAULT_BUFFERSIZE).toUInt();
    mSound.volume = settings.value("volume", DEFAULT_VOLUME).toInt();
    mSound.lowLatency = settings.value("lowLatency", true).toBool();
}

void Config::readColor(QSettings &settings, Color color, QColor def) {
    settings.setArrayIndex(+color);
    mColorTable[+color] = settings.value("color", def).value<QColor>();
}


void Config::writeSettings() {
    QSettings settings;
    settings.beginGroup("config");

    settings.setValue("fontFamily", mAppearance.font.family());
    settings.setValue("fontSize", mAppearance.font.pixelSize());
    settings.beginWriteArray("colors", mColorTable.size());
    for (int i = 0; i != mColorTable.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("color", mColorTable[i]);
    }
    settings.endArray();
    settings.setValue("showFlats", mAppearance.showFlats);
    settings.setValue("showPreviews", mAppearance.showPreviews);

    QByteArray barray;
    if (mSound.deviceIndex != 0) {
        ma_device_id *id = mMiniaudio.deviceId(mSound.deviceIndex - 1);
        barray = QByteArray(reinterpret_cast<char*>(id), sizeof(ma_device_id));
    }
    settings.setValue("deviceId", barray);

    settings.setValue("samplerateIndex", mSound.samplerateIndex);
    settings.setValue("buffersize", mSound.buffersize);
    settings.setValue("volume", mSound.volume);
    settings.setValue("lowLatency", mSound.lowLatency);
}

void Config::setDevice(int index) {
    mSound.deviceIndex = index;
    if (index == 0) {
        mSound.device = nullptr;
    } else {
        mSound.device = mMiniaudio.deviceId(index - 1);
    }
}

void Config::setSamplerate(int index) {
    mSound.samplerate = SAMPLERATE_TABLE[index];
    mSound.samplerateIndex = index;
}
