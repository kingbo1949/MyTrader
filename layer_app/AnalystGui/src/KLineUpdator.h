#pragma once
#include "Factory_AnalystGui.h"


typedef QSharedPointer<QCPFinancialDataContainer> YContainerQSPtr;

class CKLineUpdator
{
public:
	CKLineUpdator();
	virtual ~CKLineUpdator() { ; };

	// suit：已经更新到界面的kline组
	// klines：本批更新数据
	// 返回值：本批数据里全新数据的数目
	// 更新k线 返回值是添加的数目
	int						Update(const IBKLinePtrs& klines, KlinePlotSuit& suit);

protected:

	void					AddToSuit(IBKLinePtr kline, KlinePlotSuit& suit);
	void					UpdateToSuit(IBKLinePtr kline, int pos, KlinePlotSuit& suit);

};
typedef std::shared_ptr<CKLineUpdator> KLineUpdatorPtr;
