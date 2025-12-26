#include "pch.h"
#include "Factory_Calculator.h"

#include "Calculator_Ema.h"
#include "Calculator_Ma.h"
#include "Calculator_Macd_Real.h"
#include "Calculator_MacdDiv_Real.h"

#include "Container_Macd_Real.h"
#include "Container_MacdDiv_Real.h"
#include "Container_Ma.h"
#include "Container_Ema.h"

FACTORY_CALCULATOR_API Calculator_AveragePtr Make_Calculator_Ma()
{
	return std::make_shared<CCalculator_Ma>();
}

FACTORY_CALCULATOR_API Calculator_MacdPtr Make_Calculator_Macd()
{
	return std::make_shared<CCalculator_Macd_Real>();
}

FACTORY_CALCULATOR_API Calculator_AveragePtr Make_Calculator_Ema()
{
	return std::make_shared<CCalculator_Ema>();
}

FACTORY_CALCULATOR_API Calculator_MacdDivPtr Make_Calculator_MacdDiv(Calculator_MacdPtr calculator_macd)
{
	return std::make_shared<CCalculator_MacdDiv_Real>(calculator_macd);
}

FACTORY_CALCULATOR_API Container_MacdPtr Make_Container_Macd()
{
	return std::make_shared<CContainer_Macd_Real>();
}

FACTORY_CALCULATOR_API Container_MacdDivPtr Make_Container_MacdDiv(Container_MacdPtr pContainerMacd)
{
	return std::make_shared<CContainer_MacdDiv_Real>(pContainerMacd);
}

FACTORY_CALCULATOR_API Container_AveragePtr Make_Container_Ma()
{
	return std::make_shared<CContainer_Ma>();
}

FACTORY_CALCULATOR_API Container_AveragePtr Make_Container_Ema()
{
	return std::make_shared<CContainer_Ema>();
}
