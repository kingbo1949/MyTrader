

#pragma once


#include "CompileDefine.h.h"


typedef time_t Second_T;		// 秒时间类型
typedef time_t Tick_T;			// 毫秒时间类型
typedef time_t MilliSecond_T;	// 毫秒时间类型
typedef time_t MicroSecond_T;	// 微秒时间类型

typedef std::vector<std::string> Strings;
typedef int VectorIndex;
typedef int TickStep;			// 价位

enum class MarketType
{
	PreMarket,		// 盘前交易
	RegularMarket,	// 盘中交易
	AfterHour		// 盘后交易
};


enum class BoolType
{
	True,
	False,
	Unknown
};
enum class TrendType
{
	ProTrend,		// 顺势
	AntiTrend		// 逆势
};


enum class SetupType
{
	QGenerator,
	RealTrader,
	Simulator
};
typedef SetupType UseType;

enum class LineType
{
	UseTick,
	UseMa,
	UseKLine
};
enum class SelectType
{
	True,
	False,
	All
};

enum class SecurityType
{
	STK,				// 股票
	FUT					// 期货
};


enum class Thread_Status
{
	Running,			// 运行态
	QuickCover,			// 快速平仓态
	Stop				// 停止态
};


enum class CurrentTimeType
{
	For_Real = 0,					// 实时环境
	For_Simulator = 1,				// 模拟环境
};
typedef CurrentTimeType RealOrSimulator;


// 交易所
enum class ExchangePl
{
	SMART = 0,						// 智能选择交易所
	SEHK = 1,						// 香港证券交易所
	NYSE = 2,						// 纽约证券交易所
	NASDAQ = 3,						// 纳斯达克
	ARCA = 4,						// 群岛交易所 
	CME = 5,						// CME
	COMEX = 6						// COMEX

};
// 币种标识
enum class Money
{
	RMB = 0,						//人民币
	USD = 1,						//美元
	HKD = 2,						//港币
	GBP = 3,						//英镑
	JPY = 4,						//日元
	CAD = 5,						//加拿大元
	AUD = 6,						//澳大利亚元
	EUR = 7,						//殴元
	KRW = 8,						//韩元
};

// 混合品种
struct MixCode
{
	CodeHashId	myHashId = 0;
	CodeStr		firstCode = "";
	CodeHashId	firstHash = 0;
	CodeStr		secondCode = "";
	CodeHashId	secondHash = 0;
};
struct Compare_MixCode
{
	bool operator ()(const MixCode& left, const MixCode& right) const
	{
		if (left.firstHash != right.firstHash)
		{
			return left.firstHash < right.firstHash;
		}
		return left.secondHash < right.secondHash;

	}
};
typedef std::set<MixCode, Compare_MixCode> MixCodes;

class CIbContract;
typedef std::shared_ptr<CIbContract> IbContractPtr;
class CIbContract
{
public:
	std::string		codeId = "";							// 品种代码
	std::string		localSymbol = "";						// 品种别名
	SecurityType	securityType = SecurityType::STK;

	ExchangePl		exchangePl = ExchangePl::SMART;			// 交易所
	ExchangePl		primaryExchangePl = ExchangePl::NASDAQ;	// 主交易所

	Money			currencyID = Money::USD;				// 货币ID(交易结算货币)
	int				decDigits = 0;							// 小数点位数
	double			minMove = 0.0;							// 最小价格变动

	bool			enable = true;

	IbContractPtr	Clone();
};

typedef std::vector<IbContractPtr> IbContractPtrs;

struct  IBBidAsk
{
	int		bid = 0;				// 委买价位
	int		bidVol = 0;				// 委买量
	int		ask = 0;				// 委卖价位
	int		askVol = 0;				// 委卖量
};
typedef std::vector<IBBidAsk> IBBidAsks;


class  CIBTick;
typedef std::shared_ptr<CIBTick> IBTickPtr;
class  CIBTick
{
public:
	CodeHashId	codeHash = 0;				// codeId的哈希值
	Tick_T		time = 0;					// localtime时间(毫秒)
	IBBidAsks	bidAsks;
	int			open = 0;					// 今日开盘价位
	int			last = 0;					// 最新价位	
	int			vol = 0;					// 最新成交量
	int			totalVol = 0;				// 当日累计成交量
	double		turnOver = 0.0;				// 总成交金额(单位：元)
	time_t		timeStamp = 0;				// 时间戳 纳秒 

	IBTickPtr	Clone();
};

typedef std::vector<IBTickPtr> IBTickPtrs;
typedef std::vector<CIBTick> IBTicks;

enum class Time_Type
{
	S15,						// 15秒
	M1,							// 1分钟
	M5,							// 5分钟
	M15,						// 15分钟
	M30,						// 30分钟
	H1,							// 1小时
	D1							// 1天

};
enum class HighOrLow
{
	High = 0,
	Low = 1,
};

struct HighAndLow
{
	time_t			highPos = 0;
	std::string		highPosStr = "";
	double			high = -9999999.0;

	time_t			lowPos = 0;
	std::string		lowPosStr = "";
	double			low = 9999999.0;
};

struct HighLowPair
{
	double			high = std::numeric_limits<double>::quiet_NaN();
	double			low = std::numeric_limits<double>::quiet_NaN();
};


class  CIBKLine;
typedef std::shared_ptr<CIBKLine> IBKLinePtr;
class  CIBKLine
{
public:
	time_t		time = 0;					// localtime时间(毫秒)
	double		open = 0.0;					// 今日开盘价位
	double		close = 0.0;				// 最新价位	
	double		high = 0.0;					// 最高价位	
	double		low = 0.0;					// 最低价位	
	int			vol = 0;					// 最新成交量

	IBKLinePtr	Clone();
};

typedef std::vector<IBKLinePtr> IBKLinePtrs;
typedef std::vector<CIBKLine> IBKLines;

struct IBKLinePair
{
	IBKLinePtr	first = nullptr;
	IBKLinePtr	second = nullptr;
};
typedef std::vector<IBKLinePair> IBKLinePairs;


class CIBAvgValue;
typedef std::shared_ptr<CIBAvgValue> IBAvgValuePtr;
class  CIBAvgValue
{
public:
	time_t		time = 0;					// localtime时间(毫秒)
	double		v5 = 0;						// 5周期均线
	double		v20 = 0;					// 20周期均线
	double		v60 = 0;					// 60周期均线
	double		v200 = 0;					// 200周期均线
};
typedef std::vector<IBAvgValuePtr> IBAvgValuePtrs;

typedef IBAvgValuePtr IBMaPtr;
typedef std::vector<IBMaPtr> IBMaPtrs;

typedef IBAvgValuePtr IBVwMaPtr;
typedef std::vector<IBVwMaPtr> IBVwMaPtrs;

typedef IBAvgValuePtr IBEmaPtr;
typedef std::vector<IBEmaPtr> IBEmaPtrs;


class  CIBMacd;
typedef std::shared_ptr<CIBMacd> IBMacdPtr;
class  CIBMacd
{
public:
	time_t		time = 0;					// localtime时间(毫秒)
	double		emaShort = std::numeric_limits<double>::quiet_NaN();
	double		emaLong = std::numeric_limits<double>::quiet_NaN();
	double		dif = std::numeric_limits<double>::quiet_NaN();
	double		dea = std::numeric_limits<double>::quiet_NaN();		// signal
	double		macd = std::numeric_limits<double>::quiet_NaN();	// MACD 柱状图

};
typedef std::vector<IBMacdPtr> IBMacdPtrs;

enum class DivergenceType
{
	Normal,		// 正常
	Top,		// 顶背离
	Bottom,		// 底背离
	TopSub,		// 附顶背离
	BottomSub	// 附底背离
};
typedef std::vector<DivergenceType> DivergenceTypes;

class  CIBDivType;
typedef std::shared_ptr<CIBDivType> IBDivTypePtr;
class  CIBDivType
{
public:
	time_t			time = 0;					// localtime时间(毫秒)
	DivergenceType	divType = DivergenceType::Normal;
	bool			isUTurn = false;

};
typedef std::vector<IBDivTypePtr> IBDivTypePtrs;

class  CIBAtr;
typedef std::shared_ptr<CIBAtr> IBAtrPtr;
class  CIBAtr
{
public:
	time_t			time = 0;												// localtime时间(毫秒)
	double			thisAtr = std::numeric_limits<double>::quiet_NaN();
	double			avgAtr = std::numeric_limits<double>::quiet_NaN();

};
typedef std::vector<IBAtrPtr> IBAtrPtrs;


enum class TimeZone_Type
{
	For_QAnalyst = 0,			// 打印行情数据
	For_Simulator = 1,			// 模拟交易
};

struct TimeZoneOfCodeId
{
	CodeStr		codeId = "";
	LineType	lineType = LineType::UseTick;
	Time_Type	timeType = Time_Type::D1;
	Tick_T		beginPos = 0;		// 请求开始时间(毫秒)
	Tick_T		endPos = 0;			// 请求结束时间(毫秒)
};
typedef std::shared_ptr<TimeZoneOfCodeId> TimeZoneOfCodeIdPtr;
typedef std::vector<TimeZoneOfCodeId> TimeZoneOfCodeIds;

struct TimePair
{
	time_t		beginPos = 0;		// 请求开始时间(毫秒)
	time_t		endPos = 0;			// 请求结束时间(毫秒)
};
typedef std::vector<TimePair> TimePairs;

// 定义一个有效区间[beginPos, endPos]
struct RangePair
{
	size_t		beginPos = 0;		// 开始位置
	size_t		endPos = 0;			// 结束位置
};

class COneBroker
{
public:
	std::string			gatewayIp = "";										// IB网关地址
	int					gatewayPort = 0;									// IB网关port
	int					connId = 0;											// IB连接id

	SetupType			setupType = SetupType::QGenerator;					// 当前时间如何设置，这也是行情的产生方式


};
typedef std::shared_ptr<COneBroker> OneBrokerPtr;
typedef std::vector<OneBrokerPtr> Brokers;


struct MacdValue
{
	double emaShort = std::numeric_limits<double>::quiet_NaN();
	double emaLong = std::numeric_limits<double>::quiet_NaN();
	double dif = std::numeric_limits<double>::quiet_NaN();
	double dea = std::numeric_limits<double>::quiet_NaN();		// signal
	double macd = std::numeric_limits<double>::quiet_NaN();	// MACD 柱状图

};
typedef std::vector<MacdValue> MacdValues;

struct AverageValue
{
	double v5 = std::numeric_limits<double>::quiet_NaN();
	double v20 = std::numeric_limits<double>::quiet_NaN();
	double v60 = std::numeric_limits<double>::quiet_NaN();
	double v200 = std::numeric_limits<double>::quiet_NaN();
};
typedef std::vector<AverageValue> AverageValues;

double& g_GetCircleValue(AverageValue& value, int circle);

double g_SumClose(double acc, const IBKLinePtr kline);




