//
// Created by fan tang on 2026/11/21.
//

#pragma once
#include <memory>
#include <vector>
#include <Factory_QDatabase.h>
class CTableModelDataGetter
{
public:
    explicit CTableModelDataGetter() { ; };
    virtual ~CTableModelDataGetter() { ; };

    void                Go(const ::std::string& codeId);
    IBTableModelDataPtr GetResult();
protected:
    IBTableModelDataPtr m_result;

};
typedef std::shared_ptr<CTableModelDataGetter> TableModelDataGetterPtr;
typedef std::vector<TableModelDataGetterPtr> TableModelDataGetterPtrs;

TableModelDataGetterPtr         MakeAndGet_TableModelDataGetter();


