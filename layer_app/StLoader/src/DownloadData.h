#pragma once

#include <memory>
#include <string>
#include <base_struc.h>
class CDownloadData
{
public:
	CDownloadData() { ; };
	virtual ~CDownloadData() { ; };

	void				Go();

private:
	// 从IB接口取K线，不包括当前K线
	void				UpdateDbKLineFromIB(const std::string& codeId, Time_Type  timeType);


};
typedef std::shared_ptr<CDownloadData> DownloadDataPtr;

