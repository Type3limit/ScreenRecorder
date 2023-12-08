#pragma once

#include "obs.h"
#include "obs.hpp"
#include <QObject>
#include <QString>
#include <QMutex>
#include <QList>
#include <QSharedPointer>
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
	///micphone volume level change signal
	void micVolumeDataChange(const float* magnitude, const float* peak, const float* inputPeak);
	///player volume level change signal
	void playerVolumeDataChange(const float* magnitude, const float* peak, const float* inputPeak);
	///record status changed
	void recordStatusChanged(int recordingStatus);
public:
	ObsWrapper();
	~ObsWrapper();

#pragma region obs
	///initilize obs realted
	bool initObs(int srcWidth,int srcHeight,int fps);
	///release obs related soruces
	void release() ;
#pragma endregion

#pragma region audio
	///to find all input audio device
	void searchMicDevice();
	///to get micphone device names
	QList<QString> micphoneDeviceName();
	///to find all output audio device
	void searchPlayerDevice();
	///to get player device names
	QList<QString> playerDeviceName();
	///volume change callback
	static void oBSVolumeChanged(void* data, float db);
	///volume level change callback
	static void oBSVolumeLevel(void* data, const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS]);
	///micphone channel count
	int micChannelCount();
	///player channel count
	int playerChannelCount();
	///current output audio channel count
	int audioChannel();
	///to record player audio
	void recPlayerAudio(bool enable = true,const QString& target="default");
	///to record micphone audio
	void recMicAudio(bool enable = true,const QString& target="default");
	///use micphone device name to get micphone device id
	QString micDeviceId(const QString& name);
	///use player device name to get player device id
	QString playerDeviceId(const QString& name);
	///make micVolumeDataChange with target device
	void resetMicphoneVolumeLevelCallback(const QString& target);
	///make playerVolumeDataChange with target device
	void resetPlayerVolumeLevelCallback(const QString& target);
#pragma endregion

#pragma region recording
	///setup with advance ffmpeg output
	void setupFFmpeg(const QString& storePath,int srcWidth,int srcHeight,int fps,int bitRate);
	///recording start
	int  startRecording();
	///to identify recording is started
	bool isRecordingStart() const;
	///recording stop
	int  stopRecording(bool isForceStop = false);
	///recording pause
    int pauseRecording();
	///obs output callback
	static void outPutStopedCallback(void *my_data, calldata_t *cd);
	///to find window/minotor device
	void searchRecTargets(REC_TYPE type);
	///get recording items based on searchRecTargets
	QList<QString> getRecTargets();
	///get recording item ids
	QList<QString> getRecTargetIds();
	///use target as recording source,and also update clip region inneed
	bool updateRecItem(const char* target, REC_TYPE type,bool useCrop = false,
		int LeftCrop = 0,
		int RightCrop = 0,
		int TopCrop = 0,
		int BottomCrop = 0);
#pragma endregion

#pragma region scene/source
	///add target source
	int  addSceneSource(REC_TYPE type);
	///reset audio device
	void ResetAudioDevice(const char* sourceId, const char* deviceId, const char* deviceDesc, int channel);
	///reset audio output status
	bool resetAudio();
	///reset video output canvas size and fps
	int resetVideo(int srcWidth,int srcHeight,int outPutWidth,int outOutHeight,int fps);

#pragma endregion

private:
	///setup output source
	bool createOutputMode();
public:
	///current micphone audio source
	OBSSource micSource;
	///current player audio source
	OBSSource playerSource;
	///current desktop source
	OBSSource captureSource;

private:
	///mark if recording started
    bool isRecordingStarted = false;
	///mark if is in pause status
    bool isPaused = false;
	///sources to output files
	OBSOutput fileOutput;
	///fade transition
	OBSSource fadeTransition = nullptr;
	/// current scene
	OBSScene scene = nullptr;
	/// to draw current capture images
	OBSDisplay displayer;
	///ffmpeg aac encoder tracks
	OBSEncoder aacTrack[MAX_AUDIO_MIXES];
	///signal handler
	OBSSignal handler;

	std::string aacEncoderID[MAX_AUDIO_MIXES];

	OBSFader mic_obs_fader;
	OBSVolMeter mic_obs_volmeter;
	OBSFader player_obs_fader;
	OBSVolMeter player_obs_volmeter;

	QList<QString> m_vecRecTargets;
	QList<QString> m_vecRecTargetIds;

	QList<QString> m_micPhoneTargets;
	QList<QString> m_micPhoneTargetIds;

	QList<QString> m_playerTargets;
	QList<QString> m_playerTargetIds;

	QSharedPointer<SoundDeviceIdentifier> micIdentifier = nullptr;
	QSharedPointer<SoundDeviceIdentifier> playerIndentifier = nullptr;

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
