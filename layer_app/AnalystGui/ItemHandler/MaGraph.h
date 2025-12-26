#pragma once
#include "AvgValueGraph.h"
#include <Factory_Calculator.h>
class CMaGraph : public CAvgValueGraph
{
	Q_OBJECT

public:
	CMaGraph(QCustomPlot* parent);
	virtual ~CMaGraph() { ; };


protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final;



};

