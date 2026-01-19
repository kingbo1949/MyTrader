#pragma once

#include <base_struc.h>
#include <base_trade.h>

// jump之后行情指定时间内如何移动
struct MoveAfterJump
{
	TickStep		forwardHigh = 0;						// 正向移动价位 相当于High		必须正值
	TickStep		backwardLow = 0;						// 逆向移动价位 相当于Low		必须负值
	TickStep		endClose = 0;							// 结束时移动价位 相当于Close	可正可负
	int				vol = 0;								// 成交量
	int				jumpStep;								// 跃进的价位数
	TrendType		trendType = TrendType::ProTrend;		// 顺势还是逆势
	bool			isHoldon = false;						// 是否顶住状态

	double			ma5 = 0;								// 5根S15的均线
	double			diffMa5 = 0;							// 当前价与5根S15的均线的差值
	//double			mean_diff_highLowMa = 0;				// 最近20根K线高低点与均线差值的均值
	//double			stddev_diff_highLowMa = 0;				// 最近20根K线高低点与均线差值的标准差

};

// jump所在的tick索引与该jump后续move的映射
typedef std::map<VectorIndex, MoveAfterJump> TickJumpMap;

// jump点集合
typedef std::vector<VectorIndex> JumpPoints;


class CStatisTick
{
public:
	CStatisTick(const CodeStr& codeId, const IBTickPtrs& ticks, int backupStep);
	virtual ~CStatisTick() { ; };

	// 初步扫描获取jump点
	void						ScanForJumpPoints(JumpPoints& bidJumps, JumpPoints& askJumps);

	// 根据jump集合扫描并填充对应的map
	void						FillJumpMap(Second_T second, const JumpPoints& jumps, BidOrAsk bidOrAskJump);

	// 打印到文件
	void						PrintToFile(BidOrAsk bidOrAskJump);

protected:
	CodeStr						m_codeId;
	IBTickPtrs					m_ticks;
	int							m_backupStep;		// 需要冗余的价位
	TickJumpMap					m_bidJumps;
	TickJumpMap					m_askJumps;
	IbContractPtr				m_contract;


	void						FillOneJumpMap(Second_T second, VectorIndex jumpIndex, BidOrAsk bidOrAskJump);

	int							GetVol(VectorIndex jumpIndex);

	// 得到回抽有效的位置
	int							GetBeginIndexForBackupStep(VectorIndex jumpIndex, BidOrAsk bidOrAskJump, Tick_T endTickTime);

	// 得到Jump的跃迁价位数
	int							GetJumpStep(VectorIndex jumpIndex, BidOrAsk bidOrAskJump);

	// 得到Jump是否顺势
	TrendType					GetJumpTrendType(VectorIndex jumpIndex, BidOrAsk bidOrAskJump);

	// 
	bool						GetHoldon(VectorIndex jumpIndex, BidOrAsk bidOrAskJump);

	// 取指定bid ask的值
	int							GetBidOrAsk(IBTickPtr tick, BidOrAsk bidOrAsk);
};
typedef std::shared_ptr<CStatisTick> StatisTickPtr;

