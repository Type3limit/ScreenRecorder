#pragma once

#include "obs.h"
#include "obs.hpp"
#include <QObject>
#include <QString>
#include <QMutex>
#include <QList>
using namespace std;

enum REC_TYPE
{
	REC_WINDOWS,
	REC_DESKTOP
};

struct SoundDeviceIdentifier;

class ObsWrapper:public QObject
{
	Q_OBJECT
public:
signals:
		void MicVolumeDataChange(const float* magnitude, const float* peak, const float* inputPeak);
		void PlayerVolumeDataChange(const float* magnitude, const float* peak, const float* inputPeak);
public:
	ObsWrapper();
	~ObsWrapper();

	static void OBSVolumeChanged(void* data, float db);

	static void OBSVolumeLevel(void* data, const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS]);

	void VolumeChanged();
	int micChannelCount();
	int playerChannelCount();
	int audioChannel();

	void updateText();

	bool init_obs();
	int  start_rec();
	int  stop_rec();

	int  add_scene_source(REC_TYPE type);
	void rec_system_audio();
	void rec_out_audio();
	void SearchRecTargets(REC_TYPE type);
	void UpdateRecItem(const char* target, REC_TYPE type,bool useCrop = false);
	void UpdateMicAudioCache();
	void UpdatePlayerAudioCache();
	QList<QString> getRecTargets() const
	{
		return m_vecRecTargets;
	}

private:
	bool ResetAudio();
	int ResetVideo();
	bool create_output_mode();
	void SetupFFmpeg();

private:
	OBSOutput fileOutput;
	obs_source_t* fadeTransition = nullptr;
	obs_scene_t* scene = nullptr;

	obs_source_t* captureSource;

	obs_source_t* micSource;

    obs_source_t* playerSource;

	obs_properties_t* properties;
	OBSEncoder aacTrack[MAX_AUDIO_MIXES];
	std::string aacEncoderID[MAX_AUDIO_MIXES];
	obs_fader_t * mic_obs_fader;
	obs_volmeter_t * mic_obs_volmeter;
	obs_fader_t * player_obs_fader;
	obs_volmeter_t * player_obs_volmeter;
	QList<QString> m_vecRecTargets;

	obs_property_t* property = nullptr;
	obs_data_t* setting_source = nullptr;
	SoundDeviceIdentifier* micIdentifier = nullptr;
	SoundDeviceIdentifier* playerIndentifier = nullptr;

	float currentMagnitudeMic[MAX_AUDIO_CHANNELS]{0};
	float currentPeakMic[MAX_AUDIO_CHANNELS]{0};
	float currentInputPeakMic[MAX_AUDIO_CHANNELS]{0};

	float currentMagnitudePlayer[MAX_AUDIO_CHANNELS]{0};
	float currentPeakPlayer[MAX_AUDIO_CHANNELS]{0};
	float currentInputPeakPlayer[MAX_AUDIO_CHANNELS]{0};
};

struct SoundDeviceIdentifier
{
	enum Type
	{
		Micphone,
		Player
	};
	Type type;
	ObsWrapper* data;
	void * source;
	SoundDeviceIdentifier(Type curType,ObsWrapper* curData,void* curSource):type(curType),data(curData),source(curSource){}
	~SoundDeviceIdentifier(){}
};
