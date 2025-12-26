#pragma once
#include "AvgValueGraph.h"
class CVwMaGraph : public CAvgValueGraph
{
	Q_OBJECT

public:
	CVwMaGraph(QCustomPlot* parent);
	virtual ~CVwMaGraph() { ; };


protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final;

};

