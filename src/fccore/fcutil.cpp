#include "fcutil.h"
#include <QFile>
extern "C"
{
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
}

void FCUtil::printAVFilterGraph(const QString &filePath, AVFilterGraph *graph)
{
	int i, j;
	QFile file(filePath);
	file.open(QFile::WriteOnly);
	
	file.write("digraph G {\n");
	file.write("node [shape=box]\n");
	file.write("rankdir=LR\n");

	for (i = 0; i < graph->nb_filters; i++) {
		char filter_ctx_label[128];
		const AVFilterContext *filter_ctx = graph->filters[i];

		snprintf(filter_ctx_label, sizeof(filter_ctx_label), "%s\\n(%s)",
			filter_ctx->name,
			filter_ctx->filter->name);

		for (j = 0; j < filter_ctx->nb_outputs; j++) {
			AVFilterLink *link = filter_ctx->outputs[j];
			if (link) {
				char dst_filter_ctx_label[128];
				const AVFilterContext *dst_filter_ctx = link->dst;

				snprintf(dst_filter_ctx_label, sizeof(dst_filter_ctx_label),
					"%s\\n(%s)",
					dst_filter_ctx->name,
					dst_filter_ctx->filter->name);

				file.write(QString("\"%1\" -> \"%2\" [ label= \"inpad:%3 -> outpad:%4\\n")
							.arg(filter_ctx_label).arg( dst_filter_ctx_label)
							.arg(avfilter_pad_get_name(link->srcpad, 0))
							.arg(avfilter_pad_get_name(link->dstpad, 0)).toUtf8());

				if (link->type == AVMEDIA_TYPE_VIDEO) {
					const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)link->format);
					file.write(QString("fmt:%1 w:%2 h:%3 tb:%4/%5")
							.arg(desc->name).arg(link->w).arg(link->h)
							.arg(link->time_base.num).arg(link->time_base.den)
							.toUtf8());
				}
				else if (link->type == AVMEDIA_TYPE_AUDIO) {
					char buf[255];
					av_get_channel_layout_string(buf, sizeof(buf), -1,
						link->channel_layout);
					file.write(QString("fmt:%1 sr:%2 cl:%3 tb:%4/%5")
								.arg(av_get_sample_fmt_name((AVSampleFormat)link->format))
								.arg(link->sample_rate).arg(buf).arg(link->time_base.num)
								.arg( link->time_base.den).toUtf8());
				}
				file.write("\" ];\n");
			}
		}
	}
	file.write("}\n");
}