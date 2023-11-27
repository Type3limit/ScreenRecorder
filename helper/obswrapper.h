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

enum RecordingStatus
{
	Recording = 0,
	Paused = 1,
	Stoped = 2,
};

struct SoundDeviceIdentifier;

class ObsWrapper:public QObject
{
	Q_OBJECT
public:
signals:
	void MicVolumeDataChange(const float* magnitude, const float* peak, const float* inputPeak);
	void PlayerVolumeDataChange(const float* magnitude, const float* peak, const float* inputPeak);
	void recordStatusChanged(int recordingStatus);
public:
	ObsWrapper();
	~ObsWrapper();
    void release() ;
	static void OBSVolumeChanged(void* data, float db);

	static void OBSVolumeLevel(void* data, const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS]);

	void VolumeChanged();
	int micChannelCount();
	int playerChannelCount();
	int audioChannel();

	bool initObs(int srcWidth,int srcHeight,int fps);
	int  startRecording();
	int  stopRecording(bool isForceStop = false);

    int pauseRecording();

	int  addSceneSource(REC_TYPE type);
	void recSystemAudio();
	void recOutAudio();
	void SearchRecTargets(REC_TYPE type);
	bool UpdateRecItem(const char* target, REC_TYPE type,bool useCrop = false,
		int LeftCrop = 0,
		int RightCrop = 0,
		int TopCrop = 0,
		int BottomCrop = 0);
	QList<QString> getRecTargets() const
	{
		return m_vecRecTargets;
	}

    QString playerDeviceName();
    QString micphoneDeviceName();
	bool isRecordingStart() const;
	bool ResetAudio();
	int ResetVideo(int srcWidth,int srcHeight,int outPutWidth,int outOutHeight,int fps);
	void setupFFmpeg(const QString& storePath,int srcWidth,int srcHeight,int fps,int bitRate);
private:


	bool createOutputMode();


private:

    bool isRecordingStarted = false;
    bool isPaused = false;

	OBSOutput fileOutput;
	obs_source_t* fadeTransition = nullptr;
	obs_scene_t* scene = nullptr;

	obs_source_t* micSource;

    obs_source_t* playerSource;

	obs_source_t* captureSource;


	OBSEncoder aacTrack[MAX_AUDIO_MIXES];
	std::string aacEncoderID[MAX_AUDIO_MIXES];
	obs_fader_t * mic_obs_fader;
	obs_volmeter_t * mic_obs_volmeter;
	obs_fader_t * player_obs_fader;
	obs_volmeter_t * player_obs_volmeter;
	QList<QString> m_vecRecTargets;

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
