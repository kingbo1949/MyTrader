#pragma once
#include "CompileDefine.h.h"
#include "base_struc.h"


// 实盘时，如果发送策略所生成的价格
enum class RealPriceType
{
	Original,			// 使用策略生成的原生价格发送实盘委托
	Wait,				// 使用0号位置，委托等待在盘口最前面
	Eat,				// 使用-1号位置，对价单吃发送委托
	MidPoint			// 使用盘口中间价
};


// 模拟撮合类型
enum class MeetDealType
{
	loose,		// 松散型撮合 买卖盘之间的委托可以成交 落在买1上的买单也可以成交
	rigorous	// 严谨型撮合 只有越过卖1的买单才可以成交
};


enum class FileStatus
{
	Saved,		// 已保存
	Updated		// 已更新
};

enum class FirstOrSecond
{
	First,
	Second
};
enum class BidOrAsk
{
	Bid,
	Ask
};

enum class ActiveOrInactive
{
	Active,
	Inactive

};
enum class WinOrLoss
{
	Win,
	Loss
};

enum class BuyOrSell
{
	Buy,
	Sell
};
enum class RoundAction
{
	Open_Round,		// 打开一个回合 开仓
	Cover_Round		// 关闭一个回合 平仓
};

enum class LongOrShort
{
	LongT,
	ShortT
};
enum class OrderStatus
{
	AllDealed,          // 全部成交
	AllCanceled,		// 已撤单
	SomeDealed			// 委托处于活跃状态，0成交或者部分成交(部成部撤属于AllCanceled)

};

enum class StHalfStatus
{
	AtFirst,
	DoCancelOrderHalf,
	DoSendOrderHalf
};

// 策略子单元
enum class StSubModule
{
	SubOpen,			// 开仓
	SubCover,			// 平仓
};


struct SubModuleParams
{
	CodeHashId						targetCodeId = 0;
	IbContractPtr					contract = nullptr;
	StrategyIdHashId				strategyIdHashId = 0;
	RoundAction						openOrCover = RoundAction::Open_Round;
	BuyOrSell						buyOrSell = BuyOrSell::Buy;
	StSubModule						subModule = StSubModule::SubOpen;
};

struct TradeKey
{
	TradeKey() { ; };
	TradeKey(StrategyIdHashId id, StSubModule module)
	{
		strategyIdHashId = id;
		stSubModule = module;
	}

	StrategyIdHashId	strategyIdHashId = 0;
	StSubModule			stSubModule = StSubModule::SubOpen;// 发单的子模块
	bool operator<(const TradeKey& right) const
	{
		if (strategyIdHashId != right.strategyIdHashId)
		{
			return strategyIdHashId < right.strategyIdHashId;
		}
		return stSubModule < right.stSubModule;
	}

};
typedef TradeKey OrderKey;
typedef TradeKey ActionKey;
typedef TradeKey DealKey;

// 刷新以供打印的部分
struct FreshPartInPricePair
{
	Tick_T			freshTime = 0;
	TrendType		trendType = TrendType::AntiTrend;
	double			param1 = 0;
	double			sideExtreamePrice = 0.0;		// 盘前极值
	Tick_T			sideExtreamePriceTime = 0;		// 盘前极值时间
	double			targetPrice = 0.0;				// 触发目标价格
};

struct PricePair
{
	Tick_T day = 0;					// 当日零点毫秒
	double openPrice = 0.0;			// 开仓价格
	double stopLossPrice = 0.0;		// 止损价格

	FreshPartInPricePair freshPart;
};
typedef std::shared_ptr<PricePair> PricePairPtr;





class COneOrder
{
public:
	OrderKey			orderKey;

	CodeHashId			codeHashId = 0;

	// 高频开仓委托单
	int					localOrderNo = 0;						// 本地委托号码

	int					tickIndex = 0;							// 导致交易信号产生的tick索引
	Tick_T				tickTime = 0;

	RoundAction			openOrCover = RoundAction::Open_Round;	// 打开关闭回合
	BuyOrSell			buyOrSell = BuyOrSell::Buy;				// 买卖
	OrderStatus			orderStatus = OrderStatus::SomeDealed;	// 状态

	double				orderQuantity = 0.0;					// 委托数量
	double				orderPriceReal = 0;						// 真实委托价格 发送到柜台的价格 为了快速成交或者贪图价位便宜，与策略价格不同
	double				orderPriceStrategy = 0;					// 策略委托价格 策略生成的价格
	double				dealedQuantity = 0.0;					// 已成交数量
	double				avgDealedPrice = 0.0;					// 已成交平均价格

};
typedef std::shared_ptr<COneOrder> OneOrderPtr;

struct Compare_OrderPtr
{
	bool operator ()(OneOrderPtr left, OneOrderPtr right) const
	{
		return left->localOrderNo < right->localOrderNo;
	}
};
typedef std::set<OneOrderPtr, Compare_OrderPtr> OneOrderPtrs;

typedef OneOrderPtr OneOrderOfSimulatorPtr;
typedef OneOrderPtrs OneOrderOfSimulatorPtrs;


typedef StrategyIdHashId PositonSizeKey;
class CPositionSize
{
public:
	PositonSizeKey							key;						// 持仓的key

	CodeHashId								codeHashId = 0;
	LongOrShort								longOrShort = LongOrShort::LongT;
	double									vol = 0.0;					// 手数
	double									avgPrice = 0.0;				// 持仓平均价格
	Tick_T									openTickTime = 0;			// 开仓毫秒时间，也是持仓建立时间 

};
typedef std::shared_ptr<CPositionSize> PositionSizePtr;
typedef std::vector<PositionSizePtr> PositionSizePtrs;





enum class ActionType
{
	CancelOrder,		// 撤单操作
	SendOrder			// 委托操作
};


class CAction
{
public:
	ActionKey				key;
	int						localOrderNo = 0;			// 对应委托的localOrderNo
	ActionType				actionType = ActionType::SendOrder;

	bool operator<(const CAction& right) const
	{
		return localOrderNo < right.localOrderNo;
	}
};
typedef std::shared_ptr<CAction> ActionPtr;

struct Compare_ActionPtr
{
	bool operator ()(ActionPtr left, ActionPtr right) const
	{
		return left->localOrderNo < right->localOrderNo;
	}
};
typedef std::set<ActionPtr, Compare_ActionPtr> ActionPtrs;




class COneDeal
{
public:
	DealKey				key;
	CodeHashId			codeHashId = 0;
	int					dealNo = 0;								// 成交编号
	int					localOrderNo = 0;						// 委托号码
	BuyOrSell			buyOrSell = BuyOrSell::Buy;				// 方向
	RoundAction			openOrCover = RoundAction::Open_Round;// 开平
	double				dealQuantity = 0.0;						// 成交数量
	double				dealPrice = 0;							// 成交价格
	int					dealIndex = 0;							// 成交tick索引
	Tick_T				dealTickTime = 0;
	double				fee = 0.0;								// 手续费
};
typedef std::shared_ptr<COneDeal> OneDealPtr;

struct Compare_DealPtr
{
	bool operator ()(OneDealPtr left, OneDealPtr right) const
	{
		return left->dealNo < right->dealNo;
	}
};
typedef std::set<OneDealPtr, Compare_DealPtr> OneDealPtrs;

// 委托回报
struct OnOrderMsg
{
	OrderStatus		orderStatus = OrderStatus::SomeDealed;	// 在柜台中 该委托状态
	double			todayDealedVol = 0.0;					// 在柜台中 该委托当日累计已成交量
	double			todayDealedAvgPrice = 0;				// 在柜台中 该委托累计已成交平均价格
	OneOrderPtr		pOriginalOrder = nullptr;				// 回报返回前在数据库中的状态
};
typedef std::vector<OnOrderMsg> OnOrderMsgs;

// 成交回报
struct OnTradeMsg
{
	int				dealNo = 0;					// 成交编号
	double			dealQuantity = 0.0;			// 成交手数
	double			dealPrice = 0;				// 成交价格
	OneOrderPtr		pOriginalOrder = nullptr;	// 回报返回前在数据库中的状态
};
typedef std::vector<OnTradeMsg> OnTradeMsgs;


///////////////////////////////////////////////////////////////////////////////////////////////////

class Strategykey
{
public:
	std::string			strategyName = "";
	CodeStr				targetCodeId = "";
	BuyOrSell			buyOrSell = BuyOrSell::Buy;
	int					id = 0;							// 一般是0，但是如果两个策略完全一样仅是参数不同，则需要用此参数标识

	bool operator<(const Strategykey& right) const
	{
		if (strategyName != right.strategyName)
		{
			return strategyName < right.strategyName;
		}
		if (targetCodeId != right.targetCodeId)
		{
			return targetCodeId < right.targetCodeId;
		}
		if (buyOrSell != right.buyOrSell)
		{
			return buyOrSell < right.buyOrSell;
		}
		return id < right.id;


	}
	StrategyId		ToStr();
	StrategyId		FreshToStr()
	{
		str = "";
		return ToStr();
	}

protected:
	std::string str;

};



// 策略参数
class CStrategyParam
{
public:
	Strategykey					key;
	bool						canOpen = true;						// 是否允许开仓
	bool						canCover = true;					// 是否允许平仓
	Time_Type					timetype = Time_Type::M1;
	bool						useInterval = true;					// true使用18:00:00-08:29:59区间高低价，否则使用08:30开盘价格
	bool						useTimeInterval = true;				// true要求盘中极值第一次必须出现在08:30-09:30之间，false则极值出现在任何地方都可以
	int							canChgBaseCount = 0;				// 盘中是否可以修改基准线的次数
	int							maxTradeCount = 0;					// 最大交易次数
	double						proTrendParam1 = 0.0;				// 顺势参数1
	double						antiTrendParam1 = 0.0;				// 逆势参数1
	double						param2 = 0.0;
	double						line1 = 0.0;						// 指定的线用于触发某个条件
	double						line2 = 0.0;						// 指定的线用于触发某个条件
	double						probeVol = 0;						// 探针手数
	double						maxVol = 0;							// 最大持仓手数

};
typedef std::shared_ptr<CStrategyParam> StrategyParamPtr;
typedef std::vector<StrategyParamPtr> StrategyParamPtrs;


enum class TradePointStatus
{
	OpenMorning,
	CloseMorning,

	OpenAfternoon,
	CloseAfternoon,

	OpenEvening,
	CloseEvening,

	BreakBegin,
	BreakEnd
};

// 时点描述
struct TradePointDec
{
	TradePointStatus		status = TradePointStatus::CloseAfternoon;	// 最近的时间点
	int						millisecond = 60000;						// 与最近时点的偏离毫秒
};


// 交易时点
class CTradePoint
{
public:
	ExchangePl								primaryExchangePl = ExchangePl::SMART;	// 交易所
	std::string								openMorning = "09:00:00";				// 早开盘时点
	std::string								closeMorning = "11:30:00";				// 早收盘点

	std::string								openAfternoon = "13:30:00";				// 下午开盘时点
	std::string								closeAfternoon = "15:00:00";			// 下午收盘时点

	std::string								openEvening = "21:00:00";				// 晚开盘时点
	std::string								closeEvening = "01:00:00";				// 晚收盘时点

	std::string								breakBegin = "10:15:00";				// 小节开始时点
	std::string								breakEnd = "10:30:00";					// 小节结束时点
};
typedef std::shared_ptr<CTradePoint> TradePointPtr;
typedef std::vector<TradePointPtr> TradePointPtrs;

// 测试委托

enum class PriceStatus
{
	MakeItDeal,		// 使得委托成交的价格（对价或者市价）
	MakeItPending	// 使得委托悬挂不成交的价格（离盘口比较远）
};
enum class ManualStatus
{
	Init,
	BuyToOpen,		// 买开
	SellToCover,	// 卖平
	SellToOpen,		// 卖开
	BuyToCover,		// 买平
	CancelManul		// 撤单
};

