#include "AnalystGui.h"

#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include <Factory_QDatabase.h>
#include <QtGlobal.h>
#include <Global.h>
#include <Factory_AnalystDeal.h>
#include "../tableModel/TableModel.h"
#include "Factory_AnalystGui.h"
#include "../klineShape/ScanKShape_Future.h"
#include "../klineShape/ScanKShape_Stock.h"


AnalystGui::AnalystGui(QWidget* parent)
	: QMainWindow(parent), m_subAxisVisible(true)
{
	//setAttribute(Qt::WA_DeleteOnClose);

	ui.setupUi(this);
	ui.qCustomPlot->plotLayout()->clear();

	// 创建主图区域（用于显示 K 线）
	QCPAxisRect* mainAxisRect = new QCPAxisRect(ui.qCustomPlot);
	SetAxisRect(MainOrSub::MainT, mainAxisRect);

	// 创建附图区域（用于显示 MACD 指标）
	QCPAxisRect* subAxisRect = new QCPAxisRect(ui.qCustomPlot);
	SetAxisRect(MainOrSub::SubT, subAxisRect);

	MakeLayout();


	// 主图背景
	mainAxisRect->setBackground(MakeAndGet_ColorManager()->GetColor().backGround);


	// 坐标可见性
	SetAxisVisible(MainOrSub::MainT);
	SetAxisVisible(MainOrSub::SubT);

	m_plotContainer = new CPlotContainer(ui.qCustomPlot);

	QMargins margins = ui.qCustomPlot->axisRect()->margins();
	//qDebug() << "left:" << margins.left() << "right:" << margins.right();

	Init();

	// treeView
	//ui.maView->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: gray; }");
	ui.maView->horizontalHeader()->setStyleSheet("");

	ui.maView->setModel(MakeAndGet_TableModel(TableViewType::Ma).data());
	ui.macdView->setModel(MakeAndGet_TableModel(TableViewType::Macd).data());

	// 设置自适应宽度
	ui.maView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.macdView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	// 设置不显示行头
	ui.maView->verticalHeader()->hide();
	ui.macdView->verticalHeader()->hide();

	// 选中一个代码
	if (m_model_allCodeId->rowCount() > 0)
	{
		QItemSelectionModel* selectModel = ui.allCodeIdView->selectionModel();
		QModelIndex index = m_model_allCodeId->index(0, 0);
		selectModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
		ui.allCodeIdView->scrollTo(index);

		on_allCodeIdView_clicked(index);
	}

	// 显示K线形态
	onTimerKlineShape();
	




}

AnalystGui::~AnalystGui()
{
	ReleaseQDatabase();
}



void AnalystGui::Init()
{
	// 行情发生器永远不会使用仿真模式，仅使用实时模式
	Make_CurrentTime(CurrentTimeType::For_Real, 0);

	// 初始化哈希环境
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);
	Make_CodeIdEnv(CTrans::Get_CodeIds(contracts));

	// 初始化策略哈希属性表
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	Make_StrategyParamEnv(strategyParams);


	// 行情数据库
	MakeAndGet_QDatabase();

	InitAllCodeId();
	InitPositionCodeId();
	InitKLineType();
	InitStrategyId();
	InitForSimulator();

	ui.splitter_3->setSizes({100, 1, 300});
	ui.splitter->setSizes({300, 100});

	StartKlineShapeTimer();

	

	

}

void AnalystGui::InitAllCodeId()
{
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::True);

	m_model_allCodeId = new QStandardItemModel(int(contracts.size()), 1, ui.allCodeIdView);

	// 填充数据
	for (int row = 0; row < contracts.size(); ++row)
	{
		QStandardItem* item = new QStandardItem(contracts[row]->codeId.c_str());
		m_model_allCodeId->setItem(row, 0, item);
	}

	// 绑定
	ui.allCodeIdView->setModel(m_model_allCodeId);

	// 列宽自适应
	ui.allCodeIdView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


}

void AnalystGui::InitPositionCodeId()
{
	QStringList codeIds = { "NQ", "GC", "TSLA", "HOOD", "MU", "CLS", "SOFI", "BMNR", "ORCL", "COIN", "STX", "AVGO", "MSTR", "INTC", "TSM"};

	m_model_positionCodeId = new QStandardItemModel(int(codeIds.size()), 1, ui.positionCodeIdView);

	// 填充数据
	for (int row = 0; row < codeIds.size(); ++row)
	{
		QStandardItem* item = new QStandardItem(codeIds[row]);
		m_model_positionCodeId->setItem(row, 0, item);
	}

	// 绑定
	ui.positionCodeIdView->setModel(m_model_positionCodeId);
	

	// 列宽自适应
	ui.positionCodeIdView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void AnalystGui::InitKLineType()
{
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::S15).c_str());
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M1).c_str());
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M5).c_str());
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M15).c_str());
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::M30).c_str());
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::H1).c_str());
	ui.klineTypeCtrl->addItem(CTransToStr::Get_TimeType(Time_Type::D1).c_str());
}


void AnalystGui::InitForSimulator()
{
	// 按文件Simulator_TimeZone.json的内容初始化
	TimeZoneOfCodeIdPtr timeZone = MakeAndGet_JSonTimeZone(TimeZone_Type::For_Simulator)->Load_TimeZone();

	// 初始化品种
	//ui.codeIdCtrl->setCurrentText(CQtGlobal::StdString_QString(timeZone->codeId));

	// 初始化K线周期
	//std::string klineType = CTransToStr::Get_TimeType(timeZone->timeType);
	//ui.klineTypeCtrl->setCurrentText(CQtGlobal::StdString_QString(klineType));
	ui.klineTypeCtrl->setCurrentText(CQtGlobal::StdString_QString("M30"));

	// 初始化起始终止时间
	QDateTime dt;
	dt.setMSecsSinceEpoch(timeZone->beginPos);
	ui.beginTimeCtrl->setDateTime(dt);

	dt.setMSecsSinceEpoch(timeZone->endPos);
	ui.endTimeCtrl->setDateTime(dt);



}

void AnalystGui::InitStrategyId()
{
	// 按文件strategy_param.json的内容初始化
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	for (auto strategy : strategyParams)
	{
		ui.strategyIdCtrl->addItem(CQtGlobal::StdString_QString(strategy->key.ToStr()));
	}
}

void AnalystGui::SetAxisVisible(MainOrSub mainOrSub)
{
	QCPAxisRectQPtr axisRect = GetAxisRect(mainOrSub);

	if (mainOrSub == MainOrSub::MainT)
	{
		axisRect->axis(QCPAxis::atBottom)->setVisible(true);
	}
	else
	{
		axisRect->axis(QCPAxis::atBottom)->setVisible(false);
	}

	axisRect->axis(QCPAxis::atTop)->setVisible(false);
	axisRect->axis(QCPAxis::atLeft)->setVisible(false);
	axisRect->axis(QCPAxis::atRight)->setVisible(true);
	return;
}

void AnalystGui::MakeLayout()
{

	while (ui.qCustomPlot->plotLayout()->elementCount() > 0)
	{
		ui.qCustomPlot->plotLayout()->takeAt(0);
		ui.qCustomPlot->plotLayout()->simplify();
	}

	if (m_subAxisVisible)
	{
		// 设置布局：将主图放在第 0 行，附图放在第 1 行
		ui.qCustomPlot->plotLayout()->addElement(0, 0, GetAxisRect(MainOrSub::MainT));  // 添加主图
		ui.qCustomPlot->plotLayout()->addElement(1, 0, GetAxisRect(MainOrSub::SubT));  // 添加附图

		ui.qCustomPlot->plotLayout()->setRowStretchFactor(0, 4); // 主图占 4 份
		ui.qCustomPlot->plotLayout()->setRowStretchFactor(1, 1); // 附图占 1 份

		ui.qCustomPlot->plotLayout()->setColumnStretchFactor(0, 1);
		ui.qCustomPlot->plotLayout()->setColumnStretchFactor(1, 0);
		ui.qCustomPlot->plotLayout()->setColumnSpacing(0);

	}
	else
	{
		ui.qCustomPlot->plotLayout()->addElement(0, 0, GetAxisRect(MainOrSub::MainT));  // 添加主图
		ui.qCustomPlot->plotLayout()->setRowStretchFactor(0, 1); // 主图占 4 份

	}
	ui.qCustomPlot->plotLayout()->simplify();

}

CodeStr AnalystGui::GetSelectCodeId(QTabWidget* tabWidget)
{
	QTableView* pView = nullptr;
	if (tabWidget->currentIndex() == 0)
	{
		pView = ui.allCodeIdView;
	}
	else
	{
		pView = ui.positionCodeIdView;
	}

	
	// 得到codeId
	QItemSelectionModel* selectModel = pView->selectionModel();
	QList<QModelIndex> selectedRows = selectModel->selectedRows();
	if (selectedRows.isEmpty()) return "";

	int row = selectedRows.first().row();
	QStandardItemModel* model = qobject_cast<QStandardItemModel*>(pView->model());
	QString data = model->item(row, 0)->text();
	std::string codeId = CQtGlobal::QtString_StdString(data);

	return 	codeId;
}

Time_Type AnalystGui::GetSelectTimeType()
{
	Time_Type timeType = CTransToStr::Get_TimeType(CQtGlobal::QtString_StdString(ui.klineTypeCtrl->currentText()));
	return timeType;
}

void AnalystGui::ShowKlines(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines)
{
	m_plotContainer->SetKlines(codeId, timeType, klines);
	MakeAndGet_TableModel(TableViewType::Ma)->SetCodeId(codeId);
	MakeAndGet_TableModel(TableViewType::Macd)->SetCodeId(codeId);

}

IBKLinePtrs AnalystGui::GetKLines(const CodeStr& codeId, Time_Type timeType)
{
	IBKLinePtrs klines;
	if (ui.isTailKLineCtrl->isChecked())
	{
		QQuery query;
		query.query_type = QQueryType::FROM_CURRENT;
		query.query_number = 300;
		klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	}
	else
	{
		TimePair timePair;
		timePair.beginPos = ui.beginTimeCtrl->dateTime().toMSecsSinceEpoch();
		timePair.endPos = ui.endTimeCtrl->dateTime().toMSecsSinceEpoch();

		klines = MakeAndGet_QDatabase()->GetKLineByLoop(codeId, timeType, timePair);

	}

	return klines;
}





void AnalystGui::on_allCodeIdView_clicked(const QModelIndex& index)
{
	if (!index.isValid()) return;

	std::string codeId = GetSelectCodeId(ui.tabWidget);

	Time_Type timeType = GetSelectTimeType();

	IBKLinePtrs klines = GetKLines(codeId, timeType);;
	ShowKlines(codeId, timeType, klines);

	
}

void AnalystGui::on_positionCodeIdView_clicked(const QModelIndex& index)
{
	std::string codeId = GetSelectCodeId(ui.tabWidget);

	Time_Type timeType = GetSelectTimeType();

	IBKLinePtrs klines = GetKLines(codeId, timeType);;
	ShowKlines(codeId, timeType, klines);

}

void AnalystGui::on_klineTypeCtrl_currentIndexChanged(int index)
{
	if (index < 0) return;
	// 得到codeId

	std::string codeId = GetSelectCodeId(ui.tabWidget);

	// 得到timeType
	Time_Type timeType = GetSelectTimeType();

	IBKLinePtrs klines = GetKLines(codeId, timeType);;
	ShowKlines(codeId, timeType, klines);



}

void AnalystGui::on_analystTradeBtn_clicked()
{
	std::map<StrategyId, SimpleMatchPtrs> results = MakeAndGet_ScanDealsForSimpleMatchs()->Make_SimpleMatchs_FromJson();
	StrategyId strategyId = CQtGlobal::QtString_StdString(ui.strategyIdCtrl->currentText());

	m_plotContainer->SetTMatches(results[strategyId]);


}
void AnalystGui::on_clearTradeBtn_clicked()
{
	SimpleMatchPtrs matches;
	m_plotContainer->SetTMatches(matches);
}
void AnalystGui::onTimerRealQuote()
{
	std::string codeId = GetSelectCodeId(ui.tabWidget);
	Time_Type timeType = GetSelectTimeType();

	// 查询最近两根K线
	QQuery query;
	query.query_type = QQueryType::FROM_CURRENT;
	query.query_number = 2;

	IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLine(codeId, timeType, query);
	m_plotContainer->AddKlines(codeId, timeType, klines);

	MakeAndGet_TableModel(TableViewType::Ma)->SetCodeId(codeId);
	MakeAndGet_TableModel(TableViewType::Macd)->SetCodeId(codeId);
}
void AnalystGui::onTimerKlineShape()
{
	if (!m_pScanKShape_future)
	{
		ui.futureView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		ui.futureView->verticalHeader()->hide();
		m_pScanKShape_future = std::make_shared<CScanKShape_Future>(ui.futureView);
	}
	if (!m_pScanKShape_stock)
	{
		ui.stockView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		ui.stockView->verticalHeader()->hide();
		m_pScanKShape_stock = std::make_shared<CScanKShape_Stock>(ui.stockView);
	}
	m_pScanKShape_future->Go();
	m_pScanKShape_stock->Go();
	return;

}
void AnalystGui::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_End)
	{
		// 按下了 End 键
		m_plotContainer->GoEnd();
	}
	else if (event->key() == Qt::Key_Home)
	{
		m_plotContainer->GoHome();
	}
	else if (event->key() == Qt::Key_H)
	{
		// 按下 'H' 键
		m_subAxisVisible = !m_subAxisVisible; // 切换可见状态
		GetAxisRect(MainOrSub::SubT)->setVisible(m_subAxisVisible); // 设置附图的可见性

		MakeLayout();

	}
	else if (event->key() == Qt::Key_T)
	{
		// 按下 'T' 键
		if (Get_TableViewMouse() == TableViewMouse::Mouse)
		{
			Set_TableViewMouse(TableViewMouse::NoMouse);
		}
		else
		{
			Set_TableViewMouse(TableViewMouse::Mouse);
		}

		
	}


	ui.qCustomPlot->plotLayout()->updateLayout();
	ui.qCustomPlot->replot(); // 重新绘制

	QWidget::keyPressEvent(event); // 调用父类的事件处理函数
}
void AnalystGui::StartKlineShapeTimer()
{
	QObject::connect(&m_timer_klineShape, &QTimer::timeout, this, &AnalystGui::onTimerKlineShape);
	m_timer_klineShape.start(1000 * 60);  // 1分钟刷新一次

}
void AnalystGui::on_isRealCtrl_clicked()
{
	if (!ui.isRealCtrl->isChecked())
	{
		m_timer_realCtrl.stop();
		return;
	}

	QObject::connect(&m_timer_realCtrl, &QTimer::timeout, this, &AnalystGui::onTimerRealQuote);
	m_timer_realCtrl.start(1000);  // 1000毫秒刷新一次

}


