#include "businessobjectlist.h"
#include "bitcoinunits.h"
#include "forms/ui_businessobjectlist.h"
#include "secdialog.h"
#include <masternode/masternode-sync.h>
#include "forms/ui_secdialog.h"
#include "walletmodel.h"
#include "guiutil.h"
#include "rpcpog.h"
#include <QPainter>
#include <QTableWidget>
#include <QGridLayout>
#include <QUrl>
#include <QTimer>
#include <univalue.h>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()

bool bSlotsCreated = false;

QStringList BusinessObjectList::GetHeaders(std::string sFields)
{
	QStringList pHeaders;
	sHeaderFields = sFields;
	std::vector<std::string> vFields = Split(sFields.c_str(), ",");
	for (int i = 0; i < (int)vFields.size(); i++)
	{
		std::string sFieldName = vFields[i];
		pHeaders << GUIUtil::TOQS(sFieldName);
	}
	return pHeaders;
}

BusinessObjectList::BusinessObjectList(const PlatformStyle *platformStyle, QWidget *parent) : ui(new Ui::BusinessObjectList)
{
    ui->setupUi(this);
	connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomMenuRequested(QPoint)));
    connect(ui->btnSummary, SIGNAL(clicked()), this, SLOT(showSummary()));
	connect(ui->btnDetails, SIGNAL(clicked()), this, SLOT(showDetails()));
	connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionPressed(int)),this, SLOT(HandleIndicatorChanged(int)));
	connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &BusinessObjectList::cellDoubleClicked);
}

BusinessObjectList::~BusinessObjectList()
{
    delete ui;
}

void BusinessObjectList::setModel(WalletModel *model)
{
    this->model = model;
}

void BusinessObjectList::UpdateObject(int nType)
{
	if (!fWarmBootFinished)
		return;
	std::string sMyType = nType == 0 ? "pog" : "pog_leaderboard";

	std::string sFields = "campaign,nickname,cpk,cur. addr,amount,points,owed,prominence";
    QStringList pHeaders = GetHeaders(sFields);
    QString pString = GUIUtil::TOQS(GetPOGBusinessObjectList(sMyType, sFields));
	//QTimer::singleShot(700000, this, SLOT(()));
	createUI(pHeaders, pString);
}

void BusinessObjectList::addFooterRow(int& rows, int& iFooterRow, std::string sCaption, std::string sValue)
{
	rows++;
    ui->tableWidget->setItem(rows, 0, new QTableWidgetItem(GUIUtil::TOQS(sCaption)));
	ui->tableWidget->setItem(rows, 1, new QTableWidgetItem(GUIUtil::TOQS(sValue)));
}

void BusinessObjectList::initSummOnly()
{
	UpdateObject(0);
}

void BusinessObjectList::createUI(const QStringList &headers, const QString &pStr)
{
    ui->tableWidget->setShowGrid(true);
	ui->tableWidget->setRowCount(0);
	ui->tableWidget->setSortingEnabled(false);
	
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    QVector<QVector<QString> > pMatrix;
	if (pStr == "") 
		return;

    pMatrix = SplitData(pStr);
	int rows = pMatrix.size();
	int iFooterRow = 0;
	int iAmountCol = 5;
	int iNameCol = 1;
	iFooterRow += 6;
	std::string sXML = GUIUtil::FROMQS(pStr);
	std::string msNickName = ExtractXML(sXML, "<my_nickname>","</my_nickname>");
    ui->tableWidget->setRowCount(rows + iFooterRow);
	
	if (pMatrix.size() < 1)
		return;

    int cols = pMatrix[0].size();
    ui->tableWidget->setColumnCount(cols);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
	
    QString s;
	double dGrandTotal = 0;
	int iHighlighted = 0;
	// Leaderboard fields = "nickname,cpk,points,owed,prominence";
    // Sort by ShareWeight descending (unless there is already a different one)
    int default_sort_column = 5;
    int iSortColumn = ui->tableWidget->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder soDefaultOrder = Qt::DescendingOrder;
    Qt::SortOrder soCurrentOrder = ui->tableWidget->horizontalHeader()->sortIndicatorOrder();
	ui->tableWidget->setSortingEnabled(false);
	
    if (soDefaultOrder == soCurrentOrder && iSortColumn == default_sort_column && iSortColumn > 1)   
	{
		ui->tableWidget->sortByColumn(iSortColumn, soCurrentOrder);
	}
	else
	{
		ui->tableWidget->sortByColumn(default_sort_column, soDefaultOrder);
	}
	
    for (int i = 0; i < rows; i++)
	{
		bool bHighlighted = (pMatrix[i][iNameCol] == GUIUtil::TOQS(msNickName));
		// sFields = "campaign,nickname,cpk,points,owed,prominence";

        for(int j = 0; j < cols; j++)
		{
			QTableWidgetItem* q = new QTableWidgetItem();
			bool bNumeric = (j == 6 || j == 4 || j == 5 || j == 7);
            if (bNumeric) 
			{
				double theValue = cdbl(GUIUtil::FROMQS(pMatrix[i][j]), 8);
				q->setData(Qt::DisplayRole, theValue);
				// For column 4, this value can get really high and shows in scientific notation
				if (j == 4 && theValue > 1000000)
				{
					std::string sMMValue = RoundToString(theValue/1000000, 4) + "MM";
					q->setData(Qt::DisplayRole, GUIUtil::TOQS(sMMValue));
				}
				/*
				else if (j==4)
				{
					std::string sMMValue = RoundToString(theValue, 8);
					q->setData(Qt::DisplayRole, GUIUtil::TOQS(sMMValue));
				}
				*/
            }
            else 
			{
				q->setData(Qt::EditRole, pMatrix[i][j]); 
            }
			ui->tableWidget->setItem(i, j, q);
		}
		if (bHighlighted)
		{
			ui->tableWidget->selectRow(i);
			ui->tableWidget->item(i, iNameCol)->setBackground(Qt::yellow);
			iHighlighted = i;
		}

		if (iAmountCol > -1)
		{
			dGrandTotal += cdbl(GUIUtil::FROMQS(pMatrix[i][iAmountCol]), 5);
		}
	}
	
	if (true)
	{
		ui->tableWidget->setSortingEnabled(false);
	
		addFooterRow(rows, iFooterRow, "Difficulty:", ExtractXML(sXML, "<difficulty>","</difficulty>"));
		addFooterRow(rows, iFooterRow, "My Points:", ExtractXML(sXML, "<my_points>","</my_points>"));
		addFooterRow(rows, iFooterRow, "My Nick Name:", ExtractXML(sXML, "<my_nickname>","</my_nickname>"));
		addFooterRow(rows, iFooterRow, "Total Points:", ExtractXML(sXML, "<total_points>","</total_points>"));
		addFooterRow(rows, iFooterRow, "Total Participants:", ExtractXML(sXML, "<participants>","</participants>"));

		if (iHighlighted > 0) 
		{
			ui->tableWidget->selectRow(iHighlighted);
		}
		std::string sLowBlock = ExtractXML(sXML, "<lowblock>", "</lowblock>");
		std::string sHighBlock = ExtractXML(sXML, "<highblock>", "</highblock>");
		std::string sHeading = "Leaderboard v1.3 - Range " + sLowBlock + " to " + sHighBlock + " - DWU " + RoundToString(CalculateUTXOReward() * 100, 2) + "%";

		// Label Heading
		ui->lblLeaderboard->setText(GUIUtil::TOQS(sHeading));
	}

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->resizeRowsToContents();
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	// Column widths should be set 
	for (int j=0; j < cols; j++)
	{
		ui->tableWidget->setColumnWidth(j, 128);
	}

}

void BusinessObjectList::cellDoubleClicked(int Y, int X)
{
	QTableWidgetItem *item1(ui->tableWidget->item(Y, 2));
	if (item1)
	{
		std::string sCPK = GUIUtil::FROMQS(item1->text()); // CPK
		CAmount nBBPQty = 0;
		std::string sSumm = GetUTXOSummary(sCPK, nBBPQty);
		
		if (!sSumm.empty())
		{
			std::string sTitle = "My Portfolio - " + sCPK;
			QMessageBox::information(this, GUIUtil::TOQS(sTitle), GUIUtil::TOQS(sSumm), QMessageBox::Information, QMessageBox::Information);
			/*
			QMessageBox msgBox;
			msgBox.setWindowTitle(GUIUtil::TOQS(sTitle));
			msgBox.setText(GUIUtil::TOQS(sSumm));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setStyleSheet("QLabel{xmin-height: 800px;background-color:black;color:silver;}");
			int r = msgBox.exec();
			*/
		}
	}
}


void BusinessObjectList::HandleIndicatorChanged(int logicalIndex)
{
	if (logicalIndex != 0 && logicalIndex != 1)
	{
		ui->tableWidget->horizontalHeader()->setSortIndicatorShown(true);
		Qt::SortOrder soCurrentOrder = ui->tableWidget->horizontalHeader()->sortIndicatorOrder();
		ui->tableWidget->sortByColumn(logicalIndex, soCurrentOrder);
	}
}

void BusinessObjectList::slotCustomMenuRequested(QPoint pos)
{
    return;

    /* Create an object context menu 
    QMenu * menu = new QMenu(this);
    //  Create, Connect and Set the actions to the menu
    menu->addAction(tr("Navigate To"), this, SLOT(slotNavigateTo()));
	menu->addAction(tr("List"), this, SLOT(slotList()));
	menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));
	*/
}

int BusinessObjectList::GetUrlColumn(std::string sTarget)
{
	boost::to_upper(sTarget);
	std::vector<std::string> vFields = Split(sHeaderFields.c_str(), ",");
	for (int i = 0; i < (int)vFields.size(); i++)
	{
		std::string sFieldName = vFields[i];
		boost::to_upper(sFieldName);
		if (sFieldName == sTarget) return i;
	}
	return -1;
}

void BusinessObjectList::slotList()
{
    int row = ui->tableWidget->selectionModel()->currentIndex().row();
    if(row >= 0)
    {
		// Navigate to the List of the Object
        std::string sID = GUIUtil::FROMQS(ui->tableWidget->item(row, 0)->text()); 
		int iCol = GetUrlColumn("object_name");
		if (iCol > -1)
		{
			std::string sTarget = GUIUtil::FROMQS(ui->tableWidget->item(row, iCol)->text());
			// Close existing menu
			//UpdateObject(0);
		}
    }
}

void BusinessObjectList::showSummary()
{
	UpdateObject(0);
}

void BusinessObjectList::showDetails()
{
	UpdateObject(1);
}

void BusinessObjectList::slotNavigateTo()
{
    int row = ui->tableWidget->selectionModel()->currentIndex().row();
    if(row >= 0)
    {
		// Open the URL
        QMessageBox msgBox;
		int iURLCol = GetUrlColumn("URL");
		if (iURLCol > -1)
		{
			QString Url = ui->tableWidget->item(row, iURLCol)->text();
			QUrl pUrl(Url);
			QDesktopServices::openUrl(pUrl);
		}
    }
}

QVector<QVector<QString> > BusinessObjectList::SplitData(const QString &pStr)
{
	QStringList proposals = pStr.split(QRegExp("<object>"),QString::SkipEmptyParts);
    int nProposals = proposals.size();
    QVector<QVector<QString> > proposalMatrix;
    for (int i=0; i < nProposals; i++)
    {
        QStringList proposalDetail = proposals[i].split(QRegExp("<col>"));
        int detailSize = proposalDetail.size();
		if (detailSize > 1)
		{
			proposalMatrix.append(QVector<QString>());
			for (int j = 0; j < detailSize; j++)
			{
				QString sData = proposalDetail[j];
				/*  Reserved for BitcoinUnits
					sData = BitcoinUnits::format(2, cdbl(GUIUtil::FROMQS(sData), 2) * 100, false, BitcoinUnits::separatorAlways);		
				*/
				proposalMatrix[i].append(sData);
			}
		}
    }
	return proposalMatrix;
}
