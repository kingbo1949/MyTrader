#pragma once
#include "JSon_Deal.h"
class CJSon_Deal_Real : public CJSon_Deal
{
public:
	CJSon_Deal_Real() { ; };
	virtual ~CJSon_Deal_Real() { ; };

	virtual void			Save_Deals(const OneDealPtrs& deals) override final;

	virtual void			Load_Deals(OneDealPtrs& deals) override final;

protected:
	std::string				GetFileName();

	Json::Value				MakeValue_DealKey(const DealKey& dealkey);
	DealKey					Make_DealKey(const Json::Value& dealkeyValue);


	Json::Value				MakeValue_Deal(OneDealPtr& onedeal);
	OneDealPtr				Make_Deal(const Json::Value& onedealValue);

	Json::Value				MakeValue_Deals(const OneDealPtrs& deals);
	OneDealPtrs				Make_Deals(const Json::Value& dealsValue);


};

