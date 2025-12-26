#pragma once

#include <string>
#include <QString>
#include <QtWidgets/QTableWidget>
#include <base_struc.h>

class CQtGlobal
{
public:
	CQtGlobal();

	static std::string      QtString_StdString(const QString& qtString);
	static QString          StdString_QString(const std::string& string);

	static QStringList      StdStrings_QStringList(const std::vector<std::string> strings);

	// 设置K线表头
	static void             SetTableWidgetHead_KLine(QTableWidget* pTable);

	// 设置1档TICK线表头
	static void             SetTableWidgetHead_Tick(QTableWidget* pTable, SecurityType securityType);

	// 填充K线 needChgHead为false不更新表头 onlyAppendItem为true不删除原有内容进添加
	static void             FillTableWidget_KLine(QTableWidget* pTable, const IBKLinePtrs& klines, bool needChgHead, bool onlyAppendItem);

	// 填充一档TICK线 needChgHead为false不更新表头 onlyAppendItem为true不删除原有内容进添加
	static void             FillTableWidget_Tick(QTableWidget* pTable, const IBTickPtrs& ticks, IbContractPtr contract);
};
