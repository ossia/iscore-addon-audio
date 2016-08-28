#include "AudioClock.hpp"
#include <Engine/Executor/ExecutorContext.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>
#include <Engine/Executor/BaseScenarioElement.hpp>
#include <Engine/Executor/ConstraintElement.hpp>
#include <Engine/Executor/Settings/ExecutorModel.hpp>

#include <Audio/Settings/Card/CardSettingsModel.hpp>
#include <Audio/AudioStreamEngine/AudioDocumentPlugin.hpp>

#include <Audio/AudioStreamEngine/Streams/AudioStreamIScoreExtensions.h>


namespace Audio
{
namespace AudioStreamEngine
{

AudioClock::AudioClock(
        const Engine::Execution::Context& ctx):
    ClockManager{ctx},
    m_default{ctx},
    m_audioPlug{context.doc.plugin<AudioStreamEngine::DocumentPlugin>()}
{
    auto bs = context.sys.baseScenario();
    if(!bs)
        return;

    auto& audio = context.doc.app.settings<Audio::Settings::Model>();

    ossia::time_constraint& ossia_cst = *bs->baseConstraint()->OSSIAConstraint();

    ossia_cst.setDriveMode(ossia::clock::DriveMode::EXTERNAL);
    // Number of milliseconds in each step -> we tick once per buffer
    ossia_cst.setGranularity(1000 * audio.getBufferSize() / audio.getRate() );
}

void AudioClock::play_impl(
        const TimeValue& t,
        Engine::Execution::BaseScenarioElement& bs)
{
    auto stream = m_audioPlug.makeStream();
    if(!stream)
    {
        qDebug("No stream!");
        return;
    }

    m_default.play(t);
    AudioStream finals = MakeIScoreExecutor(stream, *bs.baseConstraint()->OSSIAConstraint());
    auto& player = m_audioPlug.audioContext.audio.player;


    StartSound(player, finals, GenRealDate(player, 0));
    StartAudioPlayer(player);
}

void AudioClock::pause_impl(
        Engine::Execution::BaseScenarioElement& bs)
{
    m_default.pause();
    PauseAudioPlayer(m_audioPlug.audioContext.audio.player);
}

void AudioClock::resume_impl(
        Engine::Execution::BaseScenarioElement& bs)
{
    m_default.resume();
    ContAudioPlayer(m_audioPlug.audioContext.audio.player);
}

void AudioClock::stop_impl(
        Engine::Execution::BaseScenarioElement& bs)
{
    m_audioPlug.stop();
    m_default.stop();
}

std::unique_ptr<Engine::Execution::ClockManager> AudioClockFactory::make(
        const Engine::Execution::Context& ctx)
{
    return std::make_unique<AudioClock>(ctx);
}

QString AudioClockFactory::prettyName() const
{
    return QObject::tr("Audio");
}


}
}
