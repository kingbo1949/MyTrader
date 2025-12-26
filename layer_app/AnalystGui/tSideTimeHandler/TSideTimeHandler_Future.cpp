#include "TSideTimeHandler_Future.h"
#include <Factory_IBGlobalShare.h>
#include "../src/Factory_AnalystGui.h"
// 期指的盘后时间：16：00 - 17：00 夜盘上：18：00 - 04：00 夜盘下：04：00 - 09：30
CTSideTimeHandler_Future::CTSideTimeHandler_Future()
{
	m_open = CHighFrequencyGlobalFunc::MakeMilliSecondPart("09:30:00", 0);
	m_close = CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0);

	m_afterOpen = CHighFrequencyGlobalFunc::MakeMilliSecondPart("16:00:00", 0);
	m_afterClose = CHighFrequencyGlobalFunc::MakeMilliSecondPart("17:00:00", 0);

	m_night1Open = CHighFrequencyGlobalFunc::MakeMilliSecondPart("18:00:00", 0);
	m_night1Close = CHighFrequencyGlobalFunc::MakeMilliSecondPart("23:59:59", 999);

	m_night2Open = CHighFrequencyGlobalFunc::MakeMilliSecondPart("00:00:00", 0);
	m_night2Close = CHighFrequencyGlobalFunc::MakeMilliSecondPart("04:00:00", 0);

	m_night3Open = CHighFrequencyGlobalFunc::MakeMilliSecondPart("04:00:00", 0);
	m_night3Close = CHighFrequencyGlobalFunc::MakeMilliSecondPart("09:30:00", 0);
}

QColor CTSideTimeHandler_Future::GetTSideColor(TradeSideType type)
{
	if (type == TradeSideType::AfterTrade) return MakeAndGet_ColorManager()->GetColor().tradeSide1;
	if (type == TradeSideType::NightFuture1) return MakeAndGet_ColorManager()->GetColor().tradeSide2;
	if (type == TradeSideType::NightFuture2) return MakeAndGet_ColorManager()->GetColor().tradeSide1;

	return MakeAndGet_ColorManager()->GetColor().backGround;
}

TradeSideType CTSideTimeHandler_Future::GetTSideType(Tick_T time)
{
	// 把time转换为当日的时分秒
	Tick_T hhmmss = time - CHighFrequencyGlobalFunc::GetDayMillisec(time);

	if (m_night2Open <= hhmmss && hhmmss < m_night2Close) return TradeSideType::NightFuture1;
	if (m_night3Open <= hhmmss && hhmmss < m_night3Close) return TradeSideType::NightFuture2;

	if (m_open <= hhmmss && hhmmss < m_close) return TradeSideType::TradeIn;
	if (m_afterOpen <= hhmmss && hhmmss < m_afterClose) return TradeSideType::AfterTrade;

	return TradeSideType::NightFuture1;
}
