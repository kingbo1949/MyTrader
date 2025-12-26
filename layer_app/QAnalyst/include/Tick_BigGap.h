#pragma once
#include <base_trade.h>

// 买4买5 卖4卖5之间有巨大的缝隙

struct Gap
{
	VectorIndex			index = 0;
	TickStep			step = 0;
};
class CTick_BigGap
{
public:
	CTick_BigGap(const IBTickPtrs& ticks, int bigGapStep);
	virtual ~CTick_BigGap() { ; };

	void						Scan();
	void						PrintToFile(BidOrAsk bidOrAsk);

protected:
	IBTickPtrs					m_ticks;
	int							m_bigGapStep;
	std::vector<Gap>			m_bidGaps;
	std::vector<Gap>			m_askGaps;


};
typedef std::shared_ptr<CTick_BigGap> Tick_BigGapPtr;
