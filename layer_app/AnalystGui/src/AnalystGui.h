#pragma once

#include <QtWidgets/QMainWindow>
#include <qstandarditemmodel.h>
#include "ui_AnalystGui.h"
#include "PlotContainer.h"
#include "Factory_AnalystGui.h"
#include "../klineShape/ScanKShape.h"
class AnalystGui : public QMainWindow
{
	Q_OBJECT

public:
	AnalystGui(QWidget* parent = nullptr);
	~AnalystGui();

public slots:
	// 通过命名规则自动链接的槽函数不需要connect指定 on_objectName_signalName
	void				on_analystTradeBtn_clicked();
	void				on_clearTradeBtn_clicked();
	void				on_isRealCtrl_clicked();
	void				on_allCodeIdView_clicked(const QModelIndex &index);			// 选择品种 
	void				on_positionCodeIdView_clicked(const QModelIndex& index);	// 选择品种 
	void				on_klineTypeCtrl_currentIndexChanged(int index);	// 选择周期

protected:
	void				onTimerRealQuote();
	void				onTimerKlineShape();

protected:

	// 热键事件
	void				keyPressEvent(QKeyEvent* event) override;

	// 启动klineShape定时器
	void 				StartKlineShapeTimer();




private:
	Ui::AnalystGuiClass		ui;
	PlotContainerQPtr		m_plotContainer;
	QTimer					m_timer_realCtrl;				// 刷新实时数据定时器
	QTimer					m_timer_klineShape;				// 刷新k线形态定时器
	bool					m_subAxisVisible;
	QStandardItemModel*		m_model_allCodeId;
	QStandardItemModel*		m_model_positionCodeId;
	ScanKShapePtr			m_pScanKShape_future;
	ScanKShapePtr			m_pScanKShape_stock;

	// 初始化控件
	void				Init();
	void				InitAllCodeId();
	void				InitPositionCodeId();
	void				InitKLineType();
	void				InitForSimulator();
	void				InitStrategyId();

	// 设置坐标可见性
	void				SetAxisVisible(MainOrSub mainOrSub);

	// 布局
	void				MakeLayout();

	CodeStr				GetSelectCodeId(QTabWidget* tabWidget);
	Time_Type			GetSelectTimeType();

	// 显示K线 选中品种和周期时使用
	void				ShowKlines(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines);

	// 得到KLINES 选中品种和周期时使用
	IBKLinePtrs			GetKLines(const CodeStr& codeId, Time_Type timeType);


};
