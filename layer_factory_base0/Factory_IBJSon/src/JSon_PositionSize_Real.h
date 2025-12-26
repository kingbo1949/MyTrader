#pragma once
#include "JSon_PositionSize.h"
class CJSon_PositionSize_Real : public CJSon_PositionSize
{
public:
	CJSon_PositionSize_Real() { ; };
	virtual ~CJSon_PositionSize_Real() { ; };

	virtual void			Save_PositionSizes(const PositionSizePtrs& positionsizes) override final;

	virtual void			Load_PositionSizes(PositionSizePtrs& positionsizes) override final;

protected:
	std::string				GetFileName();

	Json::Value				MakeValue_PositionSize(PositionSizePtr positionsize);
	PositionSizePtr			Make_PositionSize(const Json::Value& positionsizeValue);

	Json::Value				MakeValue_PositionSizes(const PositionSizePtrs& positonsizes);
	PositionSizePtrs		Make_PositionSizes(const Json::Value& positonsizesValue);




};

