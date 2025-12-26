// JsonCsvTransfer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <base_trade.h>
#include <Factory_IBGlobalShare.h>
#include <Factory_IBJSon.h>
#include <Factory_HashEnv.h>
#include "CsvStrategy_Real.h"
#include "CsvContract_Real.h"
#include "CsvTradePoints_Real.h"

void	CvsToJson_StrategyParams();
void	CvsToJson_Contracts();
void	CvsToJson_TradePoints();

void	Init_HashEnv();


int main()
{
	Make_CurrentTime(CurrentTimeType::For_Real, 0);

	CvsToJson_Contracts();
	CvsToJson_StrategyParams();
	CvsToJson_TradePoints();

	printf("ready \n");


	printf("EXIT_SUCCESS! \n");
	return 0;
}

void CvsToJson_StrategyParams()
{
	std::string cvsFileName = "./csv/strategyParams.csv";

	StrategyTotalParamExs paramsForPs;

	StrategyTotalParamExs params;

	params.clear();

	CsvStrategyPtr pCsvStrategy = std::make_shared<CCsvStrategy_Real>();
	pCsvStrategy->LoadStrategyTotalParams(cvsFileName, params);

	// 保存StrategyParam到json
	MakeAndGet_JSonStrategyParam()->Save_StrategyParams(pCsvStrategy->GetStrategyTotalParams(params));

	// 开始制作持仓json
	Init_HashEnv();
	pCsvStrategy->GetStrategyTotalParamsForPs(params, paramsForPs);
	pCsvStrategy->SaveStrategyTotalParamsForPs(paramsForPs);
	return;



}


void CvsToJson_Contracts()
{
	std::string cvsFileName = "./csv/contracts.csv";

	IbContractPtrs contracts;

	CsvContractPtr pCsvTrans = std::make_shared<CCsvContract_Real>();
	pCsvTrans->LoadContractFromCsv(cvsFileName, contracts);

	MakeAndGet_JSonContracts()->Save_Contracts(contracts);

	return;

}

void Init_HashEnv()
{
	// 初始化属性表部分
	IbContractPtrs contracts;
	MakeAndGet_JSonContracts()->Load_Contracts(contracts, SelectType::All);
	Make_CodeIdEnv(CTrans::Get_CodeIds(contracts));

	// 初始化策略哈希属性表
	StrategyParamPtrs strategyParams;
	MakeAndGet_JSonStrategyParam()->Load_StrategyParams(strategyParams);
	Make_StrategyParamEnv(strategyParams);

}
void CvsToJson_TradePoints()
{
	std::string cvsFileName = "./csv/tradePoints.csv";

	TradePointPtrs tradePoints;

	CsvTradePointsPtr pCsvTrans = std::make_shared<CCsvTradePoints_Real>();
	pCsvTrans->LoadTradePointsFromCsv(cvsFileName, tradePoints);
	MakeAndGet_JSonTradePoint()->Save_TradePoints(tradePoints);

	return;

}


