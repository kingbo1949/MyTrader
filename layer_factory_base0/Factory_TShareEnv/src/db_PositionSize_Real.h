#pragma once
#include "db_PositionSize.h"
class Cdb_PositionSize_Real : public Cdb_PositionSize
{
public:
	Cdb_PositionSize_Real():m_status(FileStatus::Saved){ ; };
	virtual ~Cdb_PositionSize_Real() { ; };

	virtual FileStatus			GetStatus() override final;

	virtual void				SetStatus(FileStatus fileStatus) override final;

	// 发生了新的成交，修改持仓
	virtual void				NewDeal(const OnTradeMsg& tradeMsg) override final;

	virtual void				AddOne(PositionSizePtr positionSize) override final;

	virtual void				RemoveOne(const PositonSizeKey& key) override final;

	virtual PositionSizePtr		GetOne(const PositonSizeKey& key) override final;

	virtual PositionSizePtrs	GetAll() override final;

protected:
	std::mutex									m_mutex;
	std::map<PositonSizeKey, PositionSizePtr>	m_positionSizes;
	FileStatus									m_status;

};

