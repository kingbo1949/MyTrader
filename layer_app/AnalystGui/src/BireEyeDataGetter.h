//
// Created by fan tang on 2026/11/22.
//

#pragma once
#include <memory>
#include <vector>
#include <Factory_QDatabase.h>

class CBireEyeDataGetter
{
public:
    explicit CBireEyeDataGetter() { ; };
    virtual ~CBireEyeDataGetter() { ; };

    void                Go(const ::std::string& codeId);
    IBBirdEyeViewValues GetLastOne();
    IBBirdEyeViewValues GetThisOne();
protected:
    CodeStr             m_codeId;
    IBBirdEyeViewValues m_lastOne;
    IBBirdEyeViewValues m_thisOne;

};
typedef std::shared_ptr<CBireEyeDataGetter> BireEyeDataGetterPtr;

BireEyeDataGetterPtr         MakeAndGet_BireEyeDataGetter();


