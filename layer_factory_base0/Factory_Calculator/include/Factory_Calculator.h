#pragma once

#ifdef WIN32

#ifdef FACTORY_CALCULATOR_EXPORTS
#define FACTORY_CALCULATOR_API __declspec(dllexport)
#else
#define FACTORY_CALCULATOR_API __declspec(dllimport)
#endif


#else

#define FACTORY_CALCULATOR_API 

#endif // WIN32


#include "Calculator_Macd.h"
#include "Calculator_MacdDiv.h"
#include "Calculator_Average.h"

#include "Container_Macd.h"
#include "Container_MacdDiv.h"
#include "Container_Average.h"

#ifndef WIN32


extern "C"
{

#endif // !WIN32

	FACTORY_CALCULATOR_API Calculator_AveragePtr	Make_Calculator_Ma();
	FACTORY_CALCULATOR_API Calculator_MacdPtr		Make_Calculator_Macd();
	FACTORY_CALCULATOR_API Calculator_AveragePtr	Make_Calculator_Ema();
	FACTORY_CALCULATOR_API Calculator_MacdDivPtr	Make_Calculator_MacdDiv(Calculator_MacdPtr calculator_macd);

	FACTORY_CALCULATOR_API Container_MacdPtr		Make_Container_Macd();
	FACTORY_CALCULATOR_API Container_MacdDivPtr		Make_Container_MacdDiv(Container_MacdPtr pContainerMacd);
	FACTORY_CALCULATOR_API Container_AveragePtr		Make_Container_Ma();
	FACTORY_CALCULATOR_API Container_AveragePtr		Make_Container_Ema();



#ifndef WIN32
}
#endif // !WIN32


