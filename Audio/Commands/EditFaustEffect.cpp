#include "EditFaustEffect.hpp"

#include <iscore/tools/SettableIdentifierGeneration.hpp>
#include <iscore/tools/ModelPathSerialization.hpp>
#include <Audio/EffectProcess/FaustEffectModel.hpp>
namespace Audio
{
namespace Commands
{

EditFaustEffect::EditFaustEffect(
        const Effect::FaustEffectModel& model,
        const QString& text):
    m_model{model},
    m_old{model.text()},
    m_new{text}
{
}

void EditFaustEffect::undo() const
{
    m_model.find().setText(m_old);
}

void EditFaustEffect::redo() const
{
    m_model.find().setText(m_new);
}

void EditFaustEffect::serializeImpl(DataStreamInput& s) const
{
    s << m_model << m_old << m_new;
}

void EditFaustEffect::deserializeImpl(DataStreamOutput& s)
{
    s >> m_model >> m_old >> m_new;
}

}
}
