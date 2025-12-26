#include "CsvContract_Real.h"
#include <Log.h>
#include <Global.h>
#include <Factory_IBGlobalShare.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;
void CCsvContract_Real::LoadContractFromCsv(const std::string& fileName, IbContractPtrs& contracts)
{
	printf("%s \n", fileName.c_str());

	MakeTitleIndex(fileName);


	std::string str = CLog::Instance()->ReadFileToString(fileName);
	std::vector<std::string> strs;
	split(strs, str, is_any_of("\n"), token_compress_off);
	for (const auto& oneLine : strs)
	{
		std::string lineStr = oneLine;
		trim(lineStr);
		if (lineStr == "") continue;
		if (IsTitleLine(lineStr, "code_id")) continue;

		IbContractPtr contract = MakeContract(lineStr);
		if (!contract) continue;

		contracts.push_back(contract);
	}
	return;

}

IbContractPtr CCsvContract_Real::MakeContract(const std::string& csvStr)
{
	std::vector<std::string> strs;
	split(strs, csvStr, is_any_of(","), token_compress_off);
	strs = CGlobal::Trim(strs);

	if (strs.size() <= GetIndex("minmove"))
	{
		printf("cant split line:\n%s\n", csvStr.c_str());
		exit(-1);
	}
	if (strs[0] == "")
	{
		printf("drop line:[%s] \n", csvStr.c_str());
		return nullptr;
	}

	IbContractPtr back = std::make_shared<CIbContract>();

	if (CTransToStr::TransToLowerCaseOrUpperCase(strs[GetIndex("valid")], UpperOrLower::LowerCase) == "false") return nullptr;

	back->codeId = strs[GetIndex("code_id")];

	std::string enableStr = CTransToStr::TransToLowerCaseOrUpperCase(strs[GetIndex("enable")], UpperOrLower::LowerCase);
	back ->enable = CTransToStr::Get_Bool(enableStr);
	
	back->localSymbol = strs[GetIndex("localSymbol")];
	back->securityType = CTransToStr::Get_SecurityType(strs[GetIndex("securityType")]);
	back->exchangePl = CTransToStr::Get_ExchangePl(strs[GetIndex("exchange_pl")]) ;
	back->primaryExchangePl = CTransToStr::Get_ExchangePl(strs[GetIndex("primary_exchange_pl")]);
	back->currencyID = CTransToStr::Get_Money(strs[GetIndex("currency_id")]);
	back->decDigits = atoi(strs[GetIndex("digits")].c_str() );
	back->minMove = atof(strs[GetIndex("minmove")].c_str());

	return back;
	

}
