#pragma once
#include <iscore/plugins/customfactory/FactoryInterface.hpp>
#include <iscore/serialization/VisitorCommon.hpp>
#include <iscore_plugin_audio_export.h>

namespace Audio
{
namespace Effect
{
class EffectModel;

/**
 * @brief The EffectFactory class
 *
 * An abstract factory for the generation of plug-ins.
 * This is meant to be subclassed by factories of
 * categories of plug-ins.
 * For instance : VSTEffectFactory, FaustEffectFactory, etc.
 *
 * For now a QString is passed but according to the needs
 * of various plug-in APIs this may change.
 *
 */
class ISCORE_PLUGIN_AUDIO_EXPORT EffectFactory :
        public iscore::AbstractFactory<EffectFactory>
{
        ISCORE_ABSTRACT_FACTORY_DECL(
                EffectFactory,
                "3ffe0073-dfe0-4a7f-862f-220380ebcf08")
    public:
        virtual ~EffectFactory();

        virtual QString prettyName() const = 0; // VST, FaUST, etc...

        /**
         * @brief makeModel Creates an effect model
         * @param info Data used for the creation of the effect
         * @param parent Parent object
         * @return A valid effect instance if the info is correct, else nullptr.
         */
        virtual EffectModel* makeModel(
                const QString& info, // plugin name ? faust code ? dll location ?
                const Id<EffectModel>&,
                QObject* parent) const = 0;

        /**
         * @brief load Loads an effect model
         * @param data Serialized data
         * @param parent Parent object
         * @return If the effect can be loaded, an instance,
         * else a MissingEffectModel with the serialized data should be returned.
         */
        virtual EffectModel* load(
                const VisitorVariant& data,
                QObject* parent) const = 0;
};


/**
 * @brief The EffectFactoryList class
 *
 * If a factory cannot be found at load time
 * (for instance AU plug-in on Windows),
 * a MissingEffectModel should be returned.
 */
class ISCORE_PLUGIN_AUDIO_EXPORT EffectFactoryList final :
        public iscore::ConcreteFactoryList<EffectFactory>
{
    public:
        using object_type = Audio::Effect::EffectModel;

};
}
}
Q_DECLARE_METATYPE(UuidKey<Audio::Effect::EffectFactory>)