#pragma once
#include "JSon.h"
#include <base_trade.h>
class CJSon_PositionSize : public CJSon
{
public:
	CJSon_PositionSize() { ; };
	virtual ~CJSon_PositionSize() { ; };

	virtual void			Save_PositionSizes(const PositionSizePtrs& positionsizes) = 0;

	virtual void			Load_PositionSizes(PositionSizePtrs& positionsizes) = 0;

};
typedef std::shared_ptr<CJSon_PositionSize> JSon_PositionSizePtr;



