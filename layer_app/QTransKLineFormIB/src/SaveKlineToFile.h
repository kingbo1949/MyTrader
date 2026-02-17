#pragma once
#include <base_struc.h>

// 查询K线并保存成文件，迁移数据库时需要使用这个功能
class CSaveKlineToFile
{
public:
    CSaveKlineToFile() { ; };
    virtual ~CSaveKlineToFile() { ; };

    void	    Go();
protected:
    void        QueryAndSaveKLines(const CodeStr& codeId);
    IBKLinePtrs	QueryKLine(const CodeStr& codeId, Time_Type timeType);
    void	    SaveKLine(const CodeStr& codeId, Time_Type timeType, const IBKLinePtrs& klines);

    std::string GetFileName(const CodeStr& codeId, Time_Type timeType);
    std::string GetKlineStr(const IBKLinePtr &kline);

};


