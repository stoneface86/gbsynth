
#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>

#include "trackerboy.hpp"
#include "SynthWorker.hpp"
#include "PianoWidget.hpp"


namespace instedit {




class InstEditor : public QWidget {

    Q_OBJECT

public:

    InstEditor(QWidget *parent = nullptr);
    ~InstEditor();

private slots:
    void channelSelected(int index);
    void programChanged();
    
private:

    QPlainTextEdit *programEdit;
    QPlainTextEdit *outputEdit;
    PianoWidget *pianoWidget;
    SynthWorker *worker;
    
    std::vector<trackerboy::Instruction>* programTable[4];
    trackerboy::InstrumentRuntime* runtimeTable[4];
    trackerboy::TrackId currentTrackId;
    trackerboy::WaveTable waveTable;

    QStringList programList;
    QStringList outputList;

    bool parse();
};


}