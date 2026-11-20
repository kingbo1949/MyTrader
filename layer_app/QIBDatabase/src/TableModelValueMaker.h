//
// Created by fan tang on 2026/11/21.
//

#pragma once
#include <QDatabase.h>
using namespace IBTrader;
using namespace Ice;

class CTableModelValueMaker
{
public:
    explicit CTableModelValueMaker();
    ~CTableModelValueMaker(){;};

    void                            SetCodeId(const std::string& codeId);
    bool                            Go();

    ITableModelValue		        GetResult();

protected:
    std::string                     m_codeId;
    ITableModelValue                m_tableModelValue;


    bool                            FillClose();
    bool                            FillTableModelValue();
    bool                            FillTableModelValueOneItem(ITimeType timeType, ITableModelValueOneItem& tableModelValueOneItem);

};


