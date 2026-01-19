#include "ScanKShape.h"
#include <Factory_IBJSon.h>
#include <Factory_IBGlobalShare.h>
#include "KLineShape_Div.h"
#include <Global.h>

CScanKShape::CScanKShape(QTableView* pTableView)
	:m_showTableView(pTableView)
{
	InitTableHeader();
	return;
}
void CScanKShape::Go()
{
	Show(Scan());
}
KShapeOutputs CScanKShape::Scan()
{
	KLineShapePtrs kLineShapePackets = MakeKLineShapes();

	KShapeOutputs outputs;
	for (const auto& kLineShapePacket : kLineShapePackets)
	{
		KShapeOutputs partOutputs = kLineShapePacket->Execute();
		outputs.insert(outputs.end(), partOutputs.begin(), partOutputs.end());
	}

	return outputs;
}

void CScanKShape::Show(const KShapeOutputs& kShapeOutputs)
{
	// 删除所有的行
	m_model_kshape->removeRows(0, m_model_kshape->rowCount());

	// 填充数据
	for (const auto& kShapeOutput : kShapeOutputs)
	{
		if (kShapeOutput.kShape == KShape::Normal) continue;

		TableViewRecord record = TransToTableRecord(kShapeOutput);
		m_model_kshape->appendRow(record);
		//int newRow = m_model_kshape->rowCount();
		//m_model_kshape->insertRow(newRow, record);
	}

	m_showTableView->show();

	return;
}

void CScanKShape::InitTableHeader()
{
	QStringList	headers = { "Code", "Time", "Top&Btm", "Shape", "Pos"};
	m_model_kshape = new QStandardItemModel(1, headers.size(), m_showTableView);
	m_model_kshape->setHorizontalHeaderLabels(headers);

	// 绑定
	m_showTableView->setModel(m_model_kshape);

	return;
}

TableViewRecord CScanKShape::TransToTableRecord(const KShapeOutput& kShapeOutput)
{
	TableViewRecord record;
	record.push_back(new QStandardItem(kShapeOutput.codeId.c_str()));
	record.push_back(new QStandardItem(CTransToStr::Get_TimeType(kShapeOutput.timeType).c_str()));
	record.push_back(new QStandardItem(Trans_Str(kShapeOutput.topOrBottom).c_str()));
	record.push_back(new QStandardItem(Trans_Str(kShapeOutput.kShape).c_str()));

	auto positionItem = new QStandardItem(std::to_string(kShapeOutput.signalPosition).c_str()) ;
	if (kShapeOutput.signalPosition < 3)
	{
		positionItem->setBackground(QBrush(QColor(255, 192, 203)));	// 粉红色
	}
	record.push_back(positionItem);

	return record;
}

