#pragma once

#include <base_trade.h>
class Cdb_Order
{
public:
	Cdb_Order() { ; };
	virtual ~Cdb_Order() { ; };

	virtual FileStatus		GetStatus() = 0;

	virtual void			SetStatus(FileStatus fileStatus) = 0;

	virtual void			AddOne(OneOrderPtr porder) = 0;

	virtual OneOrderPtr		RemoveOne(const OrderKey& key, int localOrderNo) = 0;

	virtual OneOrderPtr		GetOne(const OrderKey& key, int localOrderNo) = 0;

	virtual OneOrderPtr		GetOne(int localOrderNo) = 0;
	
	virtual OneOrderPtrs	GetAll(const OrderKey& key) = 0;

	

};
typedef std::shared_ptr<Cdb_Order> db_OrderPtr;

