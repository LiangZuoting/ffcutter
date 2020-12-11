#pragma once

#include <QString>
#include <QDebug>
extern "C"
{
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
};