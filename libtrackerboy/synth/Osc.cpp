
#include "trackerboy/synth/Osc.hpp"
#include "trackerboy/gbs.hpp"
#include "./sampletable.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

//
// TODO: replace PulseChannel and WaveChannel step methods with this
//

namespace {

// sinc table
// this table contains sets of normalized sinc lookup tables
// each set has a different phase (0 to -1). First set has a phase of -1/(SINC_PHASES+1)
// and the last set has a phase of -(SINC_PHASES)/(SINC_PHASES+1)
// see https://www.desmos.com/calculator/w0rh8oarmu for how these values were chosen.
// When generating a band-limited step, the fractional part of the sample counter is
// used to pick the set.
//
// using this we can create bandlimited steps by multiplying each value in a step by
// the delta change. The resulting sample is the running sum.

constexpr size_t SINC_STEPS = 15;
constexpr size_t SINC_PHASES = 32;

const float SINC_TABLE[SINC_PHASES][SINC_STEPS] = {
    // yes I know these literals would require doubles to fit them
    // table was generated by a script, do not edit manually thank you please
    {   -0.003443069602526828f,     0.004014030893398116f,    -0.004812000890278471f,     0.006005955998392686f,    -0.007987921477862281f,     0.011922270862480922f,    -0.023493886699594898f,       0.7987921477862268f,     0.024962254618319645f,    -0.012289109965941937f,     0.008150940283533003f,    -0.006097649983101034f,     0.004870683827964857f,    -0.004054782476072271f,    0.0034730093382010366f},
    {   -0.006825537811957421f,       0.0079517515509304f,    -0.009523055749617166f,     0.011868285896910953f,     -0.01574604267510959f,     0.023387504561559955f,      -0.0454385802910307f,       0.7951751550930387f,      0.05130162290922827f,     -0.02484922359665752f,     0.016395364022536853f,    -0.012233463924508274f,     0.009756750369239734f,       -0.008114032194827f,     0.006944761179851937f},
    {   -0.010117547067970183f,      0.01177863688509962f,    -0.014092297701815706f,     0.017537081584481775f,     -0.02321084327357883f,     0.034311681360942634f,     -0.06576405594180675f,       0.7891686713016817f,      0.07891686713016828f,     -0.03757946053817533f,     0.024661520978177573f,    -0.018352759797713548f,     0.014614234653734869f,     -0.01214105648156428f,     0.010383798306601036f},
    {   -0.013290303049126366f,     0.015461491171013348f,    -0.018480598914465667f,     0.022964861886358177f,     -0.03032253608295838f,      0.04461744595063868f,     -0.08441138423093807f,       0.7808053041361783f,      0.10769728332912805f,     -0.05037453575072114f,      0.03287601280573388f,     -0.02440016575425562f,      0.01939888954375603f,    -0.016099078435797463f,     0.013758683773324707f},
    {   -0.016316322221205805f,     0.018968729281795913f,    -0.022650894377673952f,       0.0281069492277706f,    -0.037025500425043975f,      0.05423453583386725f,      -0.1013329485316996f,       0.7701304088409164f,      0.13752328729302085f,     -0.06312544334761613f,      0.04096438344898488f,    -0.030320094836256528f,     0.024066575276278628f,     -0.01995156499587866f,       0.0170382833814362f},
    {   -0.019169665699003826f,      0.02227064103266621f,     -0.02656848403897022f,      0.03292181717872398f,     -0.04326867400632295f,      0.06310014959255432f,     -0.11649258386317729f,       0.7572017951106524f,      0.16826706558014506f,     -0.07572017951106536f,     0.048851728716816284f,      -0.0360572283386025f,     0.028573652645685007f,    -0.023662556097207903f,     0.020192047869617414f},
    {   -0.021826157465758483f,     0.025339636472441557f,    -0.030201310911921627f,      0.03737140630827712f,    -0.049005900725004915f,      0.07115925310754133f,     -0.12986563692126307f,        0.742089353835789f,      0.19979328757117407f,     -0.08804449960763605f,       0.0564633204005492f,     -0.04155700381480422f,     0.032877376435762835f,     -0.02719699202539544f,     0.023190292307368437f},
    {    -0.02426358514774526f,     0.028150470147141345f,    -0.033520213007578716f,      0.04142140607365078f,    -0.054196232245898215f,      0.07836482230150149f,     -0.14143894756856382f,       0.7248746062888897f,      0.23195987401244467f,     -0.09998270431570894f,      0.06372524011330893f,     -0.04676610363154124f,      0.03693628567077144f,     -0.03052103605426909f,      0.02600447018076739f},
    {   -0.026461881709576675f,      0.03068044256182803f,    -0.036499147185623006f,      0.04504150078225825f,     -0.05880418157683717f,      0.08467802147064551f,      -0.1512107526261528f,       0.7056501789220465f,      0.26461881709576746f,       -0.111418449303481f,      0.07056501789220464f,     -0.05163293992112536f,      0.04071058724550269f,     -0.03360238947247836f,     0.028607439686028874f},
    {    -0.02840328661680143f,      0.03290957728196704f,      -0.0391153832837094f,      0.04820557799048694f,     -0.06279992729035921f,      0.09006831677169945f,     -0.15919051336393386f,       0.6845192074649156f,      0.29761704672387634f,      -0.1222355727615921f,      0.07691227050167593f,     -0.05610813175941934f,     0.044162529513865484f,    -0.036410596141750795f,     0.030973719794792538f},
    {    -0.03007248520482499f,      0.03482077234242894f,    -0.041349667156634365f,      0.05089189803893462f,     -0.06615946745061506f,      0.09451352492945009f,     -0.16539866862653763f,       0.6615946745061505f,       0.3307973372530752f,      -0.1323189349012301f,      0.08269933431326887f,    -0.060144970409650084f,     0.047256762464725016f,    -0.038917333794479425f,     0.033079733725307515f},
    {   -0.031456725195207313f,       0.0363999248687399f,     -0.04318635153918293f,      0.05308322376691236f,      -0.0688647227246431f,      0.09799979772353054f,     -0.16986631605411961f,       0.6369986852029486f,      0.36399924868739925f,     -0.14155526337843305f,      0.08786188761419989f,      -0.0636998685202949f,      0.04996068119238812f,     -0.04109668936793217f,      0.03490403754536705f},
    {   -0.032545909510182346f,     0.037636028059168214f,     -0.04461349393530614f,      0.05476690979644477f,     -0.07090358857575442f,      0.10052154329727209f,     -0.17263482435835859f,       0.6108616861911151f,      0.39706009602422476f,     -0.14983399849970747f,      0.09233955721493602f,     -0.06673278924776888f,     0.052244749476871646f,     -0.04292541578640269f,      0.03642753174534173f},
    {   -0.033332664751902705f,     0.038521239925547936f,    -0.045622921029140576f,      0.05593495112476822f,     -0.07226993685147047f,      0.10208128580270204f,      -0.1737553800897056f,       0.5833216331582973f,      0.42981594022190345f,     -0.15704813200415702f,      0.09607650428489606f,     -0.06920765139166242f,     0.054082800425272595f,     -0.04438316774030525f,      0.03763365375214823f},
    {   -0.033812384932500945f,     0.039050923443170106f,    -0.046210259407751295f,      0.05658399111153221f,     -0.07296356748592313f,      0.10268946535055845f,     -0.17328847277906742f,       0.5545231128930158f,      0.46210259407751314f,     -0.16309503320382818f,      0.09902198444518139f,     -0.07109270678115588f,      0.05545231128930156f,     -0.04545271417155865f,      0.03850854950645941f},
    {    -0.03398325026262549f,      0.03922365801340419f,     -0.04637493267883147f,     0.056715289289652006f,     -0.07299011143363911f,        0.102364180669128f,     -0.17130332275241833f,        0.524616425929281f,       0.4937566361687351f,     -0.16787725629736994f,      0.10113087728757225f,     -0.07236088633507325f,     0.056334649764218085f,     -0.04612012535642031f,     0.039041222394737195f},
    {    -0.03384622102769555f,     0.039041222394737195f,     -0.04612012535642031f,     0.056334649764218085f,     -0.07236088633507325f,      0.10113087728757225f,     -0.16787725629736994f,       0.4937566361687351f,        0.524616425929281f,     -0.17130332275241833f,        0.102364180669128f,     -0.07299011143363911f,     0.056715289289652006f,     -0.04637493267883147f,      0.03922365801340419f},
    {   -0.033405006800784086f,      0.03850854950645944f,     -0.04545271417155868f,      0.05545231128930159f,     -0.07109270678115588f,      0.09902198444518139f,     -0.16309503320382818f,      0.46210259407751314f,       0.5545231128930157f,     -0.17328847277906742f,      0.10268946535055845f,     -0.07296356748592313f,      0.05658399111153222f,     -0.04621025940775131f,      0.03905092344317013f},
    {   -0.032666011456864666f,      0.03763365375214823f,     -0.04438316774030525f,     0.054082800425272595f,     -0.06920765139166242f,      0.09607650428489606f,     -0.15704813200415702f,       0.4298159402219033f,       0.5833216331582974f,      -0.1737553800897056f,      0.10208128580270204f,     -0.07226993685147047f,      0.05593495112476822f,    -0.045622921029140576f,     0.038521239925547936f},
    {   -0.031638254663284855f,      0.03642753174534173f,     -0.04292541578640269f,     0.052244749476871646f,     -0.06673278924776888f,      0.09233955721493602f,     -0.14983399849970747f,      0.39706009602422476f,       0.6108616861911151f,     -0.17263482435835859f,      0.10052154329727209f,     -0.07090358857575442f,      0.05476690979644477f,     -0.04461349393530614f,     0.037636028059168214f},
    {   -0.030333270723949937f,      0.03490403754536705f,     -0.04109668936793217f,      0.04996068119238812f,      -0.0636998685202949f,      0.08786188761419989f,     -0.14155526337843305f,      0.36399924868739925f,       0.6369986852029486f,     -0.16986631605411961f,      0.09799979772353054f,      -0.0688647227246431f,      0.05308322376691236f,     -0.04318635153918293f,       0.0363999248687399f},
    {   -0.028764985848093495f,     0.033079733725307515f,    -0.038917333794479425f,     0.047256762464725016f,    -0.060144970409650084f,      0.08269933431326887f,     -0.13231893490123017f,       0.3307973372530753f,       0.6615946745061505f,     -0.16539866862653763f,      0.09451352492945009f,     -0.06615946745061506f,      0.05089189803893462f,    -0.041349667156634365f,      0.03482077234242894f},
    {   -0.026949575097043904f,     0.030973719794792538f,    -0.036410596141750795f,     0.044162529513865484f,     -0.05610813175941925f,      0.07691227050167582f,      -0.1222355727615921f,      0.29761704672387634f,       0.6845192074649157f,     -0.15919051336393386f,      0.09006831677169935f,     -0.06279992729035914f,      0.04820557799048694f,      -0.0391153832837094f,      0.03290957728196704f},
    {   -0.024905300432542785f,     0.028607439686028874f,     -0.03360238947247836f,      0.04071058724550269f,     -0.05163293992112536f,      0.07056501789220464f,       -0.111418449303481f,      0.26461881709576746f,       0.7056501789220465f,      -0.1512107526261528f,      0.08467802147064551f,     -0.05880418157683717f,      0.04504150078225825f,    -0.036499147185623006f,      0.03068044256182803f},
    {   -0.022652331446527847f,      0.02600447018076739f,     -0.03052103605426909f,      0.03693628567077144f,     -0.04676610363154124f,      0.06372524011330893f,     -0.09998270431570894f,      0.23195987401244467f,       0.7248746062888897f,     -0.14143894756856382f,      0.07836482230150149f,    -0.054196232245898215f,      0.04142140607365078f,    -0.033520213007578716f,     0.028150470147141345f},
    {   -0.020212550493581834f,     0.023190292307368437f,     -0.02719699202539544f,     0.032877376435762835f,     -0.04155700381480422f,       0.0564633204005492f,     -0.08804449960763605f,      0.19979328757117407f,        0.742089353835789f,     -0.12986563692126307f,      0.07115925310754133f,    -0.049005900725004915f,      0.03737140630827712f,    -0.030201310911921627f,     0.025339636472441557f},
    {    -0.01760934407234077f,     0.020192047869617414f,    -0.023662556097207903f,     0.028573652645685007f,      -0.0360572283386025f,     0.048851728716816284f,     -0.07572017951106523f,      0.16826706558014493f,       0.7572017951106523f,     -0.11649258386317723f,      0.06310014959255432f,     -0.04326867400632295f,      0.03292181717872398f,     -0.02656848403897022f,      0.02227064103266621f},
    {   -0.014867382410056305f,       0.0170382833814362f,     -0.01995156499587866f,     0.024066575276278628f,    -0.030320094836256528f,      0.04096438344898488f,     -0.06312544334761602f,      0.13752328729302085f,       0.7701304088409164f,     -0.10133294853169944f,      0.05423453583386725f,    -0.037025500425043975f,       0.0281069492277706f,    -0.022650894377673952f,     0.018968729281795913f},
    {    -0.01201238929440273f,     0.013758683773324707f,    -0.016099078435797463f,      0.01939888954375603f,     -0.02440016575425562f,      0.03287601280573388f,     -0.05037453575072114f,      0.10769728332912805f,       0.7808053041361783f,     -0.08441138423093807f,      0.04461744595063868f,     -0.03032253608295838f,     0.022964861886358177f,    -0.018480598914465667f,     0.015461491171013348f},
    {   -0.009070904267835392f,     0.010383798306601036f,     -0.01214105648156428f,     0.014614234653734869f,    -0.018352759797713548f,     0.024661520978177573f,     -0.03757946053817533f,      0.07891686713016828f,       0.7891686713016817f,     -0.06576405594180675f,     0.034311681360942634f,     -0.02321084327357883f,     0.017537081584481775f,    -0.014092297701815706f,      0.01177863688509962f},
    {   -0.006070039351855322f,     0.006944761179851937f,       -0.008114032194827f,     0.009756750369239734f,    -0.012233463924508274f,     0.016395364022536853f,     -0.02484922359665752f,      0.05130162290922827f,       0.7951751550930387f,      -0.0454385802910307f,     0.023387504561559955f,     -0.01574604267510959f,     0.011868285896910953f,    -0.009523055749617166f,       0.0079517515509304f},
    {   -0.003037232501088363f,    0.0034730093382010366f,    -0.004054782476072271f,     0.004870683827964857f,    -0.006097649983101034f,     0.008150940283533003f,    -0.012289109965941937f,     0.024962254618319645f,       0.7987921477862268f,    -0.023493886699594898f,     0.011922270862480922f,    -0.007987921477862281f,     0.006005955998392686f,    -0.004812000890278471f,     0.004014030893398116f}
};

// the left side of the step includes half the step width
constexpr size_t SINC_STEP_LEFT = SINC_STEPS / 2 + 1;

// index of the center of the transition
constexpr size_t SINC_STEP_CENTER = SINC_STEPS / 2;

// the right side of the step includes the other half
constexpr size_t SINC_STEP_RIGHT = SINC_STEPS / 2;

}


namespace trackerboy {

// Public methods ------------------------------------------------------------

void Osc::reset() {
    mSampleCounter = 0.0f;
    mPrevious = 0;
    if (mDeltaBuf.size() > 0) {
        mDelta = mDeltaBuf[0];
    }
    mDeltaIndex = 0;
    mLastStepSize = 0;
}

void Osc::setFrequency(uint16_t frequency) {
    mSamplesPerDelta = (2048 - frequency) * mMultiplier * mFactor;
    mSamplesPerPeriod = mSamplesPerDelta * mWaveformSize;
    // reposition to the current delta
    if (mDeltaBuf.size() > 0) {
        mSampleCounter = mDelta.location * mSamplesPerDelta;
    } else {
        mSampleCounter = 0.0f;
    }

}

void Osc::setMute(bool muted) {
    mMuted = muted;
}

//template <unsigned multiplier, size_t segments>
//void Osc<multiplier, segments>::setWaveform(uint8_t waveform[segments / 2]) {
//
//    // convert waveform to a delta buffer
//
//
//    // first unpack the waveform so 1 byte = 1 segment
//    for (size_t i = 0, j = 0; j != segments; i++, j += 2) {
//        uint8_t byte = waveform[i];
//        mDeltaBuffer[j] = byte >> 4;
//        mDeltaBuffer[j + 1] = byte & 0xF;
//    }
//
//    // calculate the deltas
//    int8_t first = mDeltaBuffer[0]; // save the first value for the last delta
//    for (size_t i = 0; i != segments - 1; ++i) {
//        mDeltaBuffer[i] = mDeltaBuffer[i + 1] - mDeltaBuffer[i];;
//    }
//    // last one wraps to the start
//    mDeltaBuffer[segments - 1] = first - mDeltaBuffer[segments - 1];
//
//    
//}

void Osc::generate(int16_t buf[], size_t nsamples) {
    
    if (!mMuted && mDeltaBuf.size() > 0) {
        
        // start of the transition buffer for copying out
        auto transitionBufIter = mTransitionBuf.begin() + mTransitionBufRemaining;


        while (nsamples != 0) {
            // 1. generate a transition, put it into mTransitionBuf
            // 2. copy entire transition or nsamples into buffer
            // 3. once transition is fully copied out, go to step 1

            if (mTransitionBufRemaining == 0) {
                // generate the transition

                transitionBufIter = mTransitionBuf.begin();

                // transition generation process
                // A. flat part before the bandlimited step (if it exists)
                // B. left side of bandlimited step
                // C. right side of bandlimited step
                //
                // the sizes of the left and right sides depend on the durations of the deltas
                // (at high frequencies these can be zero, but some deltas will have at least one
                // sample on the left side due to the fractional part of the sample counter
                // eventually making a whole sample due to repeated summing).

                float time = mDelta.location * mSamplesPerDelta;
                float duration = mDelta.duration * mSamplesPerDelta;

                if (mSampleCounter > time) {
                    // we will be starting a new period after this step
                    mSampleCounter -= mSamplesPerPeriod;
                }

                // time, in samples, to the center of the current delta
                // the fractional part alone is used to determine the sinc set to use
                float timeToCenter = time - mSampleCounter;
                float timeToCenterWhole;
                float timeToCenterFract = std::modf(timeToCenter, &timeToCenterWhole);
                size_t samplesFromCenter = static_cast<size_t>(timeToCenterWhole);

                size_t leftSteps;
                
                // check if there is some extra samples to fill in between
                // the previous delta and the current one
                if (samplesFromCenter > SINC_STEPS) {
                    // plenty of room for the previous right side and this left side
                    leftSteps = SINC_STEP_LEFT;

                    // [A] flat part before the left of the step
                    size_t samples = samplesFromCenter - SINC_STEPS;

                    std::fill_n(transitionBufIter, samples, mPrevious);
                    transitionBufIter += samples;
                    mTransitionBufRemaining += samples;
                } else {
                    leftSteps = samplesFromCenter - mLastStepSize;
                }

                // determine sinc table
                const float *sincSet = SINC_TABLE[static_cast<size_t>(timeToCenterFract * SINC_PHASES)];

                // [B] left part of the step
                assert(leftSteps <= SINC_STEP_LEFT);
                for (size_t i = SINC_STEP_LEFT - leftSteps; i != SINC_STEP_LEFT; ++i) {
                    mPrevious += mDelta.change * sincSet[i];
                    *transitionBufIter++ = mPrevious;
                }
                mSampleCounter += timeToCenter;
                mTransitionBufRemaining += leftSteps;

                // sampleCounter is now at the center

                size_t rightSteps;
                if (duration > SINC_STEPS) {
                    rightSteps = SINC_STEP_RIGHT; // enough room for both sides
                } else {
                    rightSteps = mDelta.duration / 2; // not enough room, split it
                }

                // [C] right part of the step
                assert(rightSteps <= SINC_STEP_RIGHT);
                for (size_t i = SINC_STEPS - rightSteps; i != SINC_STEPS; ++i) {
                    mPrevious += mDelta.change * sincSet[i];
                    *transitionBufIter++ = mPrevious;
                }
                mTransitionBufRemaining += rightSteps;
                
                mLastStepSize = rightSteps;
                nextDelta();

                // go back to the start to copy out from
                transitionBufIter = mTransitionBuf.begin();
            }

            // copy the transition to the sample buffer
            size_t toCopy = std::min(mTransitionBufRemaining, nsamples);
            std::copy_n(mTransitionBuf.begin(), toCopy, buf);
            nsamples -= toCopy;
            mTransitionBufRemaining -= toCopy;
            transitionBufIter += toCopy;    // advance the input buffer
            buf += toCopy;                  // advance the output buffer   
        }
        
    } else {
        // no delta changes, waveform is completely flat or the oscilator is muted
        // this case should rarely happen
        // (only when the wave channel's waveform is flat)
        std::fill_n(buf, nsamples, (mMuted) ? static_cast<int16_t>(0) : mPrevious);
        mSampleCounter = std::fmodf(mSampleCounter + nsamples, mSamplesPerPeriod);
    }

}

// protected methods ---------------------------------------------------------

Osc::Osc(float samplingRate, size_t multiplier, size_t waveformSize) :
    mMultiplier(multiplier),
    mWaveformSize(waveformSize),
    mFactor(samplingRate / Gbs::CLOCK_SPEED),
    mSamplesPerDelta((2048 - Gbs::DEFAULT_FREQUENCY) * multiplier * mFactor),
    mSamplesPerPeriod(mSamplesPerDelta * waveformSize),
    mSampleCounter(0.0f),
    mPrevious(0),
    mDeltaBuf(),
    mDelta{ 0 },
    mDeltaIndex(0),
    mLastStepSize(0),
    mTransitionBuf(mSamplesPerPeriod + 1.0f),
    mTransitionBufRemaining(0),
    mMuted(false)
{
}

void Osc::deltaSet(uint8_t waveform[]) {
        
}

// private methods -----------------------------------------------------------

void Osc::nextDelta() {
    size_t size = mDeltaBuf.size();
    if (size > 0) {
        if (++mDeltaIndex == size) {
            mDeltaIndex = 0; // loop back
        }
        mDelta = mDeltaBuf[mDeltaIndex];
    }
}


}