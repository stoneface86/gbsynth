
#pragma once

#include "internal/fileformat/payload/handlers/TableHandler.hpp"
#include "trackerboy/data/Module.hpp"

namespace trackerboy {

class WaveHandler : public TableHandler<Waveform, BLOCK_ID_WAVE> {

public:
    using TableHandler::TableHandler;

    FormatError processIn(Module &mod, InputBlock &block, size_t index);

    void processOut(Module const& mod, OutputBlock &block, size_t index);

    static FormatError deserializeWaveform(InputBlock &block, Waveform &wave);

};

}

