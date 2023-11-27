#include "volumecontrol.h"
#include <QDateTime>
#include <QPainter>
#include <QDebug>
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define M_INFINITE 3.4e38f
#define ITEM_DISTANCE 6
#define TOP_DISTANCE 5
#define COLUMN_WIDTH 2
VolumeControl::VolumeControl(QWidget* widget)
	:QWidget(widget)
{
	updateTimer.setInterval(40);
	updateTimer.setTimerType(Qt::PreciseTimer);
	updateTimer.start();
	connect(&updateTimer, &QTimer::timeout, this, [&]() {update(); });
}

VolumeControl::~VolumeControl()
{
	updateTimer.stop();
}

void VolumeControl::setChannelCount(int curChannelCount)
{
	if (curChannelCount > 8)
		curChannelCount = 8;
	this->channelCount = curChannelCount;
}

void VolumeControl::setAudioChannel(int channel)
{
	this->audioChannelCount = channel;
}

void VolumeControl::setLevels(const float* magnitude,
	const float* peak,
	const float* inputPeak)
{
	
	QMutexLocker locker(&dataMutex);
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = magnitude[channelNr];
		currentPeak[channelNr] = peak[channelNr];
		currentInputPeak[channelNr] = inputPeak[channelNr];
	}
	locker.unlock();
}



inline void VolumeControl::resetLevels()
{
	currentLastUpdateTime = 0;
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = -M_INFINITE;
		currentPeak[channelNr] = -M_INFINITE;
		currentInputPeak[channelNr] = -M_INFINITE;

		displayMagnitude[channelNr] = -M_INFINITE;
		displayPeak[channelNr] = -M_INFINITE;
		displayPeakHold[channelNr] = -M_INFINITE;
		displayPeakHoldLastUpdateTime[channelNr] = 0;
		displayInputPeakHold[channelNr] = -M_INFINITE;
		displayInputPeakHoldLastUpdateTime[channelNr] = 0;
	}
}
inline bool VolumeControl::detectIdle(uint64_t ts)
{
	double timeSinceLastUpdate = (ts - currentLastUpdateTime) * 0.000000001;
	if (timeSinceLastUpdate > 0.5) {
		resetLevels();
		return true;
	}
	else {
		return false;
	}
}

inline int convertToInt(float number)
{
	constexpr int min = std::numeric_limits<int>::min();
	constexpr int max = std::numeric_limits<int>::max();

	// NOTE: Conversion from 'const int' to 'float' changes max value from 2147483647 to 2147483648
	if (number >= (float)max)
		return max;
	else if (number < min)
		return min;
	else
		return int(number);
}

void VolumeControl::paintEvent(QPaintEvent* event)
{

	QRect widgetRect = rect();
	int width = widgetRect.width();
	int height = widgetRect.height();
	QPainter painter(this);
	painter.setPen(QPen(QColor(15, 15, 15),1));
	painter.setBrush(QColor(33, 33, 38));
	painter.drawRoundedRect(widgetRect, 4, 4);

	if (width > 8)
	{
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(57, 57, 61));

		auto drawCount = width / (ITEM_DISTANCE + COLUMN_WIDTH);
		for (int i = 1; i <= drawCount; i++)
		{
			painter.drawRoundedRect(QRectF{ (qreal)i * (ITEM_DISTANCE + COLUMN_WIDTH),(qreal)widgetRect.y() + TOP_DISTANCE,COLUMN_WIDTH,(qreal)widgetRect.height() - (2 * TOP_DISTANCE) }, 1, 1);
		}
		qreal usedMagnitude = currentMagnitude[0];
		qreal usedPeak = currentPeak[0];
		qreal usedPeakHolder = currentInputPeak[0];
	
		qreal scale = width / minimumLevel;
		int magnitudePos = width - usedMagnitude * scale;
		int peakPos =  width - convertToInt(usedPeak * scale);
		int peakHolderPos = width- convertToInt(usedPeakHolder * scale);
		//draw magnitude
		drawCount = magnitudePos / (ITEM_DISTANCE + COLUMN_WIDTH);
		painter.setBrush(QColor(89, 103, 242));
		for (int i = 1; i <= drawCount; i++)
		{
			painter.drawRoundedRect(QRectF{ (qreal)i * (ITEM_DISTANCE + COLUMN_WIDTH),(qreal)widgetRect.y() + TOP_DISTANCE,COLUMN_WIDTH,(qreal)widgetRect.height() - (2 * TOP_DISTANCE) }, 1, 1);
		}
		//draw peak
		drawCount = peakPos / (ITEM_DISTANCE + COLUMN_WIDTH);
		painter.setBrush(Qt::green);
		if (drawCount > 0)
		{
			painter.drawRoundedRect(QRectF{ (qreal)drawCount * (ITEM_DISTANCE + COLUMN_WIDTH),(qreal)widgetRect.y() + TOP_DISTANCE,COLUMN_WIDTH,(qreal)widgetRect.height() - (2 * TOP_DISTANCE) }, 1, 1);
		}
		//draw peakholder
		drawCount = peakHolderPos / (ITEM_DISTANCE + COLUMN_WIDTH);
		painter.setBrush(Qt::red);
		if (drawCount > 0)
		{
			painter.drawRoundedRect(QRectF{ (qreal)drawCount * (ITEM_DISTANCE + COLUMN_WIDTH),(qreal)widgetRect.y() + TOP_DISTANCE,COLUMN_WIDTH,(qreal)widgetRect.height() - (2 * TOP_DISTANCE) }, 1, 1);
		}
	}
}

