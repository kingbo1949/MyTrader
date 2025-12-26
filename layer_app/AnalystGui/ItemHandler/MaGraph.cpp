#include "MaGraph.h"
#include <Factory_QDatabase.h>
CMaGraph::CMaGraph(QCustomPlot* parent)
	:CAvgValueGraph(parent)
{
}


bool CMaGraph::MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos)
{
	if (klines.empty()) return false;

	QQuery query;
	query.query_type = QQueryType::BETWEEN_TIME;
	query.time_pair.beginPos = klines[beginPos]->time;
	query.time_pair.endPos = klines.back()->time + 10;

	IBAvgValuePtrs avgValues = MakeAndGet_QDatabase()->GetMas(codeId, timetype, query);

	if (avgValues.empty()) return false;
	if (avgValues.back()->time != klines.back()->time) return false;
	if (avgValues[0]->time != klines[beginPos]->time) return false;

	for (auto value : avgValues)
	{
		m_mapAvgValue[value->time] = value;
	}
	return true;
}


