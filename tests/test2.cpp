#include <Audio/AudioStreamEngine/AudioDocumentPlugin.hpp>
#include <Audio/AudioStreamEngine/GroupAudioStream.h>
#include <QtTest/QtTest>
#include <QMetaType>
#include <QObject>
#include <thread>
#include <chrono>
#include <iostream>
using namespace std::chrono_literals;

class test1: public QObject
{
        Q_OBJECT
    public:


        struct AudioContext
        {
                AudioPlayerPtr player{};
                AudioRendererPtr renderer{};
                DeviceInfo device_info{};
                RendererInfo renderer_info{};
        };

    private slots:

        void equalityTest()
        {
            using namespace Audio;
            using namespace Audio::AudioStreamEngine;
            auto renderer = kJackRenderer;
            AudioContext m_ctx;
            GetDeviceInfo(renderer, 0, &m_ctx.device_info);
            auto& dev = m_ctx.device_info;
            qDebug() << dev.fName
                     << dev.fMaxInputChannels
                     << dev.fMaxOutputChannels
                     << dev.fDefaultBufferSize
                     << dev.fDefaultSampleRate;

            m_ctx.player = OpenAudioPlayer(2, 2, 44100, 512, 65536*4, 44100*60*20, renderer, 1);
            m_ctx.renderer = GetAudioPlayerRenderer(m_ctx.player);

            std::cout << std::flush;
            std::cerr << std::flush;
            // Build the first stream
            auto stream1 = MakeReadSound("/tmp/1.wav");
            /*
            auto stream1 = [] {
                auto player = MakeGroupPlayer();
                auto date1 = GenRealDate(player, 44100);
                auto date2 = GenRealDate(player, 44100*2);

                // First sound
                StartSound(player,
                           MakeFadeSound(
                               MakeSinusStream(44100 * 10, 330), 44100, 2048),
                           date1);

                // Second sound
                StartSound(player,
                           MakeFadeSound(
                               MakeSinusStream(44100 * 10, 220), 44100, 2048),
                           date2);

                // Third sound
                auto file = MakeReadSound("/tmp/1.wav");
                StartSound(player, file, date2);

                // Make a stream on the sound
                return MakeGroupStream(player);
            }();
            */

            auto fx_send = MakeSend(stream1);

            // Build a first effect stream
            {

                auto fx_1 = MakeFaustAudioEffect("/tmp/examples/guitarix.dsp", "/usr/local/lib/faust/architecture", "");
                auto fx_return_1 = MakeReturn(fx_send);
                auto fx_chain_1 = MakeEffectSound(fx_return_1, fx_1, 0, 0);

                StartSound(m_ctx.player, fx_chain_1, GenRealDate(m_ctx.player, 0));

            }

            // Build a second effect stream
            {
                auto fx_2 = MakeFaustAudioEffect("/tmp/examples/freeverb.dsp", "/usr/local/lib/faust/architecture/", "");
                auto fx_return_2 = MakeReturn(fx_send);
                auto fx_chain_2 = MakeEffectSound(fx_return_2, fx_2, 0, 0);

                StartSound(m_ctx.player, fx_chain_2, GenRealDate(m_ctx.player, 0));
            }

            std::cout << std::flush;
            std::cerr << std::flush;
            StartSound(m_ctx.player, fx_send, GenRealDate(m_ctx.player, 0));
            // Mix the streams
            StartAudioPlayer(m_ctx.player);

            for(int i = 5; i --> 0;)
                std::this_thread::sleep_for(1s);

            StopAudioPlayer(m_ctx.player);
            CloseAudioClient(m_ctx.player);
            CloseAudioRenderer(m_ctx.renderer);
            AudioGlobalsDestroy();
        }
};

QTEST_APPLESS_MAIN(test1)
#include "test2.moc"