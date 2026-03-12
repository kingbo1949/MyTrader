//
// Created by kingb on 2026/2/17.
//

#include "QueryThreadPoolStatus.h"
#include <Factory_Log.h>
#include <Factory_QDatabase.h>
#include <Global.h>


void CQueryThreadPoolStatus::Go()
{
    int tastCount = MakeAndGet_QDatabase()->TaskCount();
    int idlCount = MakeAndGet_QDatabase()->IdlCount();
    Log_Print(LogLevel::Info, fmt::format("threadPool taskCount = {}, idlCount = {}", tastCount, idlCount));

    // Test("20250116 12:00:00");
    // Test("20250116 16:01:00");
    // Test("20250116 18:01:00");


}

void CQueryThreadPoolStatus::Test(const std::string &timestr)
{
    //std::string timeStr = "20250116 12:00:00";
    IBKLinePtr kline = MakeAndGet_QDatabase()->GetLastDayKLine("NQ", CGlobal::GetTimeByStr(timestr) * 1000);
    if (kline)
    {
        Log_Print(LogLevel::Info, fmt::format("in = {}, out kline.time = {}", timestr.c_str(), CGlobal::GetTickTimeStr(kline->time).c_str()));
    }
}
