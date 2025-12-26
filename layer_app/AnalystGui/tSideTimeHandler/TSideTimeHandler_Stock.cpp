#include "TSideTimeHandler_Stock.h"
#include <Factory_IBGlobalShare.h>
#include "../src/Factory_AnalystGui.h"
// 股票的盘后时间：16：00 - 20：00 夜盘时间：04：00 - 09：30
CTSideTimeHandler_Stock::CTSideTimeHandler_Stock()
{
	m_open = CHighFrequencyGlobalFunc::MakeMilliSecondPart("09:30:00", 0);
	m_close = CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0);

	m_afterOpen = CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0);
	m_afterClose = CHighFrequencyGlobalFunc::MakeMilliSecondPart("20:00:00", 0);

	m_nightOpen = CHighFrequencyGlobalFunc::MakeMilliSecondPart("04:00:00", 0);
	m_nightClose = CHighFrequencyGlobalFunc::MakeMilliSecondPart("09:30:00", 0);
}

QColor CTSideTimeHandler_Stock::GetTSideColor(TradeSideType type)
{
	if (type == TradeSideType::AfterTrade) return MakeAndGet_ColorManager()->GetColor().tradeSide1;
	if (type == TradeSideType::NightStock) return MakeAndGet_ColorManager()->GetColor().tradeSide2;

	return MakeAndGet_ColorManager()->GetColor().backGround;
}


TradeSideType CTSideTimeHandler_Stock::GetTSideType(Tick_T time)
{
	// 把time转换为当日的时分秒
	Tick_T hhmmss = time - CHighFrequencyGlobalFunc::GetDayMillisec(time);

	if (m_open <= hhmmss && hhmmss < m_close) return TradeSideType::TradeIn;
	if (m_afterOpen <= hhmmss && hhmmss < m_afterClose) return TradeSideType::AfterTrade;

	return TradeSideType::NightStock;
}

