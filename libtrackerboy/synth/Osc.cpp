
#include "trackerboy/synth/Osc.hpp"
#include "trackerboy/gbs.hpp"
#include "./sampletable.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>

//
// TODO: replace PulseChannel and WaveChannel step methods with this
//

namespace trackerboy {

// Sinc table
// this table contains sets of normalized sinc lookup tables
// each set has a different phase (0 to -1). First set has a phase of -1/(SINC_PHASES+1)
// and the last set has a phase of -(SINC_PHASES)/(SINC_PHASES+1)
// see https://www.desmos.com/calculator/w0rh8oarmu for how these values were chosen.
// When generating a band-limited step, the fractional part of the delta's location in the
// waveform is used to pick the initial set (phase). This phase is incremented by mPhaseIncr
// every period.
//
// using this we can create bandlimited steps by multiplying each value in a step by
// the delta change.

const float Osc::SINC_TABLE[Osc::SINC_PHASES][Osc::SINC_STEPS] = {
    /* Phase: -0.016 */    { -0.002226277f, 0.002596360f, -0.003114014f, 0.003889488f, -0.005179266f, 0.007748825f, -0.015378438f, 0.999594152f, 0.015866643f, -0.007870854f, 0.005233500f, -0.003919994f, 0.003133537f, -0.002609918f, 0.002236238f },
    /* Phase: -0.047 */    { -0.006627868f, 0.007723950f, -0.009254392f, 0.011541191f, -0.015329070f, 0.022818081f, -0.044614457f, 0.996351123f, 0.049002767f, -0.023913350f, 0.015815707f, -0.011814896f, 0.009429554f, -0.007845587f, 0.006717233f },
    /* Phase: -0.078 */    { -0.010927046f, 0.012724811f, -0.015230620f, 0.018965332f, -0.025126658f, 0.037217680f, -0.071738429f, 0.989884257f, 0.083897486f, -0.040243510f, 0.026470330f, -0.019720923f, 0.015714131f, -0.013060559f, 0.011173706f },
    /* Phase: -0.109 */    { -0.015083657f, 0.017552592f, -0.020987963f, 0.026095299f, -0.034487758f, 0.050837509f, -0.096662872f, 0.980231822f, 0.120404623f, -0.056719534f, 0.037097640f, -0.027562505f, 0.021926722f, -0.018204413f, 0.015562503f },
    /* Phase: -0.141 */    { -0.019059258f, 0.022163056f, -0.026474411f, 0.032868229f, -0.043333735f, 0.063577235f, -0.119316176f, 0.967450798f, 0.158365101f, -0.073193960f, 0.047596071f, -0.035263486f, 0.028006691f, -0.023226881f, 0.019840730f },
    /* Phase: -0.172 */    { -0.022817463f, 0.026514469f, -0.031641133f, 0.039225526f, -0.051592194f, 0.075346872f, -0.139642864f, 0.951616645f, 0.197607830f, -0.089514658f, 0.057863068f, -0.042747818f, 0.033893902f, -0.028078325f, 0.023966167f },
    /* Phase: -0.203 */    { -0.026324267f, 0.030567976f, -0.036442902f, 0.045113333f, -0.059197497f, 0.086067282f, -0.157603726f, 0.932822585f, 0.237950712f, -0.105525970f, 0.067796014f, -0.049940273f, 0.039529271f, -0.032710206f, 0.027897671f },
    /* Phase: -0.234 */    { -0.029548351f, 0.034287937f, -0.040838469f, 0.050482977f, -0.066091239f, 0.095670536f, -0.173175782f, 0.911179066f, 0.279201776f, -0.121069796f, 0.077293143f, -0.056767166f, 0.044855367f, -0.037075575f, 0.031595580f },
    /* Phase: -0.266 */    { -0.032461360f, 0.037642226f, -0.044790898f, 0.055291329f, -0.072222643f, 0.104100220f, -0.186352253f, 0.886812925f, 0.321160257f, -0.135986775f, 0.086254470f, -0.063157037f, 0.049816940f, -0.041129515f, 0.035022117f },
    /* Phase: -0.297 */    { -0.035038136f, 0.040602505f, -0.048267875f, 0.059501126f, -0.077548862f, 0.111311629f, -0.197142288f, 0.859866500f, 0.363617986f, -0.150117517f, 0.094582714f, -0.069041394f, 0.054361492f, -0.044829614f, 0.038141746f },
    /* Phase: -0.328 */    { -0.037256937f, 0.043144453f, -0.051241945f, 0.063081242f, -0.082035229f, 0.117271841f, -0.205570638f, 0.830496550f, 0.406360567f, -0.163303778f, 0.102184236f, -0.074355334f, 0.058439814f, -0.048136376f, 0.040921554f },
    /* Phase: -0.359 */    { -0.039099615f, 0.045247957f, -0.053690724f, 0.066006877f, -0.085655436f, 0.121959724f, -0.211677223f, 0.798873305f, 0.449168742f, -0.175389707f, 0.108969934f, -0.079038277f, 0.062006459f, -0.051013626f, 0.043331575f },
    /* Phase: -0.391 */    { -0.040551752f, 0.046897259f, -0.055597037f, 0.068259709f, -0.088391602f, 0.125365868f, -0.215516612f, 0.765179217f, 0.491819948f, -0.186223090f, 0.114856154f, -0.083034538f, 0.065020263f, -0.053428907f, 0.045345102f },
    /* Phase: -0.422 */    { -0.041602768f, 0.048081055f, -0.056949034f, 0.069827966f, -0.090234309f, 0.127492353f, -0.217157304f, 0.729607522f, 0.534089565f, -0.195656583f, 0.119765542f, -0.086293951f, 0.067444757f, -0.055353820f, 0.046938989f },
    /* Phase: -0.453 */    { -0.042245992f, 0.048792586f, -0.057740226f, 0.070706449f, -0.091182530f, 0.128352478f, -0.216681063f, 0.692361057f, 0.575752497f, -0.203548878f, 0.123627841f, -0.088772416f, 0.069248587f, -0.056764334f, 0.048093885f },
    /* Phase: -0.484 */    { -0.042478692f, 0.049029622f, -0.057969499f, 0.070896491f, -0.091243468f, 0.127970397f, -0.214182049f, 0.653650880f, 0.616584659f, -0.209765911f, 0.126380712f, -0.090432420f, 0.070405863f, -0.057641059f, 0.048794471f },
    /* Phase: -0.516 */    { -0.042302065f, 0.048794471f, -0.057641059f, 0.070405863f, -0.090432420f, 0.126380712f, -0.209765911f, 0.613694608f, 0.656364322f, -0.214182049f, 0.127970397f, -0.091243468f, 0.070896491f, -0.057969499f, 0.049029622f },
    /* Phase: -0.547 */    { -0.041721199f, 0.048093885f, -0.056764334f, 0.069248587f, -0.088772416f, 0.123627841f, -0.203548878f, 0.572715044f, 0.694873750f, -0.216681063f, 0.128352478f, -0.091182530f, 0.070706449f, -0.057740226f, 0.048792586f },
    /* Phase: -0.578 */    { -0.040744979f, 0.046938989f, -0.055353820f, 0.067444757f, -0.086293951f, 0.119765542f, -0.195656583f, 0.530938804f, 0.731900513f, -0.217157304f, 0.127492353f, -0.090234309f, 0.069827966f, -0.056949034f, 0.048081055f },
    /* Phase: -0.609 */    { -0.039385993f, 0.045345102f, -0.053428907f, 0.065020263f, -0.083034538f, 0.114856154f, -0.186223090f, 0.488594294f, 0.767239153f, -0.215516612f, 0.125365868f, -0.088391602f, 0.068259709f, -0.055597037f, 0.046897259f },
    /* Phase: -0.641 */    { -0.037660364f, 0.043331575f, -0.051013626f, 0.062006459f, -0.079038277f, 0.108969934f, -0.175389707f, 0.445910722f, 0.800692141f, -0.211677223f, 0.121959724f, -0.085655436f, 0.066006877f, -0.053690724f, 0.045247957f },
    /* Phase: -0.672 */    { -0.035587583f, 0.040921554f, -0.048136376f, 0.058439814f, -0.074355334f, 0.102184236f, -0.163303778f, 0.403116137f, 0.832071602f, -0.205570638f, 0.117271841f, -0.082035229f, 0.063081242f, -0.051241945f, 0.043144453f },
    /* Phase: -0.703 */    { -0.033190284f, 0.038141746f, -0.044829614f, 0.054361492f, -0.069041394f, 0.094582714f, -0.150117517f, 0.360436112f, 0.861200511f, -0.197142288f, 0.111311629f, -0.077548862f, 0.059501126f, -0.048267875f, 0.040602505f },
    /* Phase: -0.734 */    { -0.030494004f, 0.035022117f, -0.041129515f, 0.049816940f, -0.063157037f, 0.086254470f, -0.135986775f, 0.318092167f, 0.887913644f, -0.186352253f, 0.104100220f, -0.072222643f, 0.055291329f, -0.044790898f, 0.037642226f },
    /* Phase: -0.766 */    { -0.027526936f, 0.031595580f, -0.037075575f, 0.044855367f, -0.056767166f, 0.077293143f, -0.121069796f, 0.276300311f, 0.912059128f, -0.173175782f, 0.095670536f, -0.066091239f, 0.050482977f, -0.040838469f, 0.034287937f },
    /* Phase: -0.797 */    { -0.024319611f, 0.027897671f, -0.032710206f, 0.039529271f, -0.049940273f, 0.067796014f, -0.105525970f, 0.235269666f, 0.933498979f, -0.157603726f, 0.086067282f, -0.059197497f, 0.045113333f, -0.036442902f, 0.030567976f },
    /* Phase: -0.828 */    { -0.020904621f, 0.023966167f, -0.028078325f, 0.033893902f, -0.042747818f, 0.057863068f, -0.089514658f, 0.195201159f, 0.952110469f, -0.139642864f, 0.075346872f, -0.051592194f, 0.039225526f, -0.031641133f, 0.026514469f },
    /* Phase: -0.859 */    { -0.017316263f, 0.019840730f, -0.023226881f, 0.028006691f, -0.035263486f, 0.047596071f, -0.073193960f, 0.156286135f, 0.967786789f, -0.119316176f, 0.063577235f, -0.043333735f, 0.032868229f, -0.026474411f, 0.022163056f },
    /* Phase: -0.891 */    { -0.013590225f, 0.015562503f, -0.018204413f, 0.021926722f, -0.027562505f, 0.037097640f, -0.056719534f, 0.118705325f, 0.980437696f, -0.096662872f, 0.050837509f, -0.034487758f, 0.026095299f, -0.020987963f, 0.017552592f },
    /* Phase: -0.922 */    { -0.009763218f, 0.011173706f, -0.013060559f, 0.015714131f, -0.019720923f, 0.026470330f, -0.040243510f, 0.082627609f, 0.989990294f, -0.071738429f, 0.037217680f, -0.025126658f, 0.018965332f, -0.015230620f, 0.012724811f },
    /* Phase: -0.953 */    { -0.005872630f, 0.006717233f, -0.007845587f, 0.009429554f, -0.011814896f, 0.015815707f, -0.023913350f, 0.048209105f, 0.996389568f, -0.044614457f, 0.022818081f, -0.015329070f, 0.011541191f, -0.009254392f, 0.007723950f },
    /* Phase: -0.984 */    { -0.001956161f, 0.002236238f, -0.002609918f, 0.003133537f, -0.003919994f, 0.005233500f, -0.007870854f, 0.015592244f, 0.999598444f, -0.015378438f, 0.007748825f, -0.005179266f, 0.003889488f, -0.003114014f, 0.002596360f }
};


// Public methods ------------------------------------------------------------

uint16_t Osc::frequency() {
    return mFrequency;
}

void Osc::generate(int16_t buf[], size_t nsamples) {
    // first, clear the output buffer
    std::fill_n(buf, nsamples, static_cast<int16_t>(0));

    if (!mMuted && mDeltaBuf.size() > 0) {

        auto deltaEnd = mDeltaBuf.end();

        if (mRecalc) {
            // frequency and/or waveform changed, recalculate deltas
            mSamplesPerDelta = (2048 - mFrequency) * mMultiplier * mFactor;
            mSamplesPerPeriod = mSamplesPerDelta * mWaveformSize;

            // calculate initial position
            for (auto iter = mDeltaBuf.begin(); iter != deltaEnd; ++iter) {
                iter->position = iter->location * mSamplesPerDelta;
            }

            mPrevious = mDeltaBuf[0].before;

            mRecalc = false;
        }

        // copy the leftovers
        std::copy_n(mLeftovers, SINC_STEPS - 1, buf);
        // clear the leftover buffer
        std::fill_n(mLeftovers, SINC_STEPS - 1, static_cast<int16_t>(0));

        // add the transitions (sinc sets)

        // add steps for each delta until this limit is reached
        const size_t limit = nsamples + SINC_CENTER;
        for (auto iter = mDeltaBuf.begin(); iter != deltaEnd; ++iter) {
            // cache these values
            float position = iter->position;
            int16_t change = iter->change;
            
            while (position < limit) {

                // index of the center of the step
                size_t centerIndex = static_cast<size_t>(position);
                // the sinc set chosen is determined by the fractional part of position
                const float *sincset = SINC_TABLE[static_cast<size_t>((position - centerIndex) * SINC_PHASES)];
                
                // ending index of the band limited step
                size_t endIndex = centerIndex + (SINC_STEPS - SINC_CENTER);

                // determine the range of the set to use
                size_t sincStart;
                size_t sincEnd;
                int16_t *dest = buf;

                if (centerIndex < SINC_CENTER) {
                    // we are at the start of the buffer
                    sincStart = SINC_CENTER - centerIndex;
                } else {
                    // we are somewhere in between
                    sincStart = 0;
                    // point dest to the start of the step
                    dest += centerIndex - SINC_CENTER;
                }

                if (endIndex > nsamples) {
                    // we are at the end of the buffer
                    sincEnd = endIndex - nsamples;
                } else {
                    sincEnd = SINC_STEPS;
                }

                // copy to the buffer
                int16_t error = 0;  // roundoff error
                for (size_t i = sincStart; i != sincEnd; ++i) {
                    int16_t sample = change * sincset[i];
                    error += sample;
                    *dest++ += sample;
                }

                // copy to leftovers (if needed)
                dest = mLeftovers;
                for (size_t i = sincEnd; i != SINC_STEPS; ++i) {
                    int16_t sample = change * sincset[i];
                    error += sample;
                    *dest++ = sample;
                }

                // add the error so that the sum of the step is equal to change
                // (this error comes from taking the floor of a sample in the step)
                if (centerIndex >= nsamples) {
                    mLeftovers[centerIndex - nsamples] += change - error;
                } else {
                    buf[centerIndex] += change - error;
                }


                // next period
                position += mSamplesPerPeriod;
            }

            iter->position = position - nsamples;
            
        }

        // do the running sum
        for (size_t i = 0; i != nsamples; ++i) {
            *buf += mPrevious;
            mPrevious = *buf++;
        }

    }

}

bool Osc::muted() {
    return mMuted;
}

float Osc::outputFrequency() {
    return Gbs::CLOCK_SPEED / ((2048 - mFrequency) * mMultiplier * mWaveformSize);
}

void Osc::reset() {

}

void Osc::setFrequency(uint16_t frequency) {
    mFrequency = frequency;
    // recalculate on next call to generate()
    mRecalc = true;
}


void Osc::setMute(bool muted) {
    mMuted = muted;
}


// protected methods ---------------------------------------------------------


Osc::Osc(float samplingRate, size_t multiplier, size_t waveformSize) :
    mMultiplier(multiplier),
    mWaveformSize(waveformSize),
    mFactor(samplingRate / Gbs::CLOCK_SPEED),
    mFrequency(Gbs::DEFAULT_FREQUENCY),
    mRecalc(true),
    mSamplesPerDelta(0.0f),
    mSamplesPerPeriod(0.0f),
    mPrevious(0),
    mLeftovers{ 0 },
    mDeltaBuf(),
    mMuted(false) 
{
    // assert that waveform size is a power of 2
    assert((mWaveformSize & (mWaveformSize - 1)) == 0);
}


void Osc::deltaSet(const uint8_t waveform[]) {

    // clear existing waveform
    mDeltaBuf.clear();

    // convert waveform to a delta buffer

    size_t bufsize = mWaveformSize / 2;
    // bitmask used to wrap -1 to bufsize-1 or bufsize to 0
    size_t mask = bufsize - 1;
    uint8_t lo; // keep lo out here cause we need the last one for initSample
    size_t waveIndex = 0;
    size_t deltaIndex = 0;

    // the starting volume is the first samples
    int16_t previous = SAMPLE_TABLE[waveform[0] >> 4];
    mPrevious = previous;

    for (size_t i = 0; i != mWaveformSize; ++i) {

        uint8_t hi = waveform[waveIndex];
        lo = hi & 0xF;
        hi >>= 4;


        int8_t delta;
        if ((i & 1) == 1) {
            // for odd numbered indices, the delta is calculated by
            // subtracting the high nibble from the next sample and the current low nibble
            delta = lo - hi;
            ++waveIndex;
        } else {
            // even numbered indices, the delta is the lower nibble
            // minus the high nibble of the current byte
            delta = (waveform[(waveIndex + 1) & mask] >> 4) - lo;
        }

        if (delta) {
            Delta d;
            d.change = SAMPLE_TABLE[abs(delta)];
            if (delta < 0) {
                d.change = -d.change;
            }
            d.location = static_cast<uint8_t>(i);
            d.before = previous;
            previous += d.change;
            d.after = previous;
            mDeltaBuf.push_back(d);
            ++deltaIndex;
        }
    }

    mRecalc = true;


    //size_t deltaCount = mDeltaBuf.size();

    // make sure we don't have a flat waveform
    //if (deltaCount > 0) {



        // adjust the duration of the last delta
        // since the sum of the duration must be mWaveformSize, we just add what's missing
        //mDeltaBuf.back().duration += mWaveformSize - durationCounter;

        // determine timing information for generation
        //calcDeltaTimings();

        // now we need to adjust the sample counter and the current delta
        // for the new waveform.
        //
        // Three options here:
        //  1. pick the nearest delta time of the new waveform
        //  2. pick the closest matching delta change of the new waveform
        //  3. reset the counter to 0 (new waveform starts at the beginning)
        //
        // old waveform
        //  _      ___
        //   |____|
        //        ^ current sample counter
        //
        // new waveform
        //    ______
        //  _|      |_
        // ^ ^      ^
        // 3 2      1
        //
        // the numbers above are the corresponding options
        //
        // we'll go with 3 since it's the easiest, will reconsider if the sound is wonky

        //mSampleCounter = 0.0f;
        //mDeltaIndex = mDeltaBuf.size() - 1;
        //mDelta = mDeltaBuf.back();
    //}


}

}