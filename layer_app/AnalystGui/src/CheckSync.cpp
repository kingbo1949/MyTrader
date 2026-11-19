//
// Created by kingb on 2026/5/12.
//

#include "CheckSync.h"
#include <Factory_QDatabase.h>
bool CCheckSync::IsSync(const CodeStr &codeId, Time_Type timetype, IBDivTypePtr type)
{
    if (!type->isUTurn) return false;

    CodeStr otherCodeId = GetOtherCodeId(codeId);
    if (otherCodeId.empty()) return false;

    IBDivTypePtr pOtherType = MakeAndGet_QDatabase()->GetOneDivType(otherCodeId, timetype, type->time);
    if (!pOtherType) return false;
    if (pOtherType->isUTurn) return true;

    return false;

}

CodeStr CCheckSync::GetOtherCodeId(const CodeStr &codeId)
{
    if (codeId == "NQ") return "ES";
    if (codeId == "ES") return "NQ";
    if (codeId == "SMH") return "NQ";
    if (codeId == "NVDA") return "NQ";
    if (codeId == "AVGO") return "NQ";
    if (codeId == "TSM") return "NQ";
    if (codeId == "MU") return "NQ";
    if (codeId == "AMD") return "NQ";
    if (codeId == "SOXX") return "NQ";
    if (codeId == "DRAM") return "NQ";
    if (codeId == "NASA") return "NQ";
    if (codeId == "SPCX") return "NQ";
    if (codeId == "RKLB") return "NQ";

    return "";
}
