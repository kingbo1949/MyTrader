#pragma once

#include <base_trade.h>
#include <writer.h>
#include <reader.h>

#include <string>
class CJSon
{
public:
	CJSon();
	virtual ~CJSon();

protected:

	virtual	void					SaveJSonValue(const std::string& fileName, const Json::Value& jsonValue);

	virtual	void					LoadJSonValue(const std::string& fileName, Json::Value& jsonValue);

	Json::Value						MakeValue_TradeKey(const TradeKey& tradekey);
	TradeKey						Make_TradeKey(const Json::Value& tradekeyValue);


};

