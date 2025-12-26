#pragma once
#include "ItemsHandler.h"

// 在不同品种上画水平直线
class CLineDrawer : public CItemsHandler
{
	Q_OBJECT

public:
	CLineDrawer(QCustomPlot* parent); 
	virtual ~CLineDrawer() { ; };

	// 鼠标移动
	virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override { ; };

	// 坐标变化事件
	virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override { ; };

	// K线改变或者回合改变事件
	virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) override;

	// 盘中K线更新
	virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override { ; };

	// 全部更新回合
	virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override { ; };

	// 得到坐标系
	virtual QCPAxisQPtr		GetX() override;
	virtual QCPAxisQPtr		GetY() override;

protected:
	// 把数据库查询做成map
	virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final { return true; };

	// 得到一根水平线对象
	QCPItemLine*			MakeOneLine(double yValue);

	// 清除水平线对象群
	void					ClearLines();
private:
	std::map<CodeStr, std::vector<double>>	m_codeIdToLines;	// 品种对应的水平线
	std::vector< QCPItemLine*>				m_lines;			// 水平线对象	


};

