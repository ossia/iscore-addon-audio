#include "ConstraintComponent.hpp"
#include <iscore/tools/std/Algorithms.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Loop/LoopProcessModel.hpp>
#include <boost/multi_index_container.hpp>
#include <Audio/SoundProcess/SoundProcessModel.hpp>
#include <Audio/ReturnProcess/ReturnProcessModel.hpp>
#include <Audio/EffectProcess/EffectProcessModel.hpp>
#include <Audio/MixProcess/MixProcessModel.hpp>
#include <Audio/AudioStreamEngine/Scenario/ScenarioComponent.hpp>
#include <Audio/AudioStreamEngine/Context.hpp>
#include <Audio/AudioStreamEngine/Scenario/ScenarioComponentFactory.hpp>
#include <Audio/AudioStreamEngine/Audio/EffectComponentFactory.hpp>
#include <Audio/AudioStreamEngine/Audio/SoundComponentFactory.hpp>
#include <Audio/AudioStreamEngine/Audio/MixComponentFactory.hpp>
#include <Audio/AudioStreamEngine/Audio/EffectComponent.hpp>
#include <Audio/AudioStreamEngine/Audio/ReturnComponent.hpp>
#include <Audio/AudioStreamEngine/Audio/SoundComponent.hpp>
#include <Audio/AudioStreamEngine/Audio/MixComponent.hpp>
#include <Audio/AudioStreamEngine/Utility.hpp>
namespace Audio
{
namespace AudioStreamEngine
{

const iscore::Component::Key&ConstraintComponent::key() const
{
    static const Key k{"a19b8885-5077-4331-a6b1-828571fa5128"};
    return k;
}


ConstraintComponent::ConstraintComponent(
        const Id<iscore::Component>& id,
        Scenario::ConstraintModel& constraint,
        const ConstraintComponent::system_t& doc,
        const iscore::DocumentContext& ctx,
        QObject* parent_comp):
    Component{id, "ConstraintComponent", parent_comp},
    m_baseComponent{*this, constraint, doc, ctx, this}
{
    con(constraint.duration, &Scenario::ConstraintDurations::executionSpeedChanged,
        this, [=] (double d) {
        if(d > 0.01)
            m_stretch = d;
    });
    if(constraint.duration.executionSpeed() > 0.01)
        m_stretch = constraint.duration.executionSpeed();
}

ConstraintComponent::~ConstraintComponent()
{
}


void ConstraintComponent::makeStream(const Context& player)
{
    auto& cst = m_baseComponent.constraint;
    if(cst.processes.empty())
    {
        // Silence
        /*
        auto sound = MakeNullSound(cst.duration.maxDuration().msec());
        m_stream = sound;
        */
    }
    else
    {
        using namespace std;
        // Input mix
        vector<pair<Scenario::ScenarioModel*, ScenarioComponent*>> scenarios;
        //map<Id<Scenario::ScenarioModel>, Loop::ProcessModel*> loops;
        vector<pair<Sound::ProcessModel*, SoundComponent*>> sounds;
        vector<pair<Effect::ProcessModel*, EffectComponent*>> sfxs;
        vector<pair<Mix::ProcessModel*, MixComponent*>> mixs;

        // For now we just put all the sound streams in parallel...
        // We have to take the mix volume into account.
        std::vector<AudioStream> soundStreams;
        for(auto& proc_pair :  m_baseComponent.processes())
        {
            auto& proc = proc_pair.process;
            auto& comp = proc_pair.component;
            if(auto scenar = dynamic_cast<Scenario::ScenarioModel*>(&proc))
            {
                auto stream = safe_cast<ScenarioComponent*>(&comp)->getStream();
                if(stream)
                {
                    qDebug() << "adding a scenario";
                    soundStreams.push_back(stream);
                }
            }
            else if(auto loop = dynamic_cast<Loop::ProcessModel*>(&proc))
            {
                //loops.insert(loop->id(), loop);
            }
            else if(auto sound = dynamic_cast<Sound::ProcessModel*>(&proc))
            {
                auto stream = safe_cast<SoundComponent*>(&comp)->getStream();
                if(stream)
                {
                    qDebug() << "adding a sound";
                    soundStreams.push_back(stream);
                }
            }
            else if(auto sfx = dynamic_cast<Effect::ProcessModel*>(&proc))
            {
                // TODO if there is only an effect and no input,
                // we should feed it at least some silence.

                auto stream = safe_cast<EffectComponent*>(&comp)->getStream();
                if(stream)
                {
                    qDebug() << "adding a sfx";
                    soundStreams.push_back(stream);
                }
            }
            else if(auto ret = dynamic_cast<Return::ProcessModel*>(&proc))
            {
                auto stream = safe_cast<ReturnComponent*>(&comp)->getStream();
                if(stream)
                {
                    qDebug() << "adding a return";
                    soundStreams.push_back(stream);
                }
            }
            else if(auto mix = dynamic_cast<Mix::ProcessModel*>(&proc))
            {
            }
        }

        m_stream = MixNStreams(soundStreams);
        return;

        // TODO this does not work :
        /*
        m_stream = MakePitchSchiftTimeStretchSound(
                    MixNStreams(soundStreams),
                    &m_shift,
                    &m_stretch);
        */
    }

    // Look for all the "contents" process :
    // - Scenario
    // - Loop
    // Look for all the "effects" process and apply them

    // Then look for the "Mix" process and do the mix
}


ProcessComponent*ConstraintComponent::make_processComponent(
        const Id<iscore::Component>& id,
        ProcessComponentFactory& factory,
        Process::ProcessModel& process,
        const DocumentPlugin& system,
        const iscore::DocumentContext& ctx,
        QObject* parent_component)
{
    return factory.make(id, process, system, ctx, parent_component);
}


void ConstraintComponent::removing(
        const Process::ProcessModel& cst,
        const ProcessComponent& comp)
{
}


}
}
