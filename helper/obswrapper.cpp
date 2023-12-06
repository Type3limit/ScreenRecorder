#include "obswrapper.h"
#include <string>
#include <QApplication>
#include "libavcodec/avcodec.h"
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#define USE_GS  1
#ifdef _WIN32
#define get_os_module(win, mac, linux) obs_get_module(win)
#define get_os_text(mod, win, mac, linux) obs_module_get_locale_text(mod, win)
#define INPUT_AUDIO_SOURCE "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"
#define INPUT_AUDIO_PROP_NAME "device_id"
#define OUTPUT_AUDIO_PROP_NAME "device_id"
#define DL_D3D11  "libobs-d3d11.dll"
#define DL_OPENGL  "libobs-opengl.dll"
#if USE_GS
#define DESKTOP_PROP_NAME "monitor_id"
#else
#define DESKTOP_PROP_NAME "monitor"
#endif

#define IS_INT false
#elif __APPLE__
#define get_os_module(win, mac, linux) obs_get_module(mac)
#define get_os_text(mod, win, mac, linux) obs_module_get_locale_text(mod, mac)
#define INPUT_AUDIO_SOURCE "coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE "coreaudio_output_capture"
#define DESKTOP_PROP_NAME "display_uuid";
#define INPUT_AUDIO_PROP_NAME "device_id"
#define OUTPUT_AUDIO_PROP_NAME "device_id"
#define IS_INT false;
#else
#include <obs-nix-platform.h>
#include <qpa/qplatformnativeinterface.h>
#define get_os_module(win, mac, linux) obs_get_module(linux)
#define get_os_text(mod, win, mac, linux) obs_module_get_locale_text(mod, linux)
#define INPUT_AUDIO_SOURCE "pulse_input_capture"
#define OUTPUT_AUDIO_SOURCE "pulse_output_capture"
#define DESKTOP_PROP_NAME  "screen"
#define INPUT_AUDIO_PROP_NAME "device_id"
#define OUTPUT_AUDIO_PROP_NAME "device_id"
#define DL_D3D11  "libobs-d3d11.so"
#define DL_OPENGL  "libobs-opengl.so"
#define IS_INT true
#endif
#define M_INFINITE 3.4e38f

#define VIDEO_ENCODER_ID           AV_CODEC_ID_H264
#define VIDEO_ENCODER_NAME         "libx264"
#define RECORD_OUTPUT_FORMAT       "mp4"
#define RECORD_OUTPUT_FORMAT_MIME  "video/mp4"
#define AUDIO_BITRATE 192


enum SOURCE_CHANNELS {
    SOURCE_CHANNEL_TRANSITION,
    SOURCE_CHANNEL_AUDIO_OUTPUT,
    SOURCE_CHANNEL_AUDIO_OUTPUT_2,
    SOURCE_CHANNEL_AUDIO_INPUT,
    SOURCE_CHANNEL_AUDIO_INPUT_2,
    SOURCE_CHANNEL_AUDIO_INPUT_3,
};



inline obs_source_t *getSoundSource(const char *sourceId, const char *deviceId, const char *deviceDesc, int channel)
{
    OBSSource source = obs_get_output_source(channel);
    if (!source) {
        OBSData settings = obs_data_create();
        obs_data_set_string(settings, "device_id", deviceId);
        source = obs_source_create(sourceId, deviceDesc, settings,
                                   nullptr);
    }
    return source;
}

ObsWrapper::ObsWrapper()
{

}

ObsWrapper::~ObsWrapper()
{

}

void ObsWrapper::release()
{
    obs_output_stop(fileOutput);
    obs_fader_remove_callback(this->mic_obs_fader, oBSVolumeChanged, this);
    obs_volmeter_remove_callback(this->mic_obs_volmeter, oBSVolumeLevel, this);
    obs_fader_remove_callback(this->player_obs_fader, oBSVolumeChanged, this);
    obs_volmeter_remove_callback(this->player_obs_volmeter, oBSVolumeLevel, this);

    obs_set_output_source(SOURCE_CHANNEL_TRANSITION,nullptr);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT,nullptr);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT,nullptr);

    for (int i = 0; i < MAX_CHANNELS; i++)
        obs_set_output_source(i, nullptr);

    auto cb = [](void *, obs_source_t *source) {
        obs_source_remove(source);
        return true;
    };

    obs_enum_scenes(cb, nullptr);
    obs_enum_sources(cb, nullptr);

}

void ObsWrapper::oBSVolumeChanged(void *data, float db)
{
    SoundDeviceIdentifier *curIdentifier = static_cast<SoundDeviceIdentifier *>(data);
    //todo:check some
}

void ObsWrapper::oBSVolumeLevel(void *data,
                                const float magnitude[MAX_AUDIO_CHANNELS],
                                const float peak[MAX_AUDIO_CHANNELS],
                                const float inputPeak[MAX_AUDIO_CHANNELS])
{
    SoundDeviceIdentifier *curIdentifier = static_cast<SoundDeviceIdentifier *>(data);
    if (curIdentifier != nullptr) {
        if (curIdentifier->type == SoundDeviceIdentifier::Type::Micphone) {
            if (curIdentifier->data != nullptr) {
                for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
                    curIdentifier->data->currentMagnitudeMic[channelNr] = magnitude[channelNr];
                    curIdentifier->data->currentPeakMic[channelNr] = peak[channelNr];
                    curIdentifier->data->currentInputPeakMic[channelNr] = inputPeak[channelNr];
                }
                emit curIdentifier->data->micVolumeDataChange(
                    curIdentifier->data->currentMagnitudeMic,
                    curIdentifier->data->currentPeakMic,
                    curIdentifier->data->currentInputPeakMic);
            }

        } else {
            if (curIdentifier->data != nullptr) {

                for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
                    curIdentifier->data->currentMagnitudePlayer[channelNr] = magnitude[channelNr];
                    curIdentifier->data->currentPeakPlayer[channelNr] = peak[channelNr];
                    curIdentifier->data->currentInputPeakPlayer[channelNr] = inputPeak[channelNr];
                }
                emit curIdentifier->data->playerVolumeDataChange(
                    curIdentifier->data->currentMagnitudePlayer,
                    curIdentifier->data->currentPeakPlayer,
                    curIdentifier->data->currentInputPeakPlayer);
            }

        }
    }
}

int ObsWrapper::micChannelCount()
{
    return obs_volmeter_get_nr_channels(mic_obs_volmeter);
}

int ObsWrapper::playerChannelCount()
{
    return obs_volmeter_get_nr_channels(player_obs_volmeter);
}

int ObsWrapper::audioChannel()
{
    return (int) audio_output_get_channels(obs_get_audio());
}

bool ObsWrapper::initObs(int srcWidth,int srcHeight,int fps)
{

    std::string path_str = qApp->applicationDirPath().toStdString();
    std::string cfg_path = path_str+"/desktop_rec_cfg";
    if (!obs_initialized()) {
        //init
        if (!obs_startup("zh-CN", cfg_path.c_str(), NULL)) {
            return false;
        }

        //plugin pathes

        std::string plugin_path = path_str + "/obs-plugins/64bit";

        std::string data_path = path_str + "/data/obs-plugins/%module%";




        obs_add_module_path(plugin_path.c_str(), data_path.c_str());


        //reset audio
        if (!resetAudio())
            return false;
        //reset video to enter graphcis model
        if (resetVideo(srcWidth,srcHeight,srcWidth,srcHeight,fps) != OBS_VIDEO_SUCCESS)
            return false;

        obs_load_all_modules();


    }

    if (!createOutputMode())
        return false;

    //connect player and micphone wave callback by default
    micSource = getSoundSource(INPUT_AUDIO_SOURCE, "default", "Default Mic/Aux", SOURCE_CHANNEL_AUDIO_INPUT);

    playerSource = getSoundSource(OUTPUT_AUDIO_SOURCE, "default", "Default Desktop Audio", SOURCE_CHANNEL_AUDIO_OUTPUT);

    mic_obs_fader = obs_fader_create(OBS_FADER_LOG);
    mic_obs_volmeter = obs_volmeter_create(OBS_FADER_LOG);
    player_obs_fader = obs_fader_create(OBS_FADER_LOG);
    player_obs_volmeter = obs_volmeter_create(OBS_FADER_LOG);
    micIdentifier = QSharedPointer<SoundDeviceIdentifier>(new SoundDeviceIdentifier(SoundDeviceIdentifier::Type::Micphone, this, mic_obs_volmeter));
    playerIndentifier = QSharedPointer<SoundDeviceIdentifier>(new SoundDeviceIdentifier(SoundDeviceIdentifier::Type::Player, this, player_obs_volmeter));
    obs_fader_add_callback(mic_obs_fader, oBSVolumeChanged, micIdentifier.get());
    obs_volmeter_add_callback(mic_obs_volmeter, oBSVolumeLevel, micIdentifier.get());
    obs_fader_add_callback(player_obs_fader, oBSVolumeChanged, playerIndentifier.get());
    obs_volmeter_add_callback(player_obs_volmeter, oBSVolumeLevel, playerIndentifier.get());

    obs_fader_attach_source(mic_obs_fader, micSource);
    obs_volmeter_attach_source(mic_obs_volmeter, micSource);
    obs_fader_attach_source(player_obs_fader, playerSource);
    obs_volmeter_attach_source(player_obs_volmeter, playerSource);
    obs_source_active(micSource);
    obs_source_active(playerSource);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, micSource);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, playerSource);
    return true;
}

int ObsWrapper::startRecording()
{
    emit recordStatusChanged(RecordingStatus::Recording);
    if (!obs_output_start(fileOutput))
    {
        emit recordStatusChanged(RecordingStatus::Stoped);
        return -1;
    }
    handler = obs_output_get_signal_handler(fileOutput);
    if(handler!=nullptr)
    {
        signal_handler_connect(handler, "stop", outPutStopedCallback,this);
    }
    isRecordingStarted=true;
    isPaused = false;
    return 0;
}

int ObsWrapper::pauseRecording()
{
    if(!isRecordingStarted)
        return -1;
    isPaused = !isPaused;
    obs_output_pause(fileOutput,isPaused);
    emit recordStatusChanged(isPaused?RecordingStatus::Paused:RecordingStatus::Recording);
    return 0 ;
}

void ObsWrapper::outPutStopedCallback(void* my_data, calldata_t* cd)
{
    const auto cur = static_cast<ObsWrapper*>(my_data);
    if(cur!=nullptr&&cur->handler!=nullptr)
    {
        cur->emit recordStatusChanged(RecordingStatus::Stoped);
        signal_handler_disconnect(cur->handler,"stop",outPutStopedCallback,my_data);
        //signal_handler_destroy(cur->handler);
        cur->handler = nullptr;
    }
}

int ObsWrapper::stopRecording(bool isForceStop)
{
    if (isForceStop)
        obs_output_force_stop(fileOutput);
    else
        obs_output_stop(fileOutput);
    isRecordingStarted = false;
    isPaused = false;
    return 0;
}

static void AddSource(void *_data, obs_scene_t *scene)
{
    obs_source_t *source = (obs_source_t *) _data;
    obs_scene_add(scene, source);
    obs_source_release(source);
}

int ObsWrapper::addSceneSource(const REC_TYPE type)
{
    obs_set_output_source(SOURCE_CHANNEL_TRANSITION, nullptr);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, nullptr);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, nullptr);

    size_t idx = 0;
    const char *id;
#ifndef _WIN32
#ifndef __APPLE__
    if (QApplication::platformName() == "xcb") {
        obs_set_nix_platform(OBS_NIX_PLATFORM_X11_EGL);
        blog(LOG_INFO, "Using EGL/X11");
    }

#ifdef ENABLE_WAYLAND
    if (QApplication::platformName().contains("wayland")) {
        obs_set_nix_platform(OBS_NIX_PLATFORM_WAYLAND);
        setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        blog(LOG_INFO, "Platform: Wayland");
    }
#endif
    QPlatformNativeInterface *native =
    QGuiApplication::platformNativeInterface();
    obs_set_nix_platform_display(
        native->nativeResourceForIntegration("display"));
#endif
#endif
    while (obs_enum_transition_types(idx++, &id)) {


        if (!obs_is_source_configurable(id)) {
            const char* name = obs_source_get_display_name(id);
            OBSSource tr = obs_source_create_private(id, name, NULL);

            if (strcmp(id, "fade_transition") == 0)
                fadeTransition = tr;

        }
    }

    if (!fadeTransition)
    {
        return -1;
    }



    scene = obs_scene_create("CurrentDesktopCapture");
    if (!scene) {
        return -2;
    }
    obs_set_output_source(SOURCE_CHANNEL_TRANSITION, fadeTransition);
    OBSSource s = obs_get_output_source(SOURCE_CHANNEL_TRANSITION);
    obs_transition_set(s, obs_scene_get_source(scene));

    //to create monitor capture
    if (type == REC_DESKTOP) {
#ifdef _WIN32
        //auto sets = obs_get_source_defaults("monitor_capture");
        captureSource = obs_source_create("monitor_capture", "Computer_Monitor_Capture", NULL, nullptr);
#else
        captureSource = obs_source_create("xshm_input", "Computer_Monitor_Capture", NULL, nullptr);
#endif

    } else {
        //auto sets = obs_get_source_defaults("window_capture");
        captureSource = obs_source_create("window_capture", "Window_Capture", NULL, nullptr);
    }
    if (captureSource) {
        obs_scene_atomic_update(scene, AddSource, captureSource);
    }
    else
    {
        
        return -3;
    }
    OBSData setting_source = obs_data_create();
    OBSData curSetting = obs_source_get_settings(captureSource);
    obs_data_set_int(setting_source,"method",1);
    obs_data_apply(setting_source, curSetting);
    obs_data_release(curSetting);
    return 0;
}


void ObsWrapper::ResetAudioDevice(const char *sourceId, const char *deviceId,
                      const char *deviceDesc, int channel)
{
    bool disable = deviceId && strcmp(deviceId, "disabled") == 0;
    if (disable) {
        obs_set_output_source(channel, nullptr);
        if(channel==SOURCE_CHANNELS::SOURCE_CHANNEL_AUDIO_INPUT)
        {
            memset(currentMagnitudeMic,0,sizeof(float)*8);
            memset(currentPeakMic,0,sizeof(float)*8);
            memset(currentInputPeakMic,0,sizeof(float)*8);
            emit micVolumeDataChange(
                            currentMagnitudeMic,
                            currentPeakMic,
                            currentInputPeakMic);
        }
        if(channel==SOURCE_CHANNEL_AUDIO_OUTPUT)
        {
            memset(currentMagnitudePlayer,0,sizeof(float)*8);
            memset(currentPeakPlayer,0,sizeof(float)*8);
            memset(currentInputPeakPlayer,0,sizeof(float)*8);
            emit playerVolumeDataChange(
                            currentMagnitudePlayer,
                            currentPeakPlayer,
                            currentInputPeakPlayer);
        }

    } else {
        OBSSource source = getSoundSource(sourceId, deviceId, deviceDesc, channel);
        if (source) {
            obs_set_output_source(channel, source);
            OBSData settings = obs_source_get_settings(source);
            const char *oldId =
                obs_data_get_string(settings, "device_id");
            if (strcmp(oldId, deviceId) != 0) {
                obs_data_set_string(settings, "device_id",
                                    deviceId);
                obs_source_update(source, settings);
            }
        }
    }
}

static bool HasAudioDevices(const char *source_id)
{
    const char *output_id = source_id;
    obs_properties_t *props = obs_get_source_properties(output_id);
    size_t count = 0;

    if (!props)
        return false;

    obs_property_t *devices = obs_properties_get(props, "device_id");
    if (devices)
        count = obs_property_list_item_count(devices);

    obs_properties_destroy(props);

    return count != 0;
}




static bool CreateAACEncoder(OBSEncoder &res, std::string &id,
                             const char *name, size_t idx)
{
    const char *id_ = "ffmpeg_aac";

    res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);

    if (res) {
        obs_encoder_release(res);
        return true;
    }

    return false;
}

void ObsWrapper::recPlayerAudio(bool enable,const QString& target)
{
    bool hasDesktopAudio = HasAudioDevices(OUTPUT_AUDIO_SOURCE);

    if (hasDesktopAudio)
        ResetAudioDevice(OUTPUT_AUDIO_SOURCE, enable?target.toUtf8().data():"disabled",
                         "Desktop Audio", SOURCE_CHANNEL_AUDIO_OUTPUT);
}

void ObsWrapper::recMicAudio(bool enable,const QString& target)
{
    bool hasInputAudio = HasAudioDevices(INPUT_AUDIO_SOURCE);
    if (hasInputAudio)
        ResetAudioDevice(INPUT_AUDIO_SOURCE, enable?target.toUtf8().data():"disabled",
                         "Mic/Aux", SOURCE_CHANNEL_AUDIO_INPUT);
}

QString ObsWrapper::micDeviceId(const QString& name)
{
    for(int i =0;i < m_micPhoneTargets.count();i++)
    {
        if(m_micPhoneTargets[i]==name)
        {
            return m_micPhoneTargetIds[i];
        }
    }
    return "default";
}

QString ObsWrapper::playerDeviceId(const QString& name)
{
    for(int i =0;i < m_playerTargets.count();i++)
    {
        if(m_playerTargets[i]==name)
        {
            return m_playerTargetIds[i];
        }
    }
    return "default";
}

void ObsWrapper::resetMicphoneVolumeLevelCallback(const QString& target)
{
    if(micSource)
    {
        obs_fader_detach_source(mic_obs_fader);
        obs_volmeter_detach_source(mic_obs_volmeter);
        obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, nullptr);
        obs_source_release(micSource);
        obs_fader_remove_callback(mic_obs_fader,oBSVolumeChanged,micIdentifier.get());
        obs_volmeter_remove_callback(mic_obs_volmeter,oBSVolumeLevel,micIdentifier.get());
        micSource = nullptr;
    }
    obs_fader_add_callback(mic_obs_fader, oBSVolumeChanged, micIdentifier.get());
    obs_volmeter_add_callback(mic_obs_volmeter, oBSVolumeLevel, micIdentifier.get());
    micSource = getSoundSource(INPUT_AUDIO_SOURCE, target.toStdString().c_str(), "Mic/Aux", SOURCE_CHANNEL_AUDIO_INPUT);
    obs_fader_attach_source(mic_obs_fader, micSource);
    obs_volmeter_attach_source(mic_obs_volmeter, micSource);
    obs_source_active(micSource);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, micSource);
    memset(currentMagnitudeMic,0,sizeof(float)*8);
    memset(currentPeakMic,0,sizeof(float)*8);
    memset(currentInputPeakMic,0,sizeof(float)*8);
    emit micVolumeDataChange(
                    currentMagnitudeMic,
                    currentPeakMic,
                    currentInputPeakMic);
}

void ObsWrapper::resetPlayerVolumeLevelCallback(const QString& target)
{
    if(playerSource)
    {

        obs_fader_detach_source(player_obs_fader);
        obs_volmeter_detach_source(player_obs_volmeter);
        obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, nullptr);
        obs_source_release(playerSource);
        obs_fader_remove_callback(player_obs_fader,oBSVolumeChanged,playerIndentifier.get());
        obs_volmeter_remove_callback(player_obs_volmeter,oBSVolumeLevel,playerIndentifier.get());
        playerSource = nullptr;
    }
    obs_fader_add_callback(player_obs_fader, oBSVolumeChanged, playerIndentifier.get());
    obs_volmeter_add_callback(player_obs_volmeter, oBSVolumeLevel, playerIndentifier.get());
    playerSource = getSoundSource(OUTPUT_AUDIO_SOURCE, target.toStdString().c_str(), "Desktop Audio", SOURCE_CHANNEL_AUDIO_OUTPUT);
    obs_fader_attach_source(player_obs_fader, playerSource);
    obs_volmeter_attach_source(player_obs_volmeter, playerSource);
    obs_source_active(playerSource);
    obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, playerSource);
    memset(currentMagnitudePlayer,0,sizeof(float)*8);
    memset(currentPeakPlayer,0,sizeof(float)*8);
    memset(currentInputPeakPlayer,0,sizeof(float)*8);
    emit playerVolumeDataChange(
                    currentMagnitudePlayer,
                    currentPeakPlayer,
                    currentInputPeakPlayer);
}

void ObsWrapper::searchRecTargets(REC_TYPE type)
{
    //start search record targets
    m_vecRecTargets.clear();
    //use to specific poroperty name
    std::string prop_name;
    std::string cur_id;
    bool is_int = IS_INT;
    if (type == REC_WINDOWS) {
        prop_name = "window";
    } else {
        prop_name = DESKTOP_PROP_NAME;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(captureSource);
    //auto nameOfSource =obs_source_get_id(captureSource);
    if (is_int) {
        cur_id = std::to_string(obs_data_get_int(settings, prop_name.c_str()));
    } else {
        cur_id = obs_data_get_string(settings, prop_name.c_str());
    }
    auto properties = obs_source_properties(captureSource);

    auto property = obs_properties_get(properties,prop_name.c_str());
    //property = obs_properties_first(properties);
    size_t count = obs_property_list_item_count(property);
    std::string str;
    for (size_t i = 0; i < count; i++) {
        if (type == REC_WINDOWS) {
            str = obs_property_list_item_string(property, i);
        } else {
            str = obs_property_list_item_name(property, i);
            std::string id;
            if (is_int) {
                id = std::to_string(obs_property_list_item_int(property, i));
            } else {
                const char *val = obs_property_list_item_string(property, i);
                id = val ? val : "";
            }
#ifdef _WIN32
            m_vecRecTargets.push_back(QString::fromStdString(str));
            m_vecRecTargetIds.push_back(QString::fromStdString(id));
#else
            m_vecRecTargets.insert(0,QString::fromStdString(str));//Linux use reversered order of screens
            m_vecRecTargetIds.insert(0,QString::fromStdString(id));
#endif


        }
    }
    obs_properties_destroy(properties);
}

QList<QString> ObsWrapper::getRecTargets()
{
    return m_vecRecTargets;
}

QList<QString> ObsWrapper::getRecTargetIds()
{
    return m_vecRecTargetIds;
}

bool ObsWrapper::updateRecItem(const char *target, REC_TYPE type, bool useCrop,
                               int LeftCrop ,
                               int RightCrop ,
                               int TopCrop ,
                               int BottomCrop)
{
    bool isFind = false;
    std::string prop_name = DESKTOP_PROP_NAME;
    int index = 0;
    for (const auto &ele: m_vecRecTargets) {
        if (ele == QString(target)) {
            OBSDataAutoRelease setting_source = obs_source_get_settings(captureSource);
            if (type == REC_DESKTOP)
            {

#ifdef _WIN32
#if USE_GS
                obs_data_set_string(setting_source, prop_name.c_str(), m_vecRecTargetIds.at(index).toStdString().c_str());
                obs_data_set_int(setting_source,"method",0);
#else
                obs_data_set_int(setting_source, prop_name.c_str(), index);
                 obs_data_set_bool(setting_source,"compatibility",true);
#endif
#else
                obs_data_set_int(setting_source,prop_name.c_str(),index);
#endif
            }
            else
            {
                obs_data_set_string(setting_source, "window", ele.toStdString().c_str());
            }

#ifdef _WIN32
            obs_data_set_bool(setting_source, "capture_cursor", true);
            QString name = obs_source_get_display_name("crop_filter");
            auto existingFilter = obs_source_get_filter_by_name(captureSource, name.toStdString().c_str());
            if (existingFilter) {
                obs_source_filter_remove(captureSource, existingFilter);
                obs_source_release(existingFilter);
            }
            if (useCrop) {

                auto curCrop = obs_source_create("crop_filter", name.toUtf8().data(), nullptr, nullptr);
                if (curCrop) {
                    obs_source_filter_add(captureSource, curCrop);
                }
                auto cropV = obs_data_create();
                obs_data_set_bool(cropV, "relative", true);
                obs_data_set_int(cropV, "left", LeftCrop);
                obs_data_set_int(cropV, "top", TopCrop);
                obs_data_set_int(cropV, "right", RightCrop);
                obs_data_set_int(cropV, "bottom", BottomCrop);
                obs_data_set_int(cropV, "cx", 0);
                obs_data_set_int(cropV, "cy", 0);
                obs_source_update(curCrop, cropV);
                obs_source_release(curCrop);

            }
#else
            obs_data_set_bool(setting_source, "show_cursor", true);
            if(useCrop)
            {
                obs_data_set_int(setting_source,"cut_top",TopCrop);
                obs_data_set_int(setting_source,"cut_left",LeftCrop);
                obs_data_set_int(setting_source,"cut_right",RightCrop);
                obs_data_set_int(setting_source,"cut_bot",BottomCrop);
            }
            else
            {
                obs_data_set_int(setting_source,"cut_top",0);
                obs_data_set_int(setting_source,"cut_left",0);
                obs_data_set_int(setting_source,"cut_right",0);
                obs_data_set_int(setting_source,"cut_bot",0);
            }
#endif
            obs_source_update(captureSource, setting_source);
            isFind = true;
            break;
        }
        index++;
    }
    return isFind;

}

bool ObsWrapper::resetAudio()
{
    obs_audio_info ai{};
    ai.samples_per_sec = 48000;
    ai.speakers = SPEAKERS_STEREO;

    return obs_reset_audio(&ai);
}

int ObsWrapper::resetVideo(int srcWidth,int srcHeight,int outPutWidth,int outOutHeight,int fps)
{
    obs_video_info ovi{};
    ovi.fps_num = fps;
    ovi.fps_den = 1;

#ifdef _WIN32
    ovi.graphics_module = DL_D3D11;
#else
    ovi.graphics_module = DL_OPENGL;
#endif
    ovi.base_width = srcWidth;
    ovi.base_height = srcHeight;
    ovi.output_width = outPutWidth;
    ovi.output_height = outOutHeight;
    ovi.output_format = VIDEO_FORMAT_I420;
    ovi.colorspace = VIDEO_CS_709;
    ovi.range = VIDEO_RANGE_FULL;
    ovi.adapter = 0;
    ovi.gpu_conversion = true;
    ovi.scale_type = OBS_SCALE_BICUBIC;

    return obs_reset_video(&ovi);
}


bool ObsWrapper::createOutputMode()
{
    if (!fileOutput) {
        //高级输出 ffmpeg
        fileOutput = obs_output_create("ffmpeg_output", "adv_ffmpeg_output", nullptr, nullptr);

        if (!fileOutput)
            return false;
    }

    for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
        char name[9];
        sprintf(name, "adv_aac%d", i);

        if (!CreateAACEncoder(aacTrack[i], aacEncoderID[i], name, i)) {
            return false;
        }

        obs_encoder_set_audio(aacTrack[i], obs_get_audio());
    }

    return true;
}

void ObsWrapper::setupFFmpeg(const QString& storePath,int srcWidth,int srcHeight,int fps,int bitRate)
{
    qDebug()<<"set up ffpmeg with storePath:["
        <<storePath<<"] srcWidth:["<<srcWidth<<"] srcHeight:["
        <<srcHeight<<"] fps:["<<fps<<"] bitRate:["<<bitRate<<"]";
    OBSData settings = obs_data_create();

    QString out_file_name =storePath + "."+RECORD_OUTPUT_FORMAT;

    obs_data_set_string(settings, "url", out_file_name.toUtf8().data());
    obs_data_set_string(settings, "format_name", RECORD_OUTPUT_FORMAT);
    obs_data_set_string(settings, "format_mime_type", RECORD_OUTPUT_FORMAT_MIME);
    obs_data_set_string(settings, "muxer_settings", "movflags=faststart");
    obs_data_set_int(settings, "gop_size", fps * 10);
    obs_data_set_string(settings, "video_encoder", VIDEO_ENCODER_NAME);
    obs_data_set_int(settings, "video_encoder_id", VIDEO_ENCODER_ID);

    obs_data_set_int(settings, "video_bitrate", bitRate * 1024);

    obs_data_set_int(settings, "audio_bitrate", AUDIO_BITRATE);
    obs_data_set_string(settings, "audio_encoder", "aac");
    obs_data_set_int(settings, "audio_encoder_id", AV_CODEC_ID_AAC);
    obs_data_set_string(settings, "audio_settings", nullptr);

    auto srcWidthInUse = srcWidth%2==1?srcWidth+1:srcWidth;
    auto srcHeightInUse = srcHeight%2==1?srcHeight+1:srcHeight;
    obs_data_set_int(settings, "scale_width", srcWidthInUse);
    obs_data_set_int(settings, "scale_height", srcHeightInUse);

    obs_output_set_mixer(fileOutput, 1);  //混流器，如果不设置，可能只有视频没有音频
    obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
    obs_output_update(fileOutput, settings);
}

void ObsWrapper::searchMicDevice()
{
    obs_properties_t *props = obs_get_source_properties(INPUT_AUDIO_SOURCE);
    if (!props)
        return ;
    obs_property_t *devices = obs_properties_get(props,INPUT_AUDIO_PROP_NAME );
    if (!devices)
        return;
    size_t count = obs_property_list_item_count(devices);
    bool is_int = IS_INT;
    std::string str;
    for (size_t i = 0; i < count; i++)
    {
        str = obs_property_list_item_name(devices, i);
        std::string id;
        if (is_int) {
            id = std::to_string(obs_property_list_item_int(devices, i));
        } else {
            const char *val = obs_property_list_item_string(devices, i);
            id = val ? val : "";
        }
        m_micPhoneTargetIds.push_back(QString::fromStdString(id));
        m_micPhoneTargets.push_back(QString::fromStdString(str));
    }
    if(m_micPhoneTargets.isEmpty())
    {
        m_micPhoneTargets.push_back(u8"无设备");
        m_micPhoneTargetIds.push_back("default");
    }
}

void ObsWrapper::searchPlayerDevice()
{
    obs_properties_t *props = obs_get_source_properties(OUTPUT_AUDIO_SOURCE);
    if (!props)
        return ;
    obs_property_t *devices = obs_properties_get(props, OUTPUT_AUDIO_PROP_NAME);
    if (!devices)
        return;
    size_t count = obs_property_list_item_count(devices);
    bool is_int = IS_INT;
    std::string str;
    for (size_t i = 0; i < count; i++)
    {
        str = obs_property_list_item_name(devices, i);
        std::string id;
        if (is_int) {
            id = std::to_string(obs_property_list_item_int(devices, i));
        } else {
            const char *val = obs_property_list_item_string(devices, i);
            id = val ? val : "";
        }
        m_playerTargetIds.push_back(QString::fromStdString(id));
        m_playerTargets.push_back(QString::fromStdString(str));
    }
    if(m_playerTargets.isEmpty())
    {
        m_playerTargets.push_back(u8"无设备");
        m_playerTargetIds.push_back("default");
    }
}


QList<QString> ObsWrapper::playerDeviceName()
{
    if (!playerSource)
        return {"无设备"};
   return m_playerTargets;
}

QList<QString> ObsWrapper::micphoneDeviceName()
{
    if (!micSource)
        return {"无设备"};
    return m_micPhoneTargets;
}

bool ObsWrapper::isRecordingStart() const
{
    return isRecordingStarted;
}

