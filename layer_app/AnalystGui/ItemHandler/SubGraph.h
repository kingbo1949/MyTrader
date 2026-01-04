#pragma once
#include "ItemsHandler.h"

// 副图中的图形对象
class CSubGraph : public CItemsHandler
{
    Q_OBJECT

public:
    CSubGraph(QCustomPlot* parent, SubType subType);
    virtual ~CSubGraph() { ; };

    // 鼠标移动
    virtual void			    MouseMoveEvent(QMouseEvent* event, const KlinePlotSuit& klinePlotSuit) override = 0;

    // 坐标变化事件
    virtual void			    AxisRangeChgEvent(const KlinePlotSuit& klinePlotSuit) override = 0 ;

    // K线改变或者回合改变事件
    virtual void			    SetKLines(const KlinePlotSuit& klinePlotSuit) override = 0 ;

    // 盘中K线更新
    virtual void			    UpdateKlines(const KlinePlotSuit& klinePlotSuit, const KlineChgCount& klineChgCount) override = 0 ;

    // 全部更新回合
    virtual void			    SetMatches(const KlinePlotSuit& klinePlotSuit) override = 0;

    // 副图类别改变
    virtual void			    SubTypeChg(const KlinePlotSuit& klinePlotSuit) final;


    // 得到坐标系
    virtual QCPAxisQPtr		    GetX()  final;
    virtual QCPAxisQPtr		    GetY()  final;

protected:
    // 图对象属于哪一类副图类别
    SubType                     m_subType;

    // 把数据库查询做成map
    virtual bool			    MakeMap(const CodeStr& codeId, Time_Type timetype, const IBKLinePtrs& klines, int beginPos) override = 0 ;

    // 从横轴的范围得到数据范围的索引
    void					    GetWindowsPos(int& beginPos, int& endPos);

    // 判断是否处于自己的类别中
    bool                        IsMyType();



};


