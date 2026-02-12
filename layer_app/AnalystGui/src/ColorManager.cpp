#include "ColorManager.h"

PlotColor CColorManager::GetColor()
{
	return GetColor_Material();
}

PlotColor CColorManager::GetColor_Original()
{
	PlotColor ret;
	ret.backGround = QColor(255, 255, 255);
	ret.brushPositive = QColor(50, 205, 50);
	ret.penPositive = QColor(34, 139, 34);
	ret.brushNegative = QColor(220, 20, 60);
	ret.penNegative = QColor(178, 34, 34);

	ret.lineProfit = QColor(Qt::darkGreen);
	ret.lineLoss = QColor(Qt::darkMagenta);

	ret.arrowBuy = QColor(Qt::darkRed);
	ret.arrowSell = QColor(Qt::darkBlue);
	return ret;
}

PlotColor CColorManager::GetColor_Material()
{
	PlotColor ret;
	ret.backGround = QColor(255, 255, 255);		// 白色
	ret.brushPositive = QColor(76, 175, 80);	// 绿色
	ret.penPositive = QColor(56, 142, 60);		// 深绿
	ret.brushNegative = QColor(244, 67, 54);	// 红色
	ret.penNegative = QColor(211, 47, 47);		// 深红色

	ret.lineProfit = QColor(0, 150, 136);		// 绿色
	ret.lineLoss = QColor(255, 87, 34);			// 红色

	ret.arrowBuy = QColor(63, 81, 181);			// 靛蓝色
	ret.arrowSell = QColor(255, 193, 7);		// 黄色

	ret.tradeSide1 = QColor(240, 240, 240);		// 浅灰
	ret.tradeSide2 = QColor(255, 250, 235);		// 木头黄

	ret.line5 = QColor(3, 169, 244);			// 蓝色
	ret.line20 = QColor(255, 193, 7);			// 黄色
	ret.line60 = QColor(156, 39, 176);			// 紫色
	ret.line200 = QColor(255, 87, 34);			// 橙色

	ret.upper = QColor(242, 54, 69);			// 番茄红
	ret.lower = QColor(8, 153, 129);			// 蓝绿

	return ret;
}


PlotColor CColorManager::GetColor_PythonDark()
{
	PlotColor ret;
	ret.backGround = QColor(30, 30, 30);		// 深灰
	ret.brushPositive = QColor(0, 193, 118);	// 亮绿色
	ret.penPositive = QColor(0, 143, 90);		// 深绿
	ret.brushNegative = QColor(255, 76, 76);	// 亮红色
	ret.penNegative = QColor(212, 42, 42);		// 深红色

	ret.lineProfit = QColor(255, 215, 0);		// 金色
	ret.lineLoss = QColor(128, 0, 128);			// 紫色

	ret.arrowBuy = QColor(30, 144, 255);		// 道奇蓝
	ret.arrowSell = QColor(255, 165, 0);		// 橙色
	return ret;
}

PlotColor CColorManager::GetColor_PythonLight()
{

	PlotColor ret;
	ret.backGround = QColor(240, 240, 240);		// 浅灰
	ret.brushPositive = QColor(34, 139, 134);	// 深绿
	ret.penPositive = QColor(0, 100, 0);		// 深绿
	ret.brushNegative = QColor(220, 20, 60);	// 深红色
	ret.penNegative = QColor(178, 34, 34);		// 深红色

	ret.lineProfit = QColor(50, 205, 50);		// 亮绿
	ret.lineLoss = QColor(255, 69, 0);			// 亮红

	ret.arrowBuy = QColor(0, 122, 204);			// 蓝
	ret.arrowSell = QColor(255, 140, 0);		// 橙色
	return ret;
}
