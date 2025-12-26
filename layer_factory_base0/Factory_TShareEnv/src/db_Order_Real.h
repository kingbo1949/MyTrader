#pragma once
#include "db_Order.h"
class Cdb_Order_Real : public Cdb_Order
{

public:
	Cdb_Order_Real(ActiveOrInactive activeOrInactive);
	virtual ~Cdb_Order_Real() { ; };

	virtual FileStatus		GetStatus() override final;

	virtual void			SetStatus(FileStatus fileStatus)  override final;


	virtual void			AddOne(OneOrderPtr porder) override final;

	virtual OneOrderPtr		RemoveOne(const OrderKey& key, int localOrderNo) override final;

	virtual OneOrderPtr		GetOne(const OrderKey& key, int localOrderNo)  override final;

	// 耗时操作，仅在委托和成交回报中调用
	virtual OneOrderPtr		GetOne(int localOrderNo) override final;

	virtual OneOrderPtrs	GetAll(const OrderKey& key)  override final;

protected:
	std::mutex							m_mutex;
	std::map<OrderKey, OneOrderPtrs>	m_orders;
	ActiveOrInactive					m_activeOrInactive;
	FileStatus							m_status;

};

