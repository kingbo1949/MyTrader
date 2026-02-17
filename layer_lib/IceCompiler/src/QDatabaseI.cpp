
#include <QDatabaseI.h>

int
IBTrader::IQDatabaseI::IdlCount(const Ice::Current& current)
{
    return 0;
}

bool
IBTrader::IQDatabaseI::IsAllIdle(const Ice::Current& current)
{
    return false;
}

void
IBTrader::IQDatabaseI::UpdateTickToDB(ITick /*tick*/,
                                      const Ice::Current& current)
{
}

::IBTrader::ITick
IBTrader::IQDatabaseI::GetLastUpdateTick(int& updateCount,
                                         long long int& recentUpdateSecnd,
                                         const Ice::Current& current)
{
    updateCount = 0;
    recentUpdateSecnd = 0;
    return ITick();
}

void
IBTrader::IQDatabaseI::RemoveAllTicks(::std::string /*codeId*/,
                                      const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::RemoveTicksByRange(::std::string /*codeId*/,
                                          long long int /*beginTime*/,
                                          long long int /*endTime*/,
                                          const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::GetTicks(::std::string /*codeId*/,
                                IQuery /*query*/,
                                ITicks& ticks,
                                const Ice::Current& current)
{
    ticks = ITicks();
}

bool
IBTrader::IQDatabaseI::GetOneTick(::std::string /*codeId*/,
                                  long long int /*timePos*/,
                                  ITick& tick,
                                  const Ice::Current& current)
{
    tick = ITick();
    return false;
}

void
IBTrader::IQDatabaseI::UpdateKLine(::std::string /*codeId*/,
                                   ITimeType /*timeType*/,
                                   IKLine /*kline*/,
                                   const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::RemoveAllKLines(::std::string /*codeId*/,
                                       ITimeType /*timeType*/,
                                       const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::RemoveKLinesByRange(::std::string /*codeId*/,
                                           ITimeType /*timeType*/,
                                           long long int /*beginTime*/,
                                           long long int /*endTime*/,
                                           const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::RemoveOneKLine(::std::string /*codeId*/,
                                      ITimeType /*timeType*/,
                                      long long int /*timePos*/,
                                      const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::GetKLines(::std::string /*codeId*/,
                                 ITimeType /*timeType*/,
                                 IQuery /*query*/,
                                 IKLines& klines,
                                 const Ice::Current& current)
{
    klines = IKLines();
}

bool
IBTrader::IQDatabaseI::GetOneKLine(::std::string /*codeId*/,
                                   ITimeType /*timeType*/,
                                   long long int /*timePos*/,
                                   IKLine& kline,
                                   const Ice::Current& current)
{
    kline = IKLine();
    return false;
}

void
IBTrader::IQDatabaseI::GetKLinePairs(::std::string /*first*/,
                                     ::std::string /*second*/,
                                     ITimeType /*timeType*/,
                                     IQuery /*query*/,
                                     IKLinePairs& kLinePairs,
                                     const Ice::Current& current)
{
    kLinePairs = IKLinePairs();
}

void
IBTrader::IQDatabaseI::GetInvalidKLines(::std::string /*codeId*/,
                                        ITimeType /*timeType*/,
                                        IKLines& klines,
                                        const Ice::Current& current)
{
    klines = IKLines();
}

void
IBTrader::IQDatabaseI::RecountAllIndex(::std::string /*codeId*/,
                                       ITimeType /*timeType*/,
                                       const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::UpdateAllIndexFromTimePos(::std::string /*codeId*/,
                                                 ITimeType /*timeType*/,
                                                 long long int /*timePos*/,
                                                 const Ice::Current& current)
{
}

void
IBTrader::IQDatabaseI::GetMas(::std::string /*codeId*/,
                              ITimeType /*timeType*/,
                              IQuery /*query*/,
                              IAvgValues& mas,
                              const Ice::Current& current)
{
    mas = IAvgValues();
}

bool
IBTrader::IQDatabaseI::GetOneMa(::std::string /*codeId*/,
                                ITimeType /*timeType*/,
                                long long int /*timePos*/,
                                IAvgValue& ma,
                                const Ice::Current& current)
{
    ma = IAvgValue();
    return false;
}

void
IBTrader::IQDatabaseI::GetVwMas(::std::string /*codeId*/,
                                ITimeType /*timeType*/,
                                IQuery /*query*/,
                                IAvgValues& mas,
                                const Ice::Current& current)
{
    mas = IAvgValues();
}

bool
IBTrader::IQDatabaseI::GetOneVwMa(::std::string /*codeId*/,
                                  ITimeType /*timeType*/,
                                  long long int /*timePos*/,
                                  IAvgValue& ma,
                                  const Ice::Current& current)
{
    ma = IAvgValue();
    return false;
}

void
IBTrader::IQDatabaseI::GetEmas(::std::string /*codeId*/,
                               ITimeType /*timeType*/,
                               IQuery /*query*/,
                               IAvgValues& emas,
                               const Ice::Current& current)
{
    emas = IAvgValues();
}

bool
IBTrader::IQDatabaseI::GetOneEma(::std::string /*codeId*/,
                                 ITimeType /*timeType*/,
                                 long long int /*timePos*/,
                                 IAvgValue& ema,
                                 const Ice::Current& current)
{
    ema = IAvgValue();
    return false;
}

void
IBTrader::IQDatabaseI::GetMacds(::std::string /*codeId*/,
                                ITimeType /*timeType*/,
                                IQuery /*query*/,
                                IMacdValues& macds,
                                const Ice::Current& current)
{
    macds = IMacdValues();
}

bool
IBTrader::IQDatabaseI::GetOneMacd(::std::string /*codeId*/,
                                  ITimeType /*timeType*/,
                                  long long int /*timePos*/,
                                  IMacdValue& ma,
                                  const Ice::Current& current)
{
    ma = IMacdValue();
    return false;
}

void
IBTrader::IQDatabaseI::GetDivTypes(::std::string /*codeId*/,
                                   ITimeType /*timeType*/,
                                   IQuery /*query*/,
                                   IDivTypeValues& divtypes,
                                   const Ice::Current& current)
{
    divtypes = IDivTypeValues();
}

bool
IBTrader::IQDatabaseI::GetOneDivType(::std::string /*codeId*/,
                                     ITimeType /*timeType*/,
                                     long long int /*timePos*/,
                                     IDivTypeValue& divtype,
                                     const Ice::Current& current)
{
    divtype = IDivTypeValue();
    return false;
}

void
IBTrader::IQDatabaseI::GetAtrs(::std::string /*codeId*/,
                               ITimeType /*timeType*/,
                               IQuery /*query*/,
                               IAtrValues& avgAtrs,
                               const Ice::Current& current)
{
    avgAtrs = IAtrValues();
}

bool
IBTrader::IQDatabaseI::GetOneAtr(::std::string /*codeId*/,
                                 ITimeType /*timeType*/,
                                 long long int /*timePos*/,
                                 IAtrValue& avgAtr,
                                 const Ice::Current& current)
{
    avgAtr = IAtrValue();
    return false;
}
