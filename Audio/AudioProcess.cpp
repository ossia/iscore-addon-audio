#include "AudioProcess.hpp"
#include <Audio/AudioProcessModel.hpp>
#include <Audio/AudioDocumentPlugin.hpp>

namespace RecreateOnPlay
{
namespace Audio
{
Executor::Executor(
        AudioBlock& block,
        AudioEngine& conf):
    m_conf{conf},
    m_block{block},
    m_start{OSSIA::State::create()},
    m_end{OSSIA::State::create()}
{

}

std::shared_ptr<OSSIA::StateElement> Executor::state(
        const OSSIA::TimeValue& t,
        const OSSIA::TimeValue&)
{
    if(double(t) == 0)
        m_block.start();
    if(double(t) >= 0.99)
        m_block.stop();

    return {};
}

/// Component
Component::Component(
        RecreateOnPlay::ConstraintElement& parentConstraint,
        ::Audio::ProcessModel& element,
        AudioBlock& block,
        const Context& ctx,
        const Id<iscore::Component>& id,
        QObject* parent):
    ProcessComponent{parentConstraint, element, id, "AudioComponent", parent}
{
    m_ossia_process = std::make_shared<Executor>(
                block,
                ctx.doc.plugin< ::Audio::AudioDocumentPlugin>().engine());
}

Component::~Component()
{
}

const iscore::Component::Key& Component::key() const
{
    static iscore::Component::Key k("AudioElement");
    return k;
}


/// Component Factory
ComponentFactory::~ComponentFactory()
{

}

ProcessComponent* ComponentFactory::make(
        ConstraintElement& cst,
        ::Process::ProcessModel& proc,
        const Context& ctx,
        const Id<iscore::Component>& id,
        QObject* parent) const
{
    auto& audio_proc = static_cast< ::Audio::ProcessModel&>(proc);
    if(auto b = audio_proc.block())
    {
        return new Component{
            cst,
                    audio_proc,
                    *b,
                    ctx, id, parent};
    }

    return nullptr;
}

const ComponentFactory::ConcreteFactoryKey&
ComponentFactory::concreteFactoryKey() const
{
    static ComponentFactory::ConcreteFactoryKey k("9af86804-e9a5-4e6f-b765-2de26ebe745f");
    return k;
}

bool ComponentFactory::matches(
        ::Process::ProcessModel& proc,
        const DocumentPlugin&,
        const iscore::DocumentContext&) const
{
    return dynamic_cast< ::Audio::ProcessModel*>(&proc);
}
}
}

