//
// Created by kingb on 2026/2/16.
//

#include "SaveKlineToFile.h"
#include <Factory_QDatabase.h>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <Log.h>
void CSaveKlineToFile::Go()
{
    QueryAndSaveKLines("NQ");
    QueryAndSaveKLines("ES");
    QueryAndSaveKLines("GC");
    QueryAndSaveKLines("MBT");
    QueryAndSaveKLines("ETHUSDRR");

    QueryAndSaveKLines("AAPL");
    QueryAndSaveKLines("TSLA");
    QueryAndSaveKLines("ORCL");
    QueryAndSaveKLines("SOXX");
    QueryAndSaveKLines("AVGO");



    // QueryAndSaveKLines("NVDA");
    // QueryAndSaveKLines("AMD");
    // QueryAndSaveKLines("META");
    // QueryAndSaveKLines("NFLX");
    // QueryAndSaveKLines("MSFT");
    // QueryAndSaveKLines("AMZN");
    // QueryAndSaveKLines("COST");
    // QueryAndSaveKLines("GOOG");
    // QueryAndSaveKLines("TSM");
    // QueryAndSaveKLines("INTC");
    // QueryAndSaveKLines("HOOD");
    // QueryAndSaveKLines("COIN");
    // QueryAndSaveKLines("MSTR");
    // QueryAndSaveKLines("CRCL");
    // QueryAndSaveKLines("BMNR");
    // QueryAndSaveKLines("PLTR");
    // QueryAndSaveKLines("MU");
    // QueryAndSaveKLines("WDC");
    // QueryAndSaveKLines("STX");
    // QueryAndSaveKLines("SOFI");
    // QueryAndSaveKLines("VRT");
    // QueryAndSaveKLines("GLXY");
    // QueryAndSaveKLines("RKLB");
    // QueryAndSaveKLines("ASTS");
    // QueryAndSaveKLines("HIMS");
    // QueryAndSaveKLines("ALAB");
    // QueryAndSaveKLines("CLS");
    // QueryAndSaveKLines("SNDK");
    // QueryAndSaveKLines("KTOS");
    // QueryAndSaveKLines("UPST");
}

void CSaveKlineToFile::QueryAndSaveKLines(const CodeStr &codeId)
{
    IBKLinePtrs klines = QueryKLine(codeId, Time_Type::M1);
    SaveKLine(codeId, Time_Type::M1, klines);

    klines = QueryKLine(codeId, Time_Type::M5);
    SaveKLine(codeId, Time_Type::M5, klines);

    klines = QueryKLine(codeId, Time_Type::M15);
    SaveKLine(codeId, Time_Type::M15, klines);

    klines = QueryKLine(codeId, Time_Type::M30);
    SaveKLine(codeId, Time_Type::M30, klines);

    klines = QueryKLine(codeId, Time_Type::H1);
    SaveKLine(codeId, Time_Type::H1, klines);

    klines = QueryKLine(codeId, Time_Type::D1);
    SaveKLine(codeId, Time_Type::D1, klines);

    return;

}

IBKLinePtrs CSaveKlineToFile::QueryKLine(const CodeStr &codeId, Time_Type timeType)
{
    Tick_T beginTime = CGlobal::GetTimeByStr("20240101 00:00:00") * 1000;
    Tick_T endTime = Get_CurrentTime()->GetCurrentTime_millisecond();
    // Tick_T endTime = CGlobal::GetTimeByStr("20240101");
    TimePair timePair;
    timePair.beginPos = beginTime;
    timePair.endPos = endTime;

    IBKLinePtrs klines = MakeAndGet_QDatabase()->GetKLineByLoop(codeId, timeType, timePair);
    return klines;
}

void CSaveKlineToFile::SaveKLine(const CodeStr &codeId, Time_Type timeType, const IBKLinePtrs &klines)
{
    std::string fileName = GetFileName(codeId, timeType);
    CLog::Instance()->DelLogFile(fileName);

    std::string klinetitle = "time,open,high,low,close,vol";
    CLog::Instance()->PrintStrToFile(fileName, klinetitle + "\n");

    std::string all_lineStr = "";
    for (size_t i = 0; i < klines.size(); ++i)
    {
        std::string str = GetKlineStr(klines[i]) + "\n";
        all_lineStr += str;

    }
    CLog::Instance()->PrintStrToFile(fileName, all_lineStr + "\n");

    return;


}
std::string CSaveKlineToFile::GetFileName(const CodeStr& codeId, Time_Type timeType)
{
    std::string filename = fmt::format("{}_kline_{}.csv",
        codeId.c_str(), CTransToStr::Get_TimeType(timeType)
    );
    return filename;
}
std::string CSaveKlineToFile::GetKlineStr(const IBKLinePtr &kline)
{
    std::string temstr = fmt::format(
        "{},"					    // 时间
        "{:.2f},"					// 开盘
        "{:.2f},"					// 最高
        "{:.2f},"					// 最低
        "{:.2f},"					// close
        "{},"						// vol
        ,
        CGlobal::GetTickTimeStr(kline->time).substr(0, 17).c_str(),			// 取时间字符串不要毫秒部分
        kline->open,
        kline->high,
        kline->low,
        kline->close,
        kline->vol
    );

    return temstr;

}
