//
// Created by fan tang on 2026/11/21.
//

#include "TableModelDataGetter.h"

void CTableModelDataGetter::Go(const std::string &codeId)
{
    m_result = MakeAndGet_QDatabase()->GetTableModelData(codeId);
    return;
}

IBTableModelDataPtr CTableModelDataGetter::GetResult()
{
    return m_result;
}

TableModelDataGetterPtr g_TableModelDataGetter = nullptr;
TableModelDataGetterPtr MakeAndGet_TableModelDataGetter()
{
    if (g_TableModelDataGetter == nullptr)
    {
        g_TableModelDataGetter = std::make_shared<CTableModelDataGetter>();
    }
    return g_TableModelDataGetter;
}
