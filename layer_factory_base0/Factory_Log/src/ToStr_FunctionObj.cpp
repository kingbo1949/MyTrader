#include "pch.h"
#include "ToStr_FunctionObj.h"

CToStr_FunctionObj::CToStr_FunctionObj(std::function<std::string()> fr)
	:m_fr(fr)
{

}

CToStr_FunctionObj::~CToStr_FunctionObj()
{

}

std::string CToStr_FunctionObj::to_str()
{
	return m_fr();
}
