#pragma once

#include <QtWidgets/QWidget>
// #include "ui_QAnalyst.h"
#include "./TableModel/TableModel.h"

namespace Ui { class QAnalystClass; }

class QAnalyst : public QWidget
{
	Q_OBJECT

public:
	QAnalyst(QWidget* parent = nullptr);
	~QAnalyst();

public slots:
	void				on_lineTypeCtrl_currentTextChanged(QString str);
	void				on_codeIdCtrl_currentTextChanged(QString str);
	void				on_goBtn_clicked();
	void				on_saveBtn_clicked();
	void				on_tickStatisButton_clicked();

	// 按照输入的时间定位记录
	void				on_timeGotoButton_clicked();
	void				on_delBtn_clicked();

	// 定位到下一个bid jump之处
	void				on_btnNextBidJump_clicked();
	// 定位到下一个ask jump之处
	void				on_btnNextAskJump_clicked();




private:
	Ui::QAnalystClass* ui = nullptr;
	 // Ui::QAnalystClass	ui;
	bool				m_working;
	TableModelQSPtr		m_model;

	// 初始化控件
	void				Init();
	void				InitCodeId();
	void				InitLineType();
	void				InitKLineType();
	void				InitTimePair();
	void				InitColumnList();
	void				InitTableWidget();

	TimeZoneOfCodeId	GetTimeZone();


};
