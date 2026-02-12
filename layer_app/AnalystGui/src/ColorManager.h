#pragma once
#include <qcolor.h>
struct PlotColor
{
	QColor backGround;
	QColor brushPositive;
	QColor penPositive;
	QColor brushNegative;
	QColor penNegative;

	QColor line5;
	QColor line20;
	QColor line60;
	QColor line200;

	// 盘后时段 两个颜色交替使用
	QColor tradeSide1;			// 盘后时段1
	QColor tradeSide2;			// 盘后时段2

	QColor lineProfit;
	QColor lineLoss;
	QColor arrowBuy;
	QColor arrowSell;

	QColor upper;
	QColor lower;
};
class CColorManager
{
public:
	CColorManager() { ; };
	virtual ~CColorManager() { ; };

	PlotColor		GetColor();
protected:
	PlotColor		GetColor_Original();
	PlotColor		GetColor_Material();
	PlotColor		GetColor_PythonDark();
	PlotColor		GetColor_PythonLight();




};
typedef std::shared_ptr<CColorManager> ColorManagerPtr;
