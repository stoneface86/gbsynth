
#pragma once

#include "core/audio/RenderFrame.hpp"
#include "core/audio/Ringbuffer.hpp"
#include "core/model/InstrumentListModel.hpp"
#include "core/model/SongListModel.hpp"
#include "core/model/WaveListModel.hpp"
#include "core/Config.hpp"
#include "core/Miniaudio.hpp"

#include "trackerboy/data/Song.hpp"
#include "trackerboy/data/Instrument.hpp"
#include "trackerboy/data/Waveform.hpp"
#include "trackerboy/engine/Engine.hpp"
#include "trackerboy/engine/InstrumentRuntime.hpp"
#include "trackerboy/Synth.hpp"
#include "trackerboy/note.hpp"

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QWaitCondition>

#include <cstddef>
#include <memory>
#include <optional>

//
// Class handles all sound renderering. Sound is either sent to the
// configured device set in Config or is written to file (export, not implemented yet)
//
class Renderer : public QObject {

    Q_OBJECT

public:

    struct Diagnostics {
        unsigned lockFails;
        unsigned underruns;
        unsigned elapsed;
        unsigned bufferUsage;
        unsigned bufferSize;
    };

    Renderer(
        Miniaudio &miniaudio,
        Spinlock &spinlock,
        ModuleDocument &document,
        InstrumentListModel &instrumentModel,
        SongListModel &songModel,
        WaveListModel &waveModel
    );
    ~Renderer();

    // DIAGNOSTICS ====

    Diagnostics diagnostics();

    ma_device const& device() const;

    AudioRingbuffer::Reader returnBuffer();

    Ringbuffer<RenderFrame>::Reader frameReturnBuffer();

    bool isRunning();

    ma_result lastDeviceError();

    void setConfig(Config::Sound const& config);

    void playMusic(uint8_t orderNo, uint8_t rowNo);
    
    // row preview (just the track)
    //void preview(trackerboy::TrackRow const& row);

    // row preview (all channels)
    //void preview(trackerboy::PatternRow const& row);

signals:

    //
    // Signal emitted when the audio callback thread was started
    //
    void audioStarted();

    //
    // Signal emitted when the audio callback thread was stopped.
    //
    void audioStopped();

    void audioSync();

    //
    // An error occurred during audio playback.
    //
    void audioError();

public slots:

    void clearDiagnostics();

    void stopMusic();

    // instrument preview
    void previewInstrument(trackerboy::Note note);

    // waveform preview
    void previewWaveform(trackerboy::Note note);

    void play();

    void playPattern();

    void playFromCursor();

    void playFromStart();

    void stopPreview();

private:
    Q_DISABLE_COPY(Renderer)

    enum class PreviewState {
        none,
        waveform,
        instrument
    };

    enum class CallbackState {
        running,
        stopping,
        stopped
    };

    // utility function for preview slots
    void resetPreview();

    // device management -----------------------------------------------------

    void closeDevice();

    //
    // Start the audio callback thread for the configured device. If the audio
    // callback thread is already running, the stop countdown is cancelled
    //
    void beginRender();

    // thread main functions -------------------------------------------------

    static void backgroundThreadRun(Renderer *renderer);
    void handleBackground();

    static void audioThreadRun(ma_device *device, void *out, const void *in, ma_uint32 frames);
    void handleAudio(int16_t *out, size_t frames);

    static void deviceStopHandler(ma_device *device);

    void render(AudioRingbuffer::Writer &writer);

    // class members ---------------------------------------------------------

    Miniaudio &mMiniaudio;
    Spinlock &mSpinlock;

    InstrumentListModel &mInstrumentModel;
    SongListModel &mSongModel;
    WaveListModel &mWaveModel;

    QWaitCondition mIdleCondition;
    QMutex mMutex;
    std::unique_ptr<QThread> mBackgroundThread;

    // enable flag, if true then the renderer will output sound when requested
    // otherwise it will do nothing. Renderer is enabled when setConfig successfully
    // initializes the configured device. It is disabled when an error occurs during
    // render or when initialization fails.
    bool mEnabled;
    bool mRunning;              // is the audio callback thread active?
    bool mStopBackground;       // flag to stop the background thread
    bool mStopDevice;           // flag to stop the callback thread
    bool mAborted;              // true if the render was aborted due to device error
    std::atomic_bool mSync;
    std::atomic_bool mCancelStop;
    std::optional<ma_device> mDevice;
    ma_device_config mDeviceConfig;
    ma_result mLastDeviceError;

    // renderering (synchronize access via spinlock)
    
    trackerboy::Synth mSynth;
    trackerboy::RuntimeContext mRc;
    // read access to the current song, wave table and instrument table
    trackerboy::Engine mEngine;
    // has read access to an Instrument and wave table
    trackerboy::InstrumentRuntime mIr;


    PreviewState mPreviewState;
    trackerboy::ChType mPreviewChannel;

    CallbackState mCallbackState;
    int mStopCounter;
    
    // internal sample buffer (callback reads + writes)
    AudioRingbuffer mBuffer;
    // outgoing sample buffer for GUI (callback writes, GUI reads)
    AudioRingbuffer mSampleReturnBuffer;

    // the current frame buffer
    int16_t *mFrameBuffer;
    size_t mFrameBuffersize;

    size_t mSyncCounter;
    size_t mSyncPeriod;

    trackerboy::Frame mCurrentEngineFrame;
    Ringbuffer<RenderFrame> mFrameReturnBuffer;

    bool mNewFrameSinceLastSync;

    // diagnostics
    std::atomic_uint mLockFails;
    std::atomic_uint mUnderruns;
    std::atomic_uint mSamplesElapsed;
    std::atomic_uint mBufferUsage;

};
