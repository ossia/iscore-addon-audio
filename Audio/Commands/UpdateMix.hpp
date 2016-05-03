#pragma once
#include <iscore/command/SerializableCommand.hpp>
#include <Audio/Commands/AudioCommandFactory.hpp>
#include <iscore/tools/ModelPath.hpp>
#include <Audio/MixProcess/Mix.hpp>

namespace Audio
{
namespace Effect {
class EffectModel;
class ProcessModel;
class EffectFactory;
}
namespace Mix
{
class ProcessModel;
}

namespace Commands
{
class UpdateRouting final : public iscore::SerializableCommand
{
           ISCORE_COMMAND_DECL(Audio::CommandFactoryName(), UpdateRouting, "Update routing")
    public:
        UpdateRouting(
                const Mix::ProcessModel& model,
                Mix::Routing newmix);

        void undo() const override;
        void redo() const override;

    protected:
        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        Path<Mix::ProcessModel> m_model;
        Mix::Routing m_old, m_new;
};

// MOVEME
class UpdateDirect final : public iscore::SerializableCommand
{
           ISCORE_COMMAND_DECL(Audio::CommandFactoryName(), UpdateDirect, "Update direct")
    public:
        UpdateDirect(
                const Mix::ProcessModel& model,
                Mix::DirectMix newmix);

        void undo() const override;
        void redo() const override;

    protected:
        void serializeImpl(DataStreamInput & s) const override;
        void deserializeImpl(DataStreamOutput & s) override;

    private:
        Path<Mix::ProcessModel> m_model;
        Mix::DirectMix m_old, m_new;
};
}
}
