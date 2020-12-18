#pragma once

#include <QString>
#include <QDebug>
#include <QTime>
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavutil/error.h>
}

class FCUtil
{
public:
	template <class... Args>
	static void printAVError(int error, Args&& ...args)
	{
		auto c = qCritical();
		(c << ... << std::forward<Args>(args));
		char buf[AV_ERROR_MAX_STRING_SIZE]{ 0 };
		av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, error);
		c << ",errno=" << error << "," << buf;
	}

	static void printAVFilterGraph(const QString &filePath, AVFilterGraph *graph);

	static double durationSecs(const QTime& from, const QTime &to)
	{
		double msecs = from.msecsTo(to);
		return msecs / 1000;
	}
};