#pragma once

// 活跃状态的action即是处于half状态的action, 当委托和撤单从柜台收到回报或者错误代码，即从本表中删除
#include <base_trade.h>
class Cdb_ActiveAction
{
public:
	Cdb_ActiveAction() { ; };
	virtual ~Cdb_ActiveAction() { ; };

	virtual FileStatus	GetStatus() = 0;

	virtual void		SetStatus(FileStatus fileStatus) = 0;

	virtual void		AddOne(ActionPtr pAction) = 0;

	virtual ActionPtr	GetOne(const ActionKey& key, int localOrderNo) = 0;

	virtual void		RemoveOne(const ActionKey& key, int localOrderNo ) = 0; 

	virtual ActionPtrs	GetAll(const ActionKey& key) = 0;

};

typedef std::shared_ptr<Cdb_ActiveAction> db_ActiveActionPtr;