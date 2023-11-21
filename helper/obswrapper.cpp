#include "obswrapper.h"
#include <string>
#include <QApplication>
#include "libavcodec/avcodec.h"
#include <QDateTime>
#include <QMutex>
#include <QDebug>
using namespace std;


#define DL_D3D11  "libobs-d3d11.dll"
#define DL_OPENGL  "libobs-opengl.dll"

#define INPUT_AUDIO_SOURCE "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"

#define VIDEO_ENCODER_ID           AV_CODEC_ID_H264
#define VIDEO_ENCODER_NAME         "libx264"
#define RECORD_OUTPUT_FORMAT       "mp4"
#define RECORD_OUTPUT_FORMAT_MIME  "video/mp4"
#define VIDEO_FPS            60
#define VIDEO_ENCODER_ID           AV_CODEC_ID_H264
#define AUDIO_BITRATE 128 
#define VIDEO_BITRATE 4 
#define OUT_WIDTH  1920
#define OUT_HEIGHT 1080




enum SOURCE_CHANNELS {
	SOURCE_CHANNEL_TRANSITION,
	SOURCE_CHANNEL_AUDIO_OUTPUT,
	SOURCE_CHANNEL_AUDIO_OUTPUT_2,
	SOURCE_CHANNEL_AUDIO_INPUT,
	SOURCE_CHANNEL_AUDIO_INPUT_2,
	SOURCE_CHANNEL_AUDIO_INPUT_3,
};

inline obs_source_t* getSoundSource(const char* sourceId, const char* deviceId, const char* deviceDesc, int channel)
{
	obs_source_t* source = obs_get_output_source(channel);
	if (!source) {
		obs_data_t* settings = obs_data_create();
		obs_data_set_string(settings, "device_id", deviceId);
		source = obs_source_create(sourceId, deviceDesc, settings,
			nullptr);
		obs_data_release(settings);
	}
	return source;
}

ObsWrapper::ObsWrapper()
{

}

ObsWrapper::~ObsWrapper()
{
	delete micIdentifier;
	delete playerIndentifier;
	obs_fader_remove_callback(mic_obs_fader, OBSVolumeChanged, this);
	obs_volmeter_remove_callback(mic_obs_volmeter, OBSVolumeLevel, this);
	obs_fader_destroy(mic_obs_fader);
	obs_volmeter_destroy(mic_obs_volmeter);
	obs_fader_destroy(player_obs_fader);
	obs_volmeter_destroy(player_obs_volmeter);
	obs_source_release(micSource);
	obs_source_release(playerSource);
	obs_shutdown();
}

void ObsWrapper::OBSVolumeChanged(void* data, float db)
{
	SoundDeviceIdentifier* curIdentifier = static_cast<SoundDeviceIdentifier*>(data);
	//todo: add volume changed event;
	//QMetaObject::invokeMethod(obsFader, "VolumeChanged",Qt::AutoConnection);
	if (curIdentifier)
		delete curIdentifier;
}

void ObsWrapper::OBSVolumeLevel(void* data,
	const float magnitude[MAX_AUDIO_CHANNELS],
	const float peak[MAX_AUDIO_CHANNELS],
	const float inputPeak[MAX_AUDIO_CHANNELS])
{
	//qDebug() << "get" << magnitude[0] << ":" << peak[0] << ":" << inputPeak[0];
	SoundDeviceIdentifier* curIdentifier = static_cast<SoundDeviceIdentifier*>(data);
	if (curIdentifier != nullptr)
	{
		if (curIdentifier->type == SoundDeviceIdentifier::Type::Micphone)
		{
			if (curIdentifier->data != nullptr)
			{
				for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
					curIdentifier->data->currentMagnitudeMic[channelNr] = magnitude[channelNr];
					curIdentifier->data->currentPeakMic[channelNr] = peak[channelNr];
					curIdentifier->data->currentInputPeakMic[channelNr] = inputPeak[channelNr];
				}
				emit curIdentifier->data->MicVolumeDataChange(
					curIdentifier->data->currentMagnitudeMic, 
					curIdentifier->data->currentPeakMic, 
					curIdentifier->data->currentInputPeakMic);
			}

		}
		else {
			if (curIdentifier->data != nullptr)
			{

				for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
					curIdentifier->data->currentMagnitudePlayer[channelNr] = magnitude[channelNr];
					curIdentifier->data->currentPeakPlayer[channelNr] = peak[channelNr];
					curIdentifier->data->currentInputPeakPlayer[channelNr] = inputPeak[channelNr];
				}
				emit curIdentifier->data->PlayerVolumeDataChange(
					curIdentifier->data->currentMagnitudePlayer, 
					curIdentifier->data->currentPeakPlayer,
					curIdentifier->data->currentInputPeakPlayer);
			}

		}
	}
}

void ObsWrapper::VolumeChanged()
{

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
	return (int)audio_output_get_channels(obs_get_audio());
}
void ObsWrapper::updateText()
{

}

bool ObsWrapper::init_obs()
{
	string cfg_path = "./desktop_rec_cfg";

	if (!obs_initialized())
	{
		//初始化obs
		if (!obs_startup("zh-CN", cfg_path.c_str(), NULL))
		{
			return false;
		}

		//加载插件
		QString path = qApp->applicationDirPath();
		string path_str = path.toStdString();

		string plugin_path = path_str + "/obs-plugins/64bit";
		string data_path = path_str + "/data/obs-plugins/%module%";

		obs_add_module_path(plugin_path.c_str(), data_path.c_str());

		obs_load_all_modules();

	}


	//音频设置
	if (!ResetAudio())
		return false;

	//视频设置
	if (ResetVideo() != OBS_VIDEO_SUCCESS)
		return false;

	if (!create_output_mode())
		return false;

	//关联默认扬声器和麦克风的音频回调
	micSource = getSoundSource(INPUT_AUDIO_SOURCE, "default", "Default Mic/Aux", SOURCE_CHANNEL_AUDIO_INPUT);

	playerSource = getSoundSource(OUTPUT_AUDIO_SOURCE, "default", "Default Desktop Audio", SOURCE_CHANNEL_AUDIO_OUTPUT);

	mic_obs_fader = obs_fader_create(OBS_FADER_LOG);
	mic_obs_volmeter = obs_volmeter_create(OBS_FADER_LOG);
	player_obs_fader = obs_fader_create(OBS_FADER_LOG);
	player_obs_volmeter = obs_volmeter_create(OBS_FADER_LOG);
	micIdentifier = new SoundDeviceIdentifier(SoundDeviceIdentifier::Type::Micphone, this, mic_obs_volmeter);
	playerIndentifier = new SoundDeviceIdentifier(SoundDeviceIdentifier::Type::Player, this, player_obs_volmeter);
	obs_fader_add_callback(mic_obs_fader, OBSVolumeChanged, micIdentifier);
	obs_volmeter_add_callback(mic_obs_volmeter, OBSVolumeLevel, micIdentifier);
	obs_fader_add_callback(player_obs_fader, OBSVolumeChanged, playerIndentifier);
	obs_volmeter_add_callback(player_obs_volmeter, OBSVolumeLevel, playerIndentifier);

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

int ObsWrapper::start_rec()
{
	SetupFFmpeg();

	if (!obs_output_start(fileOutput))
		return -1;

	return 0;
}

int ObsWrapper::stop_rec()
{
	bool force = false;

	if (force)
		obs_output_force_stop(fileOutput);
	else
		obs_output_stop(fileOutput);

	return 0;
}

static void AddSource(void* _data, obs_scene_t* scene)
{
	obs_source_t* source = (obs_source_t*)_data;
	obs_scene_add(scene, source);
	obs_source_release(source);
}

int ObsWrapper::add_scene_source(REC_TYPE type)
{
	obs_set_output_source(SOURCE_CHANNEL_TRANSITION, nullptr);
	obs_set_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT, nullptr);
	obs_set_output_source(SOURCE_CHANNEL_AUDIO_INPUT, nullptr);

	size_t idx = 0;
	const char* id;

	/* automatically add transitions that have no configuration (things
	 * such as cut/fade/etc) */
	while (obs_enum_transition_types(idx++, &id)) {
		const char* name = obs_source_get_display_name(id);

		if (!obs_is_source_configurable(id)) {
			obs_source_t* tr = obs_source_create_private(id, name, NULL);

			if (strcmp(id, "fade_transition") == 0)
				fadeTransition = tr;
		}
	}

	if (!fadeTransition)
	{
		return -1;
	}

	obs_set_output_source(SOURCE_CHANNEL_TRANSITION, fadeTransition);
	obs_source_release(fadeTransition);

	scene = obs_scene_create("MyScene");
	if (!scene)
	{
		return -2;
	}

	obs_source_t* s = obs_get_output_source(SOURCE_CHANNEL_TRANSITION);
	obs_transition_set(s, obs_scene_get_source(scene));
	obs_source_release(s);

	//创建源：显示器采集
	if (type == REC_DESKTOP)
		captureSource = obs_source_create("monitor_capture", "Computer_Monitor_Capture", NULL, nullptr);
	else
		captureSource = obs_source_create("window_capture", "Window_Capture", NULL, nullptr);
	if (captureSource)
	{
		obs_scene_atomic_update(scene, AddSource, captureSource);
	}
	else
	{
		return -3;
	}

	// 设置窗口捕获原的窗口或显示器
	setting_source = obs_data_create();
	obs_data_t* curSetting = obs_source_get_settings(captureSource);
	obs_data_apply(setting_source, curSetting);
	obs_data_release(curSetting);

	properties = obs_source_properties(captureSource);
	property = obs_properties_first(properties);
	return 0;
}



void ResetAudioDevice(const char* sourceId, const char* deviceId,
	const char* deviceDesc, int channel)
{
	bool disable = deviceId && strcmp(deviceId, "disabled") == 0;
	obs_source_t* source = getSoundSource(sourceId, deviceId, deviceDesc, channel);

	if (source) {
		if (disable) {
			obs_set_output_source(channel, nullptr);
		}
		else {
			obs_set_output_source(channel, source);
			auto settings = obs_source_get_settings(source);
			const char* oldId =
				obs_data_get_string(settings, "device_id");
			if (strcmp(oldId, deviceId) != 0) {
				obs_data_set_string(settings, "device_id",
					deviceId);
				obs_source_update(source, settings);
			}
			obs_data_release(settings);
		}
		obs_source_release(source);
	}
	else {
		qDebug() << "get null source";
	}
}

static inline bool HasAudioDevices(const char* source_id)
{
	const char* output_id = source_id;
	obs_properties_t* props = obs_get_source_properties(output_id);
	size_t count = 0;

	if (!props)
		return false;

	obs_property_t* devices = obs_properties_get(props, "device_id");
	if (devices)
		count = obs_property_list_item_count(devices);

	obs_properties_destroy(props);

	return count != 0;
}

static bool CreateAACEncoder(OBSEncoder& res, string& id,
	const char* name, size_t idx)
{
	const char* id_ = "ffmpeg_aac";

	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);

	if (res) {
		obs_encoder_release(res);
		return true;
	}

	return false;
}

void ObsWrapper::rec_system_audio()
{
	bool hasDesktopAudio = HasAudioDevices(OUTPUT_AUDIO_SOURCE);

	if (hasDesktopAudio)
		ResetAudioDevice(OUTPUT_AUDIO_SOURCE, "default",
			"Default Desktop Audio", SOURCE_CHANNEL_AUDIO_OUTPUT);
}

void ObsWrapper::rec_out_audio()
{
	bool hasInputAudio = HasAudioDevices(INPUT_AUDIO_SOURCE);
	if (hasInputAudio)
		ResetAudioDevice(INPUT_AUDIO_SOURCE, "default",
			"Default Mic/Aux", SOURCE_CHANNEL_AUDIO_INPUT);
}

void ObsWrapper::SearchRecTargets(REC_TYPE type)
{
	m_vecRecTargets.clear();

	const char* rec_type_name = nullptr;
	if (type == REC_WINDOWS)
	{
		rec_type_name = "window";
	}
	else
	{
		rec_type_name = "monitor";
	}

	while (property)
	{
		const char* name = obs_property_name(property);

		if (strcmp(name, rec_type_name) == 0)
		{
			size_t count = obs_property_list_item_count(property);
			const char* string = nullptr;

			for (size_t i = 0; i < count; i++)
			{
				if (type == REC_WINDOWS)
				{
					string = obs_property_list_item_string(property, i);
				}
				else
				{
					const char* item_name = obs_property_list_item_name(property, i);
					string = item_name;
				}

				m_vecRecTargets.push_back(string);
			}
		}

		obs_property_next(&property);
	}
}

void ObsWrapper::UpdateRecItem(const char* target, REC_TYPE type, bool useCrop)
{
	for (auto ele : m_vecRecTargets)
	{
		if (ele == QString(target))
		{
			if (type == REC_DESKTOP)
				obs_data_set_string(setting_source, "monitor", target);
			else
				obs_data_set_string(setting_source, "window", target);


			obs_source_update(captureSource, setting_source);
			QString name = obs_source_get_display_name("crop_filter");
			auto existingFilter = obs_source_get_filter_by_name(captureSource, name.toUtf8().data());
			if (useCrop)
			{
				if (!existingFilter)
				{
					auto curCrop = obs_source_create("crop_filter", name.toUtf8().data(), nullptr, nullptr);
					if (curCrop)
					{
						obs_source_filter_add(captureSource, curCrop);
					}
					auto cropV = obs_data_create();
					obs_data_set_bool(cropV, "relative", true);
					obs_data_set_int(cropV, "left", 1200);
					obs_data_set_int(cropV, "top", 1000);
					obs_data_set_int(cropV, "right", 100);
					obs_data_set_int(cropV, "bottom", 200);
					obs_data_set_int(cropV, "cx", 0);
					obs_data_set_int(cropV, "cy", 0);
					obs_source_update(curCrop, cropV);
				}
			}
			else
			{
				if (existingFilter)
				{
					obs_source_filter_remove(captureSource, existingFilter);
				}
			}
			break;
		}
	}

	obs_data_release(setting_source);
}

bool ObsWrapper::ResetAudio()
{
	struct obs_audio_info ai;
	ai.samples_per_sec = 48000;
	ai.speakers = SPEAKERS_STEREO;

	return obs_reset_audio(&ai);
}

int ObsWrapper::ResetVideo()
{
	struct obs_video_info ovi;
	ovi.fps_num = VIDEO_FPS;
	ovi.fps_den = 1;

	ovi.graphics_module = DL_D3D11;
	ovi.base_width = 3860;
	ovi.base_height = 2140;
	ovi.output_width = 1920;
	ovi.output_height = 1080;
	ovi.output_format = VIDEO_FORMAT_I420;
	ovi.colorspace = VIDEO_CS_709;
	ovi.range = VIDEO_RANGE_FULL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BICUBIC;

	return obs_reset_video(&ovi);
}

bool ObsWrapper::create_output_mode()
{
	if (!fileOutput)
	{
		//高级输出 ffmpeg
		fileOutput = obs_output_create("ffmpeg_output", "adv_ffmpeg_output", nullptr, nullptr);

		if (!fileOutput)
			return false;
	}

	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		char name[9];
		sprintf(name, "adv_aac%d", i);

		if (!CreateAACEncoder(aacTrack[i], aacEncoderID[i], name, i))
		{
			return false;
		}

		obs_encoder_set_audio(aacTrack[i], obs_get_audio());
	}

	return true;
}

void ObsWrapper::SetupFFmpeg()
{
	obs_data_t* settings = obs_data_create();

	QDateTime dt = QDateTime::currentDateTime();

	QString timestr = dt.toString("yyyy_MM_dd_hh_mm_ss");

	QString path = "D:/";

	QString out_file_name = path + timestr + ".mp4";

	obs_data_set_string(settings, "url", out_file_name.toUtf8().data());
	obs_data_set_string(settings, "format_name", RECORD_OUTPUT_FORMAT);
	obs_data_set_string(settings, "format_mime_type", RECORD_OUTPUT_FORMAT_MIME);
	obs_data_set_string(settings, "muxer_settings", "movflags=faststart");
	obs_data_set_int(settings, "gop_size", VIDEO_FPS * 10);
	obs_data_set_string(settings, "video_encoder", VIDEO_ENCODER_NAME);
	obs_data_set_int(settings, "video_encoder_id", VIDEO_ENCODER_ID);

	if (VIDEO_ENCODER_ID == AV_CODEC_ID_H264)
		obs_data_set_string(settings, "video_settings", "profile=main x264-params=crf=22");
	else if (VIDEO_ENCODER_ID == AV_CODEC_ID_FLV1)
		obs_data_set_int(settings, "video_bitrate", VIDEO_BITRATE * 1024);

	obs_data_set_int(settings, "audio_bitrate", AUDIO_BITRATE);
	obs_data_set_string(settings, "audio_encoder", "aac");
	obs_data_set_int(settings, "audio_encoder_id", AV_CODEC_ID_AAC);
	obs_data_set_string(settings, "audio_settings", NULL);

	obs_data_set_int(settings, "scale_width", OUT_WIDTH);
	obs_data_set_int(settings, "scale_height", OUT_HEIGHT);

	obs_output_set_mixer(fileOutput, 1);  //混流器，如果不设置，可能只有视频没有音频
	obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
	obs_output_update(fileOutput, settings);

	obs_data_release(settings);
}

