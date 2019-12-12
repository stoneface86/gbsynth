
#pragma once

#include <fstream>

#include "trackerboy/pattern/Pattern.hpp"
#include "trackerboy/song/Order.hpp"
#include "trackerboy/fileformat.hpp"
#include "trackerboy/Q53.hpp"


namespace trackerboy {


class Song {

public:

    static constexpr uint8_t DEFAULT_RPB = 4;
    static constexpr float DEFAULT_TEMPO = 150.0f;
    static constexpr uint8_t TABLE_CODE = 'S';

    Song();

    FormatError deserialize(std::ifstream &stream);

    uint8_t rowsPerBeat();

    float tempo();

    float actualTempo();

    Q53 speed();

    Order& order();

    std::vector<Pattern>& patterns();

    void serialize(std::ofstream &stream);

    void setRowsPerBeat(uint8_t rowsPerBeat);

    void setTempo(float tempo);

    void setSpeed(Q53 speed);

private:

    void calcSpeed();

    std::vector<Pattern> mPatterns;
    Order mOrder;

    uint8_t mRowsPerBeat;
    float mTempo;

    // Speed - fixed point F5.3
    // frame timing for each row
    // 4.0:   4, 4, 4, 4, ...
    // 4.125: 4, 4, 4, 4, 4, 4, 4, 5, ...
    // 4.25:  4, 4, 4, 5, 4, 4, 4, 5, ...
    // 4.375: 4, 4, 5, 4, 4, 5, 4, 4, 5, ...
    // 4.5:   4, 5, 4, 5, ...
    // 4.675: 4, 5, 5, 4, 5, 5, 4, 5, 5, 4, ...
    // 4.75:  4, 5, 5, 5, 4, 5, 5, 5, 4, ...
    // 4.875: 4, 5, 5, 5, 5, 5, 5, 4, ...

    Q53 mSpeed; // frames per row

};


}