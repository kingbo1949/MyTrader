#pragma once
#include <RocksTable.h>
#include <QStruc.h>
#include "../RocksTypes.h"
#include <map>
#include <memory>
using namespace IBTrader;
using namespace Ice;


class CDbTable_KLine
{
public:
	/// @param env     RocksDB 环境引用
	/// @param prefix  列族名前缀（如 "KLINE"），实际列族名为 "KLINE_M5" 等
	CDbTable_KLine(CRocksEnv& env, const std::string& prefix);
	~CDbTable_KLine() = default;

	void		AddOne(const std::string& codeId, ITimeType timeType, const IKLine& value);
	void		AddSome(const std::string& codeId, ITimeType timeType, const IKLines& values);
	bool		GetOne(const std::string& codeId, ITimeType timeType, Long timePos, IKLine& value);

	void		RemoveOne(const std::string& codeId, ITimeType timeType, Long timePos);
	void		RemoveKey(const std::string& codeId, ITimeType timeType);
	void		RemoveByRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime);
	void		RemoveAll();

	void		GetKLines(const std::string& codeId, ITimeType timeType, const IQuery& query, IKLines& values);

private:
	CRocksEnv& m_env;
	std::string m_prefix;
	std::map<ITimeType, std::unique_ptr<CRocksTable<IKLine>>> m_tables;

	/// 获取指定 timeType 的 CRocksTable，不存在则创建
	CRocksTable<IKLine>& GetTable(ITimeType timeType);

	void		GetRange(const std::string& codeId, ITimeType timeType, Long beginTime, Long endTime, IKLines& values);
	void		GetForWard(const std::string& codeId, ITimeType timeType, Long beginTime, Long count, IKLines& values);
	void		GetBackWard(const std::string& codeId, ITimeType timeType, Long endTime, Long count, IKLines& values);
};
