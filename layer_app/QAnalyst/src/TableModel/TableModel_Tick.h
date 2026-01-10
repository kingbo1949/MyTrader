#pragma once
#include "TableModel.h"
class CTableModel_Tick : public CTableModel
{
	Q_OBJECT

public:
	CTableModel_Tick(IbContractPtr pContract);
	virtual ~CTableModel_Tick() { ; }

	virtual void					QueryData(const TimeZoneOfCodeId& timeZone) override final;

	virtual void					SaveData() override final;

	virtual int						rowCount(const QModelIndex& parent = QModelIndex()) const override final;

	virtual QModelIndex				nextBidJump(int beginRow) override final;

	virtual QModelIndex				nextAskJump(int beginRow) override final;


protected:
	IBTickPtrs						m_ticks;
	virtual std::string				GetFileName(const CodeStr& codeId) override final;
	virtual std::string				GetTickStr(IBTickPtr thisTick, IbContractPtr contract) = 0;

	virtual bool					IsJump_Bid(IBTickPtr lastTick, IBTickPtr thisTick) const { return false; };
	virtual bool					IsJump_Ask(IBTickPtr lastTick, IBTickPtr thisTick) const { return false; };;



};

