#pragma once
#include <vector>
#include <QString>
#include <libwatermark/Parameters.h>

class AudioEngine;
class AudioBlock
{
    public:
        AudioBlock(AudioEngine& en):
            m_engine{en}
        {

        }

        const Parameters<float>& parameters() const;

        virtual ~AudioBlock();
        virtual std::vector<float> data(int size, int buffer, int offset) const = 0;

        void start();
        void stop();

        int currentBuffer = 0;
        int offset = 0; // Offset in samples between the playing audio and the buffer.

        AudioEngine& m_engine;
};
