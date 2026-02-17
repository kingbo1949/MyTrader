#ifndef __QDatabaseI_h__
#define __QDatabaseI_h__

#include <QDatabase.h>

namespace IBTrader
{

class IQDatabaseI : public virtual IQDatabase
{
public:

    virtual int IdlCount(const Ice::Current&) override;

    virtual bool IsAllIdle(const Ice::Current&) override;

    virtual int TaskCount(const Ice::Current&) override;

    virtual void UpdateTickToDB(ITick,
                                const Ice::Current&) override;

    virtual ITick GetLastUpdateTick(int&,
                                    long long int&,
                                    const Ice::Current&) override;

    virtual void RemoveAllTicks(::std::string,
                                const Ice::Current&) override;

    virtual void RemoveTicksByRange(::std::string,
                                    long long int,
                                    long long int,
                                    const Ice::Current&) override;

    virtual void GetTicks(::std::string,
                          IQuery,
                          ITicks&,
                          const Ice::Current&) override;

    virtual bool GetOneTick(::std::string,
                            long long int,
                            ITick&,
                            const Ice::Current&) override;

    virtual void UpdateKLine(::std::string,
                             ITimeType,
                             IKLine,
                             const Ice::Current&) override;

    virtual void UpdateKLines(::std::string,
                              ITimeType,
                              IKLines,
                              const Ice::Current&) override;

    virtual void RemoveAllKLines(::std::string,
                                 ITimeType,
                                 const Ice::Current&) override;

    virtual void RemoveKLinesByRange(::std::string,
                                     ITimeType,
                                     long long int,
                                     long long int,
                                     const Ice::Current&) override;

    virtual void RemoveOneKLine(::std::string,
                                ITimeType,
                                long long int,
                                const Ice::Current&) override;

    virtual void GetKLines(::std::string,
                           ITimeType,
                           IQuery,
                           IKLines&,
                           const Ice::Current&) override;

    virtual bool GetOneKLine(::std::string,
                             ITimeType,
                             long long int,
                             IKLine&,
                             const Ice::Current&) override;

    virtual void GetKLinePairs(::std::string,
                               ::std::string,
                               ITimeType,
                               IQuery,
                               IKLinePairs&,
                               const Ice::Current&) override;

    virtual void GetInvalidKLines(::std::string,
                                  ITimeType,
                                  IKLines&,
                                  const Ice::Current&) override;

    virtual void RecountAllIndex(::std::string,
                                 ITimeType,
                                 const Ice::Current&) override;

    virtual void UpdateAllIndexFromTimePos(::std::string,
                                           ITimeType,
                                           long long int,
                                           const Ice::Current&) override;

    virtual void GetMas(::std::string,
                        ITimeType,
                        IQuery,
                        IAvgValues&,
                        const Ice::Current&) override;

    virtual bool GetOneMa(::std::string,
                          ITimeType,
                          long long int,
                          IAvgValue&,
                          const Ice::Current&) override;

    virtual void GetVwMas(::std::string,
                          ITimeType,
                          IQuery,
                          IAvgValues&,
                          const Ice::Current&) override;

    virtual bool GetOneVwMa(::std::string,
                            ITimeType,
                            long long int,
                            IAvgValue&,
                            const Ice::Current&) override;

    virtual void GetEmas(::std::string,
                         ITimeType,
                         IQuery,
                         IAvgValues&,
                         const Ice::Current&) override;

    virtual bool GetOneEma(::std::string,
                           ITimeType,
                           long long int,
                           IAvgValue&,
                           const Ice::Current&) override;

    virtual void GetMacds(::std::string,
                          ITimeType,
                          IQuery,
                          IMacdValues&,
                          const Ice::Current&) override;

    virtual bool GetOneMacd(::std::string,
                            ITimeType,
                            long long int,
                            IMacdValue&,
                            const Ice::Current&) override;

    virtual void GetDivTypes(::std::string,
                             ITimeType,
                             IQuery,
                             IDivTypeValues&,
                             const Ice::Current&) override;

    virtual bool GetOneDivType(::std::string,
                               ITimeType,
                               long long int,
                               IDivTypeValue&,
                               const Ice::Current&) override;

    virtual void GetAtrs(::std::string,
                         ITimeType,
                         IQuery,
                         IAtrValues&,
                         const Ice::Current&) override;

    virtual bool GetOneAtr(::std::string,
                           ITimeType,
                           long long int,
                           IAtrValue&,
                           const Ice::Current&) override;
};

}

#endif
