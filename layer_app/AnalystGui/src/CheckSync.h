//
// Created by kingb on 2026/5/12.
//

#pragma once

#include <base_struc.h>

class CCheckSync
{
public:
    CCheckSync() { ; };
    virtual ~CCheckSync() { ; };

    // 判断codeId是否处于同步状态
    bool					IsSync(const CodeStr& codeId, Time_Type timetype, IBDivTypePtr type);


protected:

    CodeStr		            GetOtherCodeId(const CodeStr& codeId);

};

