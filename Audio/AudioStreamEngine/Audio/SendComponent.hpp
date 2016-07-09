#pragma once
#include <Audio/AudioStreamEngine/Scenario/ProcessComponent.hpp>

namespace Audio
{
namespace Send
{
class ProcessModel;
}
namespace AudioStreamEngine
{
class SendComponent final :
        public ProcessComponent_T<Send::ProcessModel, true, false>
{
        COMPONENT_METADATA("8f973706-d3c2-4025-b121-e7918cf3a286")
    public:
       SendComponent(
               Send::ProcessModel& sound,
               DocumentPlugin& doc,
                const Id<Component>& id,
               QObject* parent_obj);


       void makeStream(const Context& ctx) override;
};
AUDIO_PROCESS_COMPONENT_FACTORY(SendComponentFactory, "2340bde2-3226-48ce-960b-5243563e026a", SendComponent, Send::ProcessModel)
}
}
