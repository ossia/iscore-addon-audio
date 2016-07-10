#include "AudioApplicationPlugin.hpp"
#include "AudioDocumentPlugin.hpp"
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <Scenario/Application/ScenarioApplicationPlugin.hpp>
#include <OSSIA/OSSIAApplicationPlugin.hpp>
#include <QAction>
#include <Audio/Settings/Card/CardSettingsModel.hpp>
#include <LibAudioStreamMC++.h>
//#include <faust/dsp/llvm-dsp.h>
namespace Audio
{
namespace AudioStreamEngine
{

ApplicationPlugin::ApplicationPlugin(const iscore::GUIApplicationContext& app):
    iscore::GUIApplicationContextPlugin{app},
    m_ctx{*this}
{
}

void ApplicationPlugin::initialize()
{
    // Restart everything if audio settings change.
    auto& set = GUIApplicationContextPlugin::context.settings<Settings::Model>();
    con(set, &Settings::Model::BufferSizeChanged,
        this, &ApplicationPlugin::startEngine);
    con(set, &Settings::Model::CardChanged,
        this, &ApplicationPlugin::startEngine);
    con(set, &Settings::Model::DriverChanged,
        this, &ApplicationPlugin::startEngine);
    con(set, &Settings::Model::RateChanged,
        this, &ApplicationPlugin::startEngine);
    startEngine();

   // startMTDSPFactories();
}

ApplicationPlugin::~ApplicationPlugin()
{
    stopEngine();
    //stopMTDSPFactories();
}

void ApplicationPlugin::on_newDocument(iscore::Document* doc)
{
    auto plug = new DocumentPlugin{m_ctx, *doc, &doc->model()};
    doc->model().addPluginModel(plug);
}

void ApplicationPlugin::on_loadedDocument(iscore::Document* doc)
{
    on_newDocument(doc);
}

static int CardIdFromString(int api, const QString& str);
static int CardIdFromString(int api, const QString& str)
{
    int n = GetDeviceCount(api);
    for(int i = 0; i < n; ++i)
    {
        DeviceInfo devinfo;
        GetDeviceInfo(api, i, &devinfo);
        if(str == QString::fromUtf8(devinfo.fName))
        {
            return i;
        }
    }
    return -1;
}

void ApplicationPlugin::startEngine()
{
    stopEngine();

    auto& stngs = iscore::GUIApplicationContextPlugin::context.settings<Audio::Settings::Model>();
    auto api = stngs.getDriverId();
    if(api == -1)
        return;

    // Initialize libaudiostream structures
    auto card = CardIdFromString(api, stngs.getCard());
    if(card == -1)
        return;

    GetDeviceInfo(api, card, &m_ctx.device_info);
    auto& dev = m_ctx.device_info;
    qDebug() << dev.fName
             << dev.fMaxInputChannels
             << dev.fMaxOutputChannels
             << dev.fDefaultBufferSize
             << dev.fDefaultSampleRate;

    AudioGlobalsInit(2, 2, stngs.getRate(),
                     stngs.getBufferSize(),
                     65536*4,
                     stngs.getRate()*60*20, // TODO this ends up allocating a big 400 mb memory chunk. Instead we should use some kind of circular buffer.
                     1);

    m_ctx.renderer = MakeAudioRenderer(api);
    m_ctx.sample_rate = stngs.getRate();
    GetAudioRendererInfo(m_ctx.renderer, &m_ctx.renderer_info);
    OpenAudioRenderer(m_ctx.renderer, 0, card, 2, 2, stngs.getBufferSize(), stngs.getRate());

    emit audioEngineRestarted();
}

void ApplicationPlugin::stopEngine()
{
    if(m_ctx.player)
    {
        StopAudioPlayer(m_ctx.player);
        CloseAudioClient(m_ctx.player);
        m_ctx.player = nullptr;
    }

    if(m_ctx.renderer)
    {
        CloseAudioRenderer(m_ctx.renderer);
        m_ctx.renderer = nullptr;
        m_ctx.renderer_info = {};
        m_ctx.device_info = {};
    }

    AudioGlobalsDestroy();
}

bool ApplicationPlugin::engineStatus() const
{
    return m_ctx.renderer;
}

}
}
