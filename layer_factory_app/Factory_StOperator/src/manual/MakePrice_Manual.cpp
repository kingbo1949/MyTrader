#include "../pch.h"
#include "MakePrice_Manual.h"
#include <Factory_UnifyInterface.h>
#include <Factory_IBGlobalShare.h>
#include <Global.h>
CMakePrice_Manual::CMakePrice_Manual(const SubModuleParams& stParams)
	:CMakePrice_Real(stParams)
{

}

bool CMakePrice_Manual::GetOpenPrice(double& price, RealPriceType& priceType)
{
	priceType = RealPriceType::Original;
	if (Get_Sys_Status() != Thread_Status::Running) return false;

	PositionSizePtr oppositePs = GetOppositPs();
	if (oppositePs)
	{
		// 反向持仓存在，不要开仓
		return false;
	}

	if (m_stParams.buyOrSell == BuyOrSell::Buy && Get_Manual_Status() == ManualStatus::BuyToOpen)
	{
		return GetOpenCoverPrice(price);
	}
	if (m_stParams.buyOrSell == BuyOrSell::Sell && Get_Manual_Status() == ManualStatus::SellToOpen)
	{
		return GetOpenCoverPrice(price);
	}
	return false;




}

bool CMakePrice_Manual::GetCoverPrice(double& price, RealPriceType& priceType)
{
	priceType = RealPriceType::Original;
	if (m_stParams.buyOrSell == BuyOrSell::Buy && Get_Manual_Status() == ManualStatus::BuyToCover)
	{
		return GetOpenCoverPrice(price);
	}
	if (m_stParams.buyOrSell == BuyOrSell::Sell && Get_Manual_Status() == ManualStatus::SellToCover)
	{
		return GetOpenCoverPrice(price);
	}

	return false;

}

bool CMakePrice_Manual::GetOpenCoverPrice(double& price)
{
	BuyOrSell buyOrSell = m_stParams.buyOrSell;
	IBTickPtr ptick = m_targetTickEnv->pTick;

	// 挂单开平仓，位置由opendiff和coverdiff指定
	int pos = 0;
	if (m_stParams.openOrCover == RoundAction::Open_Round)
	{
		pos = -1;
	}
	else
	{
		pos = -1;
	}

	int priceStatus = CHighFrequencyGlobalFunc::GetPriceByPriceStatus(m_targetTickEnv->pTick, buyOrSell, pos);
	price = priceStatus * m_stParams.contract->minMove;

	return true;


}
