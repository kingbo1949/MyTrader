#pragma once

#include <Factory_QDatabase.h>
#include "KLineConverter_NoMix.h"

class CConvertContractsKlines
{
public:
    CConvertContractsKlines(const std::set<CodeStr>& coodeIds);
    virtual ~CConvertContractsKlines() { ; };

    void		            Go();

protected:
    std::set<CodeStr>       m_codeIds;
    CKLineConverter_NoMix   m_klineConverter_nomix;

    void                    ConvertOneContractKLines(const CodeStr& codeId);
};

