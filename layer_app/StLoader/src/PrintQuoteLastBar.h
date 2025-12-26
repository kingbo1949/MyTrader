#pragma once
#include <base_struc.h>
class CPrintQuoteLastBar
{
public:
	CPrintQuoteLastBar() { ; };
	virtual ~CPrintQuoteLastBar() { ; };

	void		GoPrint() ;

protected:
	// 查询并打印数据库最后更新的tick
	void		PrintLastTickToDb();

	// 查询并打印指定合约的最后一个tick
	void		PrintLastTick(IbContractPtr contract);

	// 查询并打印指定合约的最后一个BAR
	void		PrintLastBar(IbContractPtr contract, Time_Type timetype);

	// 打印实时接收的tick
	void		PrintRealTick(IbContractPtr contract);

};
typedef std::shared_ptr<CPrintQuoteLastBar> PrintQuoteLastBarPtr;


