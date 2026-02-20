#pragma once
#include "SubGraph.h"
class CAtrGraph: public CSubGraph
{
    Q_OBJECT

public:
    CAtrGraph(QCustomPlot* parent , SubType subType);
    virtual ~CAtrGraph() { ; };

    // 鼠标移动
    virtual void			    MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override final { ; };

    // 坐标变化事件
    virtual void			    AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override final ;

    // K线改变或者回合改变事件
    virtual void			    SetKLines(const KlinePlotSuit& klinePlotSuit) override final ;

    // 盘中K线更新
    virtual void			    UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override final ;

    // 全部更新回合
    virtual void			    SetMatches(const KlinePlotSuit& klinePlotSuit) override final { ; };

    IBAtrPtr                    GetAtrValue(Tick_T time);


protected:
    // 把数据库查询做成map
    virtual bool			    MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) final ;

private:
    std::mutex				    m_mutex;
    std::map<Tick_T, IBAtrPtr>	m_mapAtrValue;
    QCPGraphQPtr			    m_atr;

    // [xBeginPos, xEndPos)内Atr的高低点
    void					    GetHighLow(Tick_T xBeginPos, Tick_T xEndPos, double& high, double& low);


};


