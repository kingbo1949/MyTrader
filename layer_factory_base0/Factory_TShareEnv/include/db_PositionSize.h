#pragma once
#include <base_trade.h>
class Cdb_PositionSize
{
public:
	Cdb_PositionSize() { ; };
	virtual ~Cdb_PositionSize() { ; };

	
	virtual FileStatus			GetStatus() = 0;

	virtual void				SetStatus(FileStatus fileStatus) = 0;

	virtual void				NewDeal(const OnTradeMsg& tradeMsg) = 0;

	virtual void				AddOne(PositionSizePtr positionSize) = 0;

	virtual void				RemoveOne(const PositonSizeKey& key) = 0;

	virtual PositionSizePtr		GetOne(const PositonSizeKey& key) = 0;

	virtual PositionSizePtrs	GetAll() = 0;


};
typedef std::shared_ptr<Cdb_PositionSize> db_PositionSizePtr;

