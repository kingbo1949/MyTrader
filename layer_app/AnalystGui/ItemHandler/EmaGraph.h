#pragma once
#include "AvgValueGraph.h"
// #include <Calculator_EMa.h>
class CEmaGraph : public CAvgValueGraph
{
	Q_OBJECT

public:
	CEmaGraph(QCustomPlot* parent);
	virtual ~CEmaGraph() { ; };


protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final;


};

