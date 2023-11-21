#pragma once
#include <QWidget>
#include <QMutex>
#include <QTimer>
#define MAX_AUDIO_MIXES 6
#define MAX_AUDIO_CHANNELS 8
class VolumeControl :public QWidget
{
	Q_OBJECT
public:
	VolumeControl(QWidget* widget = nullptr);
	~VolumeControl();
public:
	void setChannelCount(int channelCount);
	void setAudioChannel(int channel);
	void setLevels(const float* magnitude, const float* peak, const float* inputPeak);
protected:
	void resetLevels();
	bool detectIdle(uint64_t ts);
	void paintEvent(QPaintEvent *event) override;
private:

	void calculateBallistics(uint64_t ts, qreal timeSinceLastRedraw = 0.0);

	void calculateBallisticsForChannel(int channelNr, uint64_t ts, qreal timeSinceLastRedraw);
private:
	int channelCount = 8;
	int audioChannelCount = 0;
	//音频绘制
	float currentMagnitude[MAX_AUDIO_CHANNELS]{0};
	float currentPeak[MAX_AUDIO_CHANNELS]{0};
	float currentInputPeak[MAX_AUDIO_CHANNELS]{0};
	float displayMagnitude[MAX_AUDIO_CHANNELS]{0};
	float displayPeak[MAX_AUDIO_CHANNELS]{0};
	float displayPeakHold[MAX_AUDIO_CHANNELS]{0};
	uint64_t displayPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS]{0};
	float displayInputPeakHold[MAX_AUDIO_CHANNELS]{0};
	uint64_t displayInputPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS]{0};

	QMutex dataMutex;
	uint64_t lastRedrawTime = 0;
	uint64_t currentLastUpdateTime = 0;
	qreal peakHoldDuration = 20.0;                 //  20 seconds
	qreal inputPeakHoldDuration = 1.0;             //  1 second
	qreal magnitudeIntegrationTime = 0.3;          //  99% in 300 ms
	qreal minimumLevel = -90.0;                    // -60 dB
	QTimer updateTimer;
};


