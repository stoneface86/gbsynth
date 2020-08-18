
#pragma once

#include <fstream>

#include "trackerboy/data/PatternMaster.hpp"
#include "trackerboy/data/Order.hpp"
#include "trackerboy/fileformat.hpp"
#include "trackerboy/Speed.hpp"


namespace trackerboy {


class Song {

public:

    static constexpr uint8_t DEFAULT_RPB = 4;
    static constexpr float DEFAULT_TEMPO = 150.0f;
    // Tempo = 150, RPB = 4  => 6.0 frames per row
    static constexpr Speed DEFAULT_SPEED = 0x30;

    Song();

    uint8_t rowsPerBeat();

    float tempo();

    float actualTempo();

    Speed speed();

    std::vector<Order>& orders();

    PatternMaster& patterns();

    Pattern getPattern(uint8_t orderNo);

    void setRowsPerBeat(uint8_t rowsPerBeat);

    void setTempo(float tempo);

    // sets the speed from the tempo and rowsPerBeat settings
    void setSpeed();

    void setSpeed(Speed speed);

private:

    //void calcSpeed();

    PatternMaster mMaster;
    std::vector<Order> mOrder;

    uint8_t mRowsPerBeat;
    float mTempo;

    // Speed - fixed point Q5.3
    // frame timing for each row
    // 4.0:   4, 4, 4, 4, ...
    // 4.125: 4, 4, 4, 4, 4, 4, 4, 5, ...
    // 4.25:  4, 4, 4, 5, 4, 4, 4, 5, ...
    // 4.375: 4, 4, 5, 4, 4, 5, 4, 4, 5, ...
    // 4.5:   4, 5, 4, 5, ...
    // 4.675: 4, 5, 5, 4, 5, 5, 4, 5, 5, 4, ...
    // 4.75:  4, 5, 5, 5, 4, 5, 5, 5, 4, ...
    // 4.875: 4, 5, 5, 5, 5, 5, 5, 4, ...

    Speed mSpeed; // frames per row

};


}