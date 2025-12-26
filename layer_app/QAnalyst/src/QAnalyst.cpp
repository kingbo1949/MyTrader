#include "stdafx.h"
#include "QAnalyst.h"
#include "ui_QAnalyst.h"
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_QDatabase.h>
#include <QtGlobal.h>
#include <Global.h>


#include <base_struc.h>
#include <base_trade.h>
#include <Factory_Log.h>
#include "StatisTick.h"
#include "Tick_BigGap.h"
#include "ModifyKline.h"




QAnalyst::QAnalyst(QWidget* parent)
	: QWidget(parent), ui(new Ui::QAnalystClass), m_working(false)
{
	ui->setupUi(this);

	Init();
	m_working = true;
}

QAnalyst::~QAnalyst()
{
	ReleaseQDatabase();

	delete ui;
	ui = nullptr;

}


void QAnalyst::on_goBtn_clicked()
{
	m_model->QueryData(GetTimeZone());
	ui->tableViewCtrl->resizeColumnsToContents();

	int rowCount = ui->tableViewCtrl->model()->rowCount();//行数
	std::string str = fmt::format("{} items", rowCount);
	ui->resultlabel->setText(str.c_str());


	return;


}
void QAnalyst::on_saveBtn_clicked()
{
	m_model->SaveData();

	int rowCount = ui->tableViewCtrl->model()->rowCount();//行数
	std::string str = fmt::format("{} items saved", rowCount);
	ui->resultlabel->setText(str.c_str());

	return;

}

void QAnalyst::on_tickStatisButton_clicked()
{
	TimeZoneOfCodeId timeZone = GetTimeZone();
	if (timeZone.lineType != LineType::UseTick) return;

	TimePair timePair;
	timePair.beginPos = timeZone.beginPos;
	timePair.endPos = timeZone.endPos;
	IBTickPtrs ticks = MakeAndGet_QDatabase()->GetTickHisByLoop(timeZone.codeId, timePair);

	StatisTickPtr pStatisTick = std::make_shared<CStatisTick>(timeZone.codeId, ticks, 0);

	JumpPoints bidJumps, askJumps;
	pStatisTick->ScanForJumpPoints(bidJumps, askJumps);
	pStatisTick->FillJumpMap(3, bidJumps, BidOrAsk::Bid);
	pStatisTick->FillJumpMap(3, askJumps, BidOrAsk::Ask);

	pStatisTick->PrintToFile(BidOrAsk::Bid);
	pStatisTick->PrintToFile(BidOrAsk::Ask);

	//Tick_BigGapPtr pTickGap = std::make_shared<CTick_BigGap>(ticks, 4);
	//pTickGap->Scan();
	//pTickGap->PrintToFile(BidOrAsk::Bid);
	//pTickGap->PrintToFile(BidOrAsk::Ask);






}

void QAnalyst::on_timeGotoButton_clicked()
{
	QModelIndex index = m_model->findTime(ui->timeGotoCtrl->text());
	if (!index.isValid())
	{
		ui->resultlabel->setText("can not find time");
		return;
	}
	ui->tableViewCtrl->setCurrentIndex(index);
	ui->tableViewCtrl->scrollTo(index);
	ui->resultlabel->setText("-----------");
	return;

}

void QAnalyst::on_delBtn_clicked()
{
	TimeZoneOfCodeId timeZone = GetTimeZone();
	if (timeZone.lineType != LineType::UseKLine || timeZone.timeType != Time_Type::M30) return;

	QQuery qQuery;
	qQuery.query_type = QQueryType::BETWEEN_TIME;
	qQuery.time_pair.beginPos = timeZone.beginPos;
	qQuery.time_pair.endPos = timeZone.endPos;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(timeZone.codeId, timeZone.timeType, qQuery.time_pair);

	CModifyKline modifykline(timeZone.codeId, timeZone.timeType, klines);
	modifykline.CheckHoliday();
	modifykline.CheckDayKLineCount();
	//modifykline.CheckVol();
	//modifykline.AddSpecaildata();






	//TimeZoneOfCodeId timeZone = GetTimeZone();
	//if (timeZone.lineType != LineType::UseKLine) return;

	//TimePair timePair;
	//timePair.beginPos = timeZone.beginPos;
	//timePair.endPos = timeZone.endPos;
	//MakeAndGet_QDatabase()->RemoveKLines(timeZone.codeId, timeZone.timeType, timePair);

}

void QAnalyst::on_btnNextBidJump_clicked()
{
	QModelIndex index = ui->tableViewCtrl->currentIndex();
	index = m_model->nextBidJump(index.row() + 1);
	if (!index.isValid())
	{
		ui->resultlabel->setText("can not find next bid jump");
		return;
	}
	ui->tableViewCtrl->setCurrentIndex(index);
	ui->tableViewCtrl->scrollTo(index);
	ui->resultlabel->setText("-----------");
	return;


}

void QAnalyst::on_btnNextAskJump_clicked()
{
	QModelIndex index = ui->tableViewCtrl->currentIndex();
	index = m_model->nextAskJump(index.row() + 1);
	if (!index.isValid())
	{
		ui->resultlabel->setText("can not find next ask jump");
		return;
	}
	ui->tableViewCtrl->setCurrentIndex(index);
	ui->tableViewCtrl->scrollTo(index);
	ui->resultlabel->setText("-----------");
	return;
}



void QAnalyst::Init()
{
	// 永远不会使用仿真模式，仅使用实时模式
	Make_CurrentTime(CurrentTimeType::For_Real, 0);

	// 初始化哈希环境
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	Make_CodeIdEnv(CTrans::Get_CodeIds(contracts));

	// 行情数据库
	MakeAndGet_QDatabase();

	InitCodeId();
	InitLineType();
	InitKLineType();
	InitTimePair();
	InitColumnList();
	InitTableWidget();


}

void QAnalyst::InitCodeId()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	for (const auto& contract : contracts)
	{
		ui->codeIdCtrl->addItem(contract->codeId.c_str());
	}
}

void QAnalyst::InitLineType()
{
	ui->lineTypeCtrl->addItem(CTransToStr::Get_LineType(LineType::UseKLine).c_str());
	ui->lineTypeCtrl->addItem(CTransToStr::Get_LineType(LineType::UseTick).c_str());
}

void QAnalyst::InitKLineType()
{
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::S15).c_str());
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M1).c_str());
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M5).c_str());
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M15).c_str());
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M30).c_str());
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::H1).c_str());
	ui->klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::D1).c_str());
}

void QAnalyst::InitTimePair()
{
	QDateTime dt = QDateTime::currentDateTime();
	QString str = dt.toString();
	ui->endTimeCtrl->setDateTime(QDateTime::currentDateTime());
}

void QAnalyst::InitColumnList()
{
	std::string titel = CTransDataToStr::GetDeepTickTitle_Csv();
	std::vector<std::string> strings = CGlobal::SplitStr(titel, ",");
	QStringList headList = CQtGlobal::StdStrings_QStringList(strings);
	ui->columnListCtrl->addItems(headList);
}

void QAnalyst::InitTableWidget()
{
	LineType lineType = CTransToStr::Get_LineType(CQtGlobal::QtString_StdString(ui->lineTypeCtrl->currentText()));
	CodeStr codeId = CQtGlobal::QtString_StdString(ui->codeIdCtrl->currentText());
	m_model = MakeAndGet_TableModel(lineType, codeId);
	ui->tableViewCtrl->setModel(m_model.data());

}

TimeZoneOfCodeId QAnalyst::GetTimeZone()
{
	TimeZoneOfCodeId timeZone;
	timeZone.codeId = CQtGlobal::QtString_StdString(ui->codeIdCtrl->currentText());
	timeZone.lineType = CTransToStr::Get_LineType(CQtGlobal::QtString_StdString(ui->lineTypeCtrl->currentText()));
	timeZone.timeType = CTransToStr::Get_TimeType(CQtGlobal::QtString_StdString(ui->klineTypeCtrl->currentText()));
	timeZone.beginPos = ui->beginTimeCtrl->dateTime().toMSecsSinceEpoch();
	timeZone.endPos = ui->endTimeCtrl->dateTime().toMSecsSinceEpoch();
	return timeZone;
}



void QAnalyst::on_lineTypeCtrl_currentTextChanged(QString str)
{
	LineType lineType = CTransToStr::Get_LineType(str.toUtf8().constData());
	if (lineType == LineType::UseKLine)
	{
		ui->klineTypeCtrl->setEnabled(true);
	}
	else
	{
		ui->klineTypeCtrl->setEnabled(false);
	}
	if (!m_working) return;
	InitTableWidget();
}
void QAnalyst::on_codeIdCtrl_currentTextChanged(QString str)
{
	if (!m_working) return;

	InitTableWidget();
}
