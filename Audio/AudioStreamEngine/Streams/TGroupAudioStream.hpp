#pragma once

#include <3rdparty/libaudiostream/src/TAudioStream.h>
#include <3rdparty/libaudiostream/src/renderer/TAudioRenderer.h>
#include "TExpAudioMixer.h"
#include "TEffectAudioStream.h"

#include <set>
#include <iostream>
#include <memory>
#include <cmath>
#include <unordered_map>

#include <iscore/tools/Todo.hpp>


#if defined(LILV_SHARED) // TODO instead add a proper preprocessor macro that also works in static case
#include <lilv/lilvmm.hpp>
struct LV2PluginInfo
{
};

struct LV2Data
{
        LV2Data(const LilvPlugin& plug, LilvWorld& w):
            plugin{&plug}, world{w}
        {
            using node_ptr = Lilv::Node;
            node_ptr input_class{lilv_new_uri(&world, LILV_URI_INPUT_PORT)};
            node_ptr output_class{lilv_new_uri(&world, LILV_URI_OUTPUT_PORT)};
            node_ptr control_class{lilv_new_uri(&world, LILV_URI_CONTROL_PORT)};
            node_ptr audio_class{lilv_new_uri(&world, LILV_URI_AUDIO_PORT)};

            int32_t numports = plugin.get_num_ports();
            for(int32_t i = 0; i < numports; i++)
            {
                auto port = plugin.get_port_by_index(i);
                if(port.is_a(audio_class))
                {
                    if(port.is_a(input_class))
                    {
                        in_ports.push_back(i);
                    }
                    else if(port.is_a(output_class))
                    {
                        out_ports.push_back(i);
                    }
                    else
                    {
                        qDebug() << "Audio port not input or output";
                    }
                }
                else if(port.is_a(control_class))
                {
                    if(port.is_a(input_class))
                    {
                        control_in_ports.push_back(i);
                    }
                    else if(port.is_a(output_class))
                    {
                        control_out_ports.push_back(i);
                    }
                }
            }
        }

        ~LV2Data()
        {

        }

        Lilv::Plugin plugin;
        LilvWorld& world;
        std::vector<int> in_ports, out_ports, control_in_ports, control_out_ports;
};

class LV2AudioEffect : public TAudioEffectInterface
{
    protected:
        LV2Data data;
        std::vector<float> fInData, fOutData, fParamMin, fParamMax, fParamInit;
        std::unordered_map<std::string, int> mLabelsMap;
    public:
        LV2AudioEffect(LV2Data dat):
            data{dat}
        {
            const int in_size = data.control_in_ports.size();
            fInData.resize(in_size);

            const int num_ports = data.plugin.get_num_ports();
            fParamMin.resize(num_ports);
            fParamMax.resize(num_ports);
            fParamInit.resize(num_ports);

            data.plugin.get_port_ranges_float(fParamMin.data(), fParamMax.data(), fParamInit.data());

            const int out_size = data.control_out_ports.size();
            fOutData.resize(out_size);

            for(int i = 0; i < in_size; i++)
            {
                Lilv::Port p{data.plugin.get_port_by_index(data.control_in_ports[i])};
                Lilv::Node n = p.get_name();
                mLabelsMap.emplace(n.as_string(), i);
            }
        }

        long Inputs() final override { return 2; }
        long Outputs() final override { return 2; }

        long GetControlCount() final override
        {
            return data.control_in_ports.size();
        }

        void GetControlParam(long param, char* label, float* min, float* max, float* init) final override
        {
            if(param >= 0 && param < (int64_t)data.control_in_ports.size())
            {
                Lilv::Port p = data.plugin.get_port_by_index(data.control_in_ports[param]);
                Lilv::Node n = p.get_name();
                strcpy(label, n.as_string());
                *min = fParamMin[param];
                *max = fParamMax[param];
                *init = fParamInit[param];
            }
        }

        void SetName(const std::string& name) final override
        {

        }

        std::string GetName() final override
        {
            return data.plugin.get_name().as_string();
        }


        void SetControlValue(long param, float value) final override
        {
            if(param >= 0 && param < (int64_t)data.control_in_ports.size())
                fInData[param] = value;
        }
        void SetControlValue(const char* label, float value) final override
        {
            auto it = mLabelsMap.find(label);
            if(it != mLabelsMap.end())
                SetControlValue(it->second, value);
        }

        float GetControlValue(long param) final override
        {
            if(param >= 0 && param < (int64_t)data.control_in_ports.size())
                return fInData[param];
            return {};
        }
        float GetControlValue(const char* label) final override
        {
            auto it = mLabelsMap.find(label);
            if(it != mLabelsMap.end())
                return GetControlValue(it->second);
            return {};
        }


};

class StereoLV2AudioEffect final : public LV2AudioEffect
{
        LilvInstance* fInstance;
    public:

        StereoLV2AudioEffect(LV2Data dat):
            LV2AudioEffect{std::move(dat)},
            fInstance{lilv_plugin_instantiate(data.plugin.me, 44100, nullptr)}
        {
            if(!fInstance)
                throw std::runtime_error("Error while creating a LV2 plug-in");

            const int in_size = data.control_in_ports.size();
            for(int i = 0; i < in_size; i++)
            {
                lilv_instance_connect_port(fInstance, data.control_in_ports[i], &fInData[i]);
            }

            const int out_size = data.control_out_ports.size();
            for(int i = 0; i < out_size; i++)
            {
                lilv_instance_connect_port(fInstance, data.control_out_ports[i], &fOutData[i]);
            }

            lilv_instance_activate(fInstance);
        }

        ~StereoLV2AudioEffect()
        {
            lilv_instance_deactivate(fInstance);
            lilv_instance_free(fInstance);
        }
    private:

        void Process(float** input, float** output, long framesNum) override
        {
            if(framesNum <= 0)
                return;

            lilv_instance_connect_port(fInstance, data.in_ports[0], input[0]);
            lilv_instance_connect_port(fInstance, data.in_ports[1], input[1]);
            lilv_instance_connect_port(fInstance, data.out_ports[0], output[0]);
            lilv_instance_connect_port(fInstance, data.out_ports[1], output[1]);

            lilv_instance_run(fInstance, framesNum);
        }

        TAudioEffectInterface* Copy() override
        {
            return nullptr;
        }

        void Reset() override
        {
            lilv_instance_deactivate(fInstance);
            lilv_instance_activate(fInstance);
        }
};

class MonoLV2AudioEffect final : public LV2AudioEffect
{
        LilvInstance* fLeft, *fRight;
    public:

        MonoLV2AudioEffect(LV2Data dat):
            LV2AudioEffect{std::move(dat)},
            fLeft{lilv_plugin_instantiate(data.plugin.me, 44100, nullptr)},
            fRight{lilv_plugin_instantiate(data.plugin.me, 44100, nullptr)}
        {
            if(!fLeft || !fRight)
                throw std::runtime_error("Error while creating a LV2 plug-in");

            const int in_size = data.control_in_ports.size();
            for(int i = 0; i < in_size; i++)
            {
                lilv_instance_connect_port(fLeft, data.control_in_ports[i], &fInData[i]);
                lilv_instance_connect_port(fRight, data.control_in_ports[i], &fInData[i]);
            }

            const int out_size = data.control_out_ports.size();
            for(int i = 0; i < out_size; i++)
            {
                lilv_instance_connect_port(fLeft, data.control_out_ports[i], &fOutData[i]);
                lilv_instance_connect_port(fRight, data.control_out_ports[i], &fOutData[i]);
            }

            lilv_instance_activate(fLeft);
            lilv_instance_activate(fRight);
        }

        ~MonoLV2AudioEffect()
        {
            lilv_instance_deactivate(fLeft);
            lilv_instance_free(fLeft);
            lilv_instance_deactivate(fRight);
            lilv_instance_free(fRight);
        }
    private:

        void Process(float** input, float** output, long framesNum) override
        {
            if(framesNum <= 0)
                return;

            lilv_instance_connect_port(fLeft, data.in_ports[0], input[0]);
            lilv_instance_connect_port(fLeft, data.out_ports[0], output[0]);
            lilv_instance_run(fLeft, framesNum);

            lilv_instance_connect_port(fRight, data.in_ports[0], input[1]);
            lilv_instance_connect_port(fRight, data.out_ports[0], output[1]);
            lilv_instance_run(fRight, framesNum);
        }

        TAudioEffectInterface* Copy() override
        {
            return nullptr;
        }

        void Reset() override
        {
            lilv_instance_deactivate(fLeft);
            lilv_instance_activate(fLeft);
            lilv_instance_deactivate(fRight);
            lilv_instance_activate(fRight);
        }
};


#endif
/**/
/**
 * @brief The TTimeNodeControlCommand struct
 * Only useful for interactive time nodes.
 * Maybe for events too ?
 */
//struct TTimeNodeControlCommand : public TControlCommand
//{
//        TTimeNodeControlCommand(const Audio::AudioStreamEngine::TimeNodeComponent& ctx):
//            m_audio_parent{ctx}
//        {

//        }

//        const Audio::AudioStreamEngine::TimeNodeComponent& m_audio_parent;

//        //std::vector<SymbolicDate> m_beforeDates;
//        //std::vector<SymbolicDate> m_afterDates;

//        bool Execute(
//                TNonInterleavedAudioBuffer<float>* buffer,
//                map<SymbolicDate, audio_frame_t>& date_map,
//                audio_frame_t cur_frame,
//                long frames)
//        {
//            auto timenode_time = fStartDate->getDate();
//            if (InBuffer(timenode_time, cur_frame, frames))
//            {
//                m_audio_parent.onDateFixed(cur_frame);

//                // Set stop date of all previous constraint to the TN frame
//                for(SymbolicDate date : m_beforeDates)
//                    date->setDate(timenode_time);

//                // Execute the time node.
//                // If we are here, it has already been triggered.
//                // We only have to check the conditions.
//                m_node.trigger();

//                // For all true events, set start date of all next constraints's start, and
//                // trigger their start / their start + offset.
//                for(SymbolicDate date : m_afterDates)
//                    date->setDate(timenode_time + 1);


//                // TODO after each command, the commands should be re-sorted.
//                // TODO the commands should have a priority for hierarchy ?

//                // TODO else find a way to disable' em

//                return false;
//            }
//            else
//            {
//                return true;
//            }
//        }

//};

