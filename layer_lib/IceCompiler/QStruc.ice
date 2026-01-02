#ifndef QSTRUC_ICE
#define QSTRUC_ICE

module IBTrader
{
	// 交易所
	enum IExchangePl
	{
		SMART,						// 智能选择交易所
		SEHK,						// 香港证券交易所
		NYSE,						// 纽约证券交易所
		NASDAQ,						// 纳斯达克
		ARCA						// 群岛交易所
	};
	// 币种标识
	enum IMoney
	{
		RMB,						//人民币
		USD,						//美元
		HKD,						//港币
		GBP,						//英镑
		JPY,						//日元
		CAD,						//加拿大元
		AUD,						//澳大利亚元
		EUR,						//殴元
		KRW,						//韩元
	};

    sequence<double> IAvgAtrs;

	struct ITimePair
	{
		long		beginPos = 0;		// 请求开始时间(毫秒)
		long		endPos = 0;			// 请求结束时间(毫秒)
	};
	sequence<ITimePair> ITimePairs;
	
	struct IQuery
	{
		byte		byReqType = 0;			// 请求类型 
											// 0表示请求最近多少个单位的数据
											// 1表示请求所有的数据(下载)
											// 2表示请求某个时间以前(包括该时间)多少个单位的数据(dwSubscribeNum为0时表示该时间前所有的数据) 
											// 3表示请求某个时间以后多少个单位的数据(dwSubscribeNum为0时表示该时间后所有的数据)
											// 4表示请求一个时间段的数据
		long		dwSubscribeNum = 0;     // 订阅数据的数量(请求类型为0、2、3时使用) 
		long		tTime = 0;				// 时间(请求类型为2、3时使用)(毫秒) 数据区间为[... : tTime] 或者 [tTime : ...]
		ITimePair	timePair;				// 时间段(请求类型为4时使用) 数据区间为[beginPos : endPos)
	};

	struct  IContract						// 合约
	{
		string			codeId = "";						// 品种代码
		string			des = "";							// 商品描述

		IExchangePl		exchangePl = SMART;					// 交易所
		IExchangePl		primaryExchangePl = NASDAQ;			// 主交易所

		IMoney			currencyID= USD;					// 货币ID(交易结算货币)
		int				decDigits = 0;						// 小数点位数
		double			minMove = 0.0;						// 最小价格变动

	};
	sequence<IContract> IContracts;

	struct  IBidAsk
	{
		double	bid = 0;				// 委买价
		int		bidVol = 0;				// 委买量
		double	ask = 0;				// 委卖价
		int		askVol = 0;				// 委卖量
	};
	sequence<IBidAsk> IBidAsks;

	struct  ITick								
	{
		string		codeId = "";				// 代码信息
		long		time = 0;					// localtime时间(毫秒)
		IBidAsks	bidAsks;
		double		open = 0;					// 今日开盘价
		double		last = 0;					// 最新价	
		int			vol	= 0;					// 最新成交量
		int			totalVol = 0;				// 当日累计成交量
		double		turnOver = 0.0;				// 总成交金额(单位：元)
		long		timeStamp = 0;				// 时间戳 纳秒 
	};
	sequence<ITick> ITicks;


	// K线的时段类别
	enum ITimeType
			{
				S15				,							// 15秒
				M1				,							// 1分钟
				M5				,							// 5分钟
				M15				,							// 15分钟
				M30				,							// 30分钟
				H1				,							// 1小时
				D1											// 1天
			};

	struct IQKey    // K线
	{
		string			codeId = "";				// 代码信息
		ITimeType		timeType = D1;
	};
	
	struct IKLine
	{
		long			time;			// 毫秒
		double			close;
		double			open;
		double			high; 
		double			low;
		long 			vol;
	};
	sequence<IKLine> IKLines;

	struct IKLinePair
	{
		IKLine			first;
		IKLine			second;
	};
	sequence<IKLinePair> IKLinePairs;



	// 移动平均线
	struct IAvgValue
	{
		long			time;			// 毫秒
		double			v5;				// 5周期均线
		double			v20;			// 20周期均线
		double			v60;			// 60周期均线
		double			v200;			// 200周期均线
	};
	sequence<IAvgValue> IAvgValues;

	struct IMacdValue
	{
		long			time;			// 毫秒
		double			emaShort;
		double			emaLong;
		double			dif;
		double			dea ;
		double			macd ;
	};
	sequence<IMacdValue> IMacdValues;

	// 背离类型
	enum IDivType
	{
		NORMAL,		// 正常
		TOP,		// 顶背离
		BOTTOM,		// 底背离
		TOPSUB,		// 附顶背离
		BOTTOMSUB	// 附底背离
	};
	sequence<IDivType> IDivTypes;

	struct IDivTypeValue
	{
		long			time = 0;			// 毫秒
		IDivType		divType = NORMAL;
		bool			isUTurn = false;	// 是否拐点型
	}
	sequence<IDivTypeValue> IDivTypeValues;


	
	


};
#endif