#include "stdafx.h"
#include "QtGlobal.h"
#include <Factory_IBGlobalShare.h>
#include <Global.h>

CQtGlobal::CQtGlobal()
{
}

std::string CQtGlobal::QtString_StdString(const QString& qtString)
{
	return qtString.toLocal8Bit().constData();
}

QString CQtGlobal::StdString_QString(const std::string& string)
{
	return QString::fromLocal8Bit(QByteArray::fromRawData(string.c_str(), string.size()));
}

QStringList CQtGlobal::StdStrings_QStringList(const std::vector<std::string> strings)
{
	QStringList ret;
	for (const auto& str : strings)
	{
		ret.append(StdString_QString(str));
	}
	return ret;

}

void CQtGlobal::SetTableWidgetHead_KLine(QTableWidget* pTable)
{
	std::string titel = CTransDataToStr::GetKlineTitle_Csv();
	std::vector<std::string> strings = CGlobal::SplitStr(titel, ",");
	QStringList headList = StdStrings_QStringList(strings);
	pTable->setColumnCount(headList.count());
	pTable->setHorizontalHeaderLabels(headList);


}

void CQtGlobal::SetTableWidgetHead_Tick(QTableWidget* pTable, SecurityType securityType)
{
	std::string titel = "";
	if (securityType == SecurityType::FUT)
	{
		titel = CTransDataToStr::GetDeepTickTitle_Csv();
	}
	else
	{
		titel = CTransDataToStr::GetTickTitle_Csv();
	}

	std::vector<std::string> strings = CGlobal::SplitStr(titel, ",");
	QStringList headList = StdStrings_QStringList(strings);
	pTable->setColumnCount(headList.count());
	pTable->setHorizontalHeaderLabels(headList);
}

void CQtGlobal::FillTableWidget_KLine(QTableWidget* pTable, const IBKLinePtrs& klines, bool needChgHead, bool onlyAppendItem)
{
	if (needChgHead)
	{
		pTable->clear();
		SetTableWidgetHead_KLine(pTable);
	}
	if (!onlyAppendItem)
	{
		// 清空行， 不含表头
		pTable->clearContents();

		// 删除所有行
		pTable->setRowCount(0);
	}
	for (auto kline : klines)
	{
		int index = pTable->rowCount();//行数
		pTable->insertRow(index);//在最后插入一行

		std::string klingstr = CTransDataToStr::GetKlineStr_Csv(kline);
		Strings klinestrs = CGlobal::SplitStr(klingstr, ",");
		if (klinestrs.size() == 0) continue;

		int colCount = pTable->columnCount();
		for (int col = 0; col < colCount; col++)
		{
			QTableWidgetItem* item = new QTableWidgetItem(klinestrs[col].c_str());
			pTable->setItem(index, col, item);
		}

	}
}

void CQtGlobal::FillTableWidget_Tick(QTableWidget* pTable, const IBTickPtrs& ticks, IbContractPtr contract)
{
	IBTickPtr pLast = nullptr;

	int buyJump = 0;
	int sellJump = 0;
	for (auto tick : ticks)
	{
		int index = pTable->rowCount();//行数
		pTable->insertRow(index);//在最后插入一行

		MicroSecond_T a = Get_CurrentTime()->GetCurrentTime_microsecond();
		std::string tickstr = "";
		if (contract->securityType == SecurityType::FUT)
		{
			tickstr = CTransDataToStr::GetTickDepthStr_Csv(tick, contract->minMove);
		}
		else
		{
			tickstr = CTransDataToStr::GetStr_Csv(tick, contract->minMove);
		}

		Strings tickstrs = CGlobal::SplitStr(tickstr, ",");
		MicroSecond_T b = Get_CurrentTime()->GetCurrentTime_microsecond();
		qDebug() << "handel string used:" << b - a;
		if (tickstrs.size() == 0) continue;

		bool bidJump = false;
		bool askJump = false;
		if (pLast)
		{
			if (tick->bidAsks[0].bid - pLast->bidAsks[0].bid >= 8)
			{
				bidJump = true;
				buyJump++;
			}
			if (pLast->bidAsks[0].ask - tick->bidAsks[0].ask >= 8)
			{
				askJump = true;
				sellJump++;
			}
		}



		int colCount = pTable->columnCount();
		for (int col = 0; col < colCount; col++)
		{
			QTableWidgetItem* item = new QTableWidgetItem(tickstrs[col].c_str());
			pTable->setItem(index, col, item);

			// 设置背景色
			if (tickstrs[col].substr(0, 2) == "->")
			{
				QBrush backgroundBrush(QColor(255, 87, 34)); // 橙色背景
				item->setBackground(backgroundBrush);
			}
			if (tickstrs[col].substr(0, 4) == "0.00")
			{
				QBrush backgroundBrush(QColor(200, 200, 200)); // 浅色背景
				item->setBackground(backgroundBrush);
			}
			// 13 16
			// std::string ticktitle = "time, open, close, totalvol, turnover, bid5, bidvol5, bid4, bidvol4, bid3, bidvol3, bid2, bidvol2, bid1, bidvol1, ->, ask1, askvol1, ask2, askvol2, ask3, askvol3, ask4, askvol4, ask5, askvol5,";

			if (col == 13 && bidJump)
			{
				QBrush backgroundBrush(QColor(0, 122, 204)); // 蓝色背景
				item->setBackground(backgroundBrush);
			}
			if (col == 16 && askJump)
			{
				QBrush backgroundBrush(QColor(0, 122, 204)); // 蓝色背景
				item->setBackground(backgroundBrush);
			}
		}
		MicroSecond_T c = Get_CurrentTime()->GetCurrentTime_microsecond();
		qDebug() << "fill table used:" << c - b;


		pLast = tick;

	}
	qDebug() << "buyJump:" << buyJump << "sellJump:" << sellJump;
}
