//
// Created by fan tang on 2026/11/22.
//

#include "BireEyeDataGetter.h"

BireEyeDataGetterPtr g_BireEyeDataGetter = nullptr;

void CBireEyeDataGetter::Go(const std::string &codeId)
{
    m_codeId = codeId;
    MakeAndGet_QDatabase()->GetBirdEyeViewData(m_codeId, m_lastOne, m_thisOne);
}

IBBirdEyeViewValues CBireEyeDataGetter::GetLastOne()
{
    return m_lastOne;
}

IBBirdEyeViewValues CBireEyeDataGetter::GetThisOne()
{
    return m_thisOne;
}

BireEyeDataGetterPtr MakeAndGet_BireEyeDataGetter()
{
    if (g_BireEyeDataGetter == nullptr)
    {
        g_BireEyeDataGetter = std::make_shared<CBireEyeDataGetter>();
    }
    return g_BireEyeDataGetter;

}
