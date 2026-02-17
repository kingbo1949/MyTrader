#pragma once
#include <base_struc.h>
class CUpdateKlineFromFile
{
public:
	CUpdateKlineFromFile() { ; };
	virtual ~CUpdateKlineFromFile() { ; };

	void	Go();

protected:
	std::string GetFileName(const CodeStr& codeId, Time_Type timeType);

	void UpdateOneCode(const CodeStr& codeId);

	// 从文件中读取kline并更新到数据库， filename是带路径的文件名
	void UpdateFile(const CodeStr& codeId, Time_Type timeType);

	// 检查更新后的服务器数据
	void CheckServerData(const CodeStr& codeId, Time_Type timeType);

};

