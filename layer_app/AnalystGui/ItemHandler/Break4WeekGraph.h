#pragma once
#include "ItemsHandler.h"

// 按四周规则画线
class CBreak4WeekGraph : public CItemsHandler
{
    Q_OBJECT

public:
    CBreak4WeekGraph(QCustomPlot* parent);
    virtual ~CBreak4WeekGraph() { ; };

    // 鼠标移动
    virtual void			MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override final { ; };

    // 坐标变化事件
    virtual void			AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override final { ; };

    // K线改变或者回合改变事件
    virtual void			SetKLines(const KlinePlotSuit& klinePlotSuit) override final;

    // 盘中K线更新
    virtual void			UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override final;

    // 全部更新回合
    virtual void			SetMatches(const KlinePlotSuit& klinePlotSuit) override final { ; };

    // 接受shift + key
    virtual void			NumberKey(int key) override final;


    // 得到坐标系
    virtual QCPAxisQPtr		GetX() override;
    virtual QCPAxisQPtr		GetY() override;

protected:
    // 把数据库查询做成map
    virtual bool			MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override final ;

    // 取指定位置之前的count个元素（不包括pos）
    IBKLinePtrs             GetPreviousN(const IBKLinePtrs& klines, int pos, int count);

    // 把klines的高低价做成MAP元素插入
    void                    InsertMap(const IBKLinePtrs& klines, Tick_T tickTime);

    void					SetKLines(const IBKLinePtrs& klines, HighOrLow highOrLow);

    void					UpdateKlines(const IBKLinePtrs& klines, HighOrLow highOrLow, int addcount, int chgcount);


    QCPGraphQPtr            GetGraphLine(HighOrLow highOrLow);

protected:

    QCPGraphQPtr			m_graphHigh;
    QCPGraphQPtr			m_graphLow;

    std::map<Tick_T, HighLowPair>	m_map;


};


