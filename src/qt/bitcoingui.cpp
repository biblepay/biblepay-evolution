// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2019 The Däsh Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoingui.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/modaloverlay.h>
#include <qt/networkstyle.h>
#include <qt/notificator.h>
#include <qt/openuridialog.h>
#include <qt/optionsdialog.h>
#include <qt/optionsmodel.h>
#include <qt/rpcconsole.h>
#include <qt/utilitydialog.h>
#include "menudialog.h"
#include "chatdialog.h"
#include "utilitydialog.h"
#include "businessobjectlist.h"
#include "generictabledialog.h"
#include "rpcpog.h"
#include "sendcoinsdialog.h"

#ifdef ENABLE_WALLET
#include <privatesend/privatesend-client.h>
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include <qt/macdockiconhandler.h>
#endif

#include <chainparams.h>
#include <init.h>
#include <ui_interface.h>
#include <util.h>
#include <masternode/masternode-sync.h>
#include <qt/masternodelist.h>

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#if QT_VERSION < 0x050000
#include <QTextDocument>
#include <QUrl>
#else
#include <QUrlQuery>
#endif

const std::string BitcoinGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

/** Display name for default wallet name. Uses tilde to avoid name
 * collisions in the future with additional wallets */
const QString BitcoinGUI::DEFAULT_WALLET = "~Default";

BitcoinGUI::BitcoinGUI(const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    enableWallet(false),
    clientModel(0),
    walletFrame(0),
    unitDisplayControl(0),
    labelWalletEncryptionIcon(0),
    labelWalletHDStatusIcon(0),
    labelConnectionsIcon(0),
    labelBlocksIcon(0),
    progressBarLabel(0),
    progressBar(0),
    progressDialog(0),
    appMenuBar(0),
    overviewAction(0),
	sendCoinsAction(0),
    sendCoinsMenuAction(0),
    privateSendCoinsAction(0),
    privateSendCoinsMenuAction(0),
	receiveCoinsAction(0),
    receiveCoinsMenuAction(0),
    historyAction(0),
    masternodeAction(0),
    webAction(0),
	leaderboardAction(0),
    proposalListAction(0),
	orphanAction(0),
	exchangeAction(0),
	quitAction(0),
 	sinnerAction(0),
    TheLordsPrayerAction(0),
    TheApostlesCreedAction(0),
    TheNiceneCreedAction(0),
	TheTenCommandmentsAction(0),
    JesusConciseCommandmentsAction(0),
	ReadBibleAction(0),
	OneClickMiningAction(0),
	openChatGeneralAction(0),
    openChatPMAction(0),
	openChatPMEncryptedAction(0),
	univAction(0),
	proposalAddMenuAction(0),
	nftAddMenuAction(0),
	utxoAddMenuAction(0),
	listNFTMenuAction(0),
	mailSendMenuAction(0),
	userEditMenuAction(0),
	memorizeScriptureMenuAction(0),
	aboutAction(0),
    optionsAction(0),
    toggleHideAction(0),
    encryptWalletAction(0),
    backupWalletAction(0),
    changePassphraseAction(0),
	unlockWalletAction(0),
	lockWalletAction(0),
	signMessageAction(0),
    verifyMessageAction(0),
	openInfoAction(0),
	openRPCConsoleAction(0),
	openGraphAction(0),
	openPeersAction(0),
	openRepairAction(0),
	openConfEditorAction(0),
	showBackupsAction(0),
	usedSendingAddressesAction(0),
    usedReceivingAddressesAction(0),
    openAction(0),
	showHelpMessageAction(0),
    showPrivateSendHelpAction(0),
    trayIcon(0),
    trayIconMenu(0),
    dockIconMenu(0),
    notificator(0),
    rpcConsole(0),
    helpMessageDialog(0),
    modalOverlay(0),
    tabGroup(0),
    timerConnecting(0),
    timerSpinner(0)
{

	/* Open CSS when configured */

    QSettings settings;
    if (!restoreGeometry(settings.value("MainWindowGeometry").toByteArray())) {
        // Restore failed (perhaps missing setting), center the window
		LogPrintf("\nBitcoinGUI::Launch Failed to restore geometry - centering %f", 03162021);
        move(QApplication::desktop()->availableGeometry().center() - frameGeometry().center());
    }
	static bool fDebugged = false;
	if (!fDebugged)
	{
		fDebugged = true;
		std::string sMySettings = settings.fileName().toUtf8().constData();
		LogPrintf("\nBIBLEPAY::loadStyleSheet::Using QT settings file location %s \n", sMySettings);
	}

    QString windowTitle = tr(PACKAGE_NAME) + " - ";
#ifdef ENABLE_WALLET
    enableWallet = WalletModel::isWalletEnabled();
#endif // ENABLE_WALLET
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }
    QString userWindowTitle = QString::fromStdString(gArgs.GetArg("-windowtitle", ""));
    if(!userWindowTitle.isEmpty()) windowTitle += " - " + userWindowTitle;
    windowTitle += " " + networkStyle->getTitleAddText();
    QApplication::setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC) && QT_VERSION < 0x050000
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    rpcConsole = new RPCConsole(this, enableWallet ? Qt::Window : Qt::Widget);
    helpMessageDialog = new HelpMessageDialog(this, HelpMessageDialog::cmdline, 0, uint256S("0x0"), "");
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame*/
        walletFrame = new WalletFrame(this);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon(networkStyle);

    // Create status bar
    statusBar();

    // Disable size grip because it looks ugly and nobody needs it
    statusBar()->setSizeGripEnabled(false);

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    unitDisplayControl = new UnitDisplayStatusBarControl();
    labelWalletEncryptionIcon = new QLabel();
    labelWalletHDStatusIcon = new QLabel();
    labelConnectionsIcon = new GUIUtil::ClickableLabel();

    labelBlocksIcon = new GUIUtil::ClickableLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelWalletHDStatusIcon);
        frameBlocksLayout->addWidget(labelWalletEncryptionIcon);
    }
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Hide the spinner/synced icon by default to avoid
    // that the spinner starts before we have any connections
    labelBlocksIcon->hide();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(true);
    progressBarLabel->setObjectName("lblStatusBarProgress");
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(true);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #F8F8F8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #00CCFF, stop: 1 #33CCFF); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    // Jump to peers tab by clicking on connections icon
    connect(labelConnectionsIcon, SIGNAL(clicked(QPoint)), this, SLOT(showPeers()));

    modalOverlay = new ModalOverlay(this->centralWidget());
#ifdef ENABLE_WALLET
    if(enableWallet) {
        connect(walletFrame, SIGNAL(requestedSyncWarningInfo()), this, SLOT(showModalOverlay()));
        connect(labelBlocksIcon, SIGNAL(clicked(QPoint)), this, SLOT(showModalOverlay()));
        connect(progressBar, SIGNAL(clicked(QPoint)), this, SLOT(showModalOverlay()));
    }
#endif

#ifdef Q_OS_MAC
    m_app_nap_inhibitor = new CAppNapInhibitor;
#endif

    incomingTransactionsTimer = new QTimer(this);
    incomingTransactionsTimer->setSingleShot(true);
    connect(incomingTransactionsTimer, SIGNAL(timeout()), SLOT(showIncomingTransactions()));
}

BitcoinGUI::~BitcoinGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    QSettings settings;
    settings.setValue("MainWindowGeometry", saveGeometry());
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete m_app_nap_inhibitor;
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

    delete rpcConsole;
    delete tabGroup;
}

void BitcoinGUI::startSpinner()
{
    if (labelBlocksIcon == nullptr || labelBlocksIcon->isHidden() || timerSpinner != nullptr) {
        return;
    }
    auto getNextFrame = []() {
        static std::vector<std::unique_ptr<QPixmap>> vecFrames;
        static std::vector<std::unique_ptr<QPixmap>>::iterator itFrame;
        while (vecFrames.size() < SPINNER_FRAMES) {
            QString&& strFrame = QString("spinner-%1").arg(vecFrames.size(), 3, 10, QChar('0'));
            QPixmap&& frame = getIcon(strFrame, GUIUtil::ThemedColor::ORANGE, MOVIES_PATH).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE);
            itFrame = vecFrames.insert(vecFrames.end(), std::make_unique<QPixmap>(frame));
        }
        assert(vecFrames.size() == SPINNER_FRAMES);
        if (itFrame == vecFrames.end()) {
            itFrame = vecFrames.begin();
        }
        return *itFrame++->get();
    };

    timerSpinner = new QTimer(this);
    QObject::connect(timerSpinner, &QTimer::timeout, [=]() {
        if (timerSpinner == nullptr) {
            return;
        }
        labelBlocksIcon->setPixmap(getNextFrame());
    });
    timerSpinner->start(40);
}

void BitcoinGUI::stopSpinner()
{
    if (timerSpinner == nullptr) {
        return;
    }
    timerSpinner->deleteLater();
    timerSpinner = nullptr;
}

void BitcoinGUI::startConnectingAnimation()
{
    static int nStep{-1};
    const int nAnimationSteps = 10;

    if (timerConnecting != nullptr) {
        return;
    }

    timerConnecting = new QTimer(this);
    QObject::connect(timerConnecting, &QTimer::timeout, [=]() {

        if (timerConnecting == nullptr) {
            return;
        }

        QString strImage;
        GUIUtil::ThemedColor color;

        nStep = (nStep + 1) % (nAnimationSteps + 1);
        if (nStep == 0) {
            strImage = "connect_4";
            color = GUIUtil::ThemedColor::ICON_ALTERNATIVE_COLOR;
        } else if (nStep == nAnimationSteps / 2) {
            strImage = "connect_1";
            color = GUIUtil::ThemedColor::ORANGE;
        } else {
            return;
        }
        labelConnectionsIcon->setPixmap(GUIUtil::getIcon(strImage, color).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    });
    timerConnecting->start(100);
}

void BitcoinGUI::stopConnectingAnimation()
{
    if (timerConnecting == nullptr) {
        return;
    }
    timerConnecting->deleteLater();
    timerConnecting = nullptr;
}

void BitcoinGUI::createActions()
{
    tabGroup = new QButtonGroup(this);

    overviewAction = new QToolButton(this);
    overviewAction->setText(tr("&Overview"));
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
	overviewAction->setIcon(QIcon(":/icons/overview"));
	overviewAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	overviewAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tabGroup->addButton(overviewAction);

    sendCoinsAction = new QToolButton(this);
    sendCoinsAction->setText(tr("&Send"));
    sendCoinsAction->setStatusTip(tr("Send coins to a BiblePay address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
	sendCoinsAction->setIcon(QIcon(":/icons/send"));
	sendCoinsAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	sendCoinsAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	tabGroup->addButton(sendCoinsAction);

    sendCoinsMenuAction = new QAction(sendCoinsAction->text(), this);
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

    privateSendCoinsAction = new QToolButton(this);
    privateSendCoinsAction->setText("&PrivateSend");
    privateSendCoinsAction->setStatusTip(tr("PrivateSend coins to a BiblePay address"));
    privateSendCoinsAction->setToolTip(privateSendCoinsAction->statusTip());
    privateSendCoinsAction->setCheckable(false);
    //tabGroup->addButton(privateSendCoinsAction);

    privateSendCoinsMenuAction = new QAction(privateSendCoinsAction->text(), this);
    privateSendCoinsMenuAction->setStatusTip(privateSendCoinsAction->statusTip());
    privateSendCoinsMenuAction->setToolTip(privateSendCoinsMenuAction->statusTip());

    receiveCoinsAction = new QToolButton(this);
    receiveCoinsAction->setText(tr("&Receive"));
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and biblepay: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    tabGroup->addButton(receiveCoinsAction);

    receiveCoinsMenuAction = new QAction(receiveCoinsAction->text(), this);
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

    historyAction = new QToolButton(this);
    historyAction->setText(tr("&Transactions"));
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
	historyAction->setIcon(QIcon(":/icons/history"));
	historyAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	historyAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	tabGroup->addButton(historyAction);

#ifdef ENABLE_WALLET
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction = new QToolButton(this);
        masternodeAction->setText(tr("&Sanctuaries"));
        masternodeAction->setStatusTip(tr("Browse Sanctuaries"));
        masternodeAction->setToolTip(masternodeAction->statusTip());
        masternodeAction->setCheckable(true);

		masternodeAction->setIcon(QIcon(":/icons/masternodes"));
		masternodeAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		masternodeAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        tabGroup->addButton(masternodeAction);
        connect(masternodeAction, SIGNAL(clicked()), this, SLOT(showNormalIfMinimized()));
        connect(masternodeAction, SIGNAL(clicked()), this, SLOT(gotoMasternodePage()));
    }
#endif // ENABLE_WALLET

	// BBP

	webAction = new QToolButton(this);
	webAction->setIcon(QIcon(":/icons/account32"));
	webAction->setText(tr("&Web"));
	webAction->setStatusTip(tr("Web"));
	webAction->setToolTip(webAction->statusTip());
	webAction->setCheckable(false);
	webAction->setVisible(false);
	tabGroup->addButton(webAction);

	leaderboardAction = new QToolButton(this);
	leaderboardAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	leaderboardAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	leaderboardAction->setText(tr("&Leaderboard"));
	leaderboardAction->setStatusTip(tr("Leaderboard"));
	leaderboardAction->setIcon(QIcon(":/icons/address-book"));

	leaderboardAction->setCheckable(true);
	leaderboardAction->setVisible(true);
	tabGroup->addButton(leaderboardAction);
	connect(leaderboardAction, SIGNAL(clicked()), this, SLOT(gotoBusinessObjectListPage()));

	proposalListAction = new QToolButton(this);
	proposalListAction->setIcon(QIcon(":/icons/address-book"));
	proposalListAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	proposalListAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	proposalListAction->setText(tr("&Proposals"));
	proposalListAction->setStatusTip(tr("Show Proposals"));
	proposalListAction->setToolTip(proposalListAction->statusTip());
	proposalListAction->setCheckable(true);

	proposalListAction->setIcon(QIcon(":/icons/edit"));
	proposalListAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	proposalListAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	
	tabGroup->addButton(proposalListAction);
	connect(proposalListAction, SIGNAL(clicked()), this, SLOT(gotoProposalListPage()));

	orphanAction = new QToolButton(this);
	orphanAction->setText("&Accounting");
	orphanAction->setStatusTip(tr("Show Accountability Page"));
	orphanAction->setToolTip(orphanAction->statusTip());
	orphanAction->setCheckable(true);
	orphanAction->setVisible(true);
	orphanAction->setIcon(QIcon(":/icons/editcopy"));
	orphanAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	orphanAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	tabGroup->addButton(orphanAction);
	connect(orphanAction, SIGNAL(clicked()), this, SLOT(showAccountability()));

	exchangeAction = new QToolButton(this);

	exchangeAction->setText(tr("&Exchange"));
	exchangeAction->setStatusTip(tr("Show Exchange"));
	exchangeAction->setToolTip(exchangeAction->statusTip());
	exchangeAction->setCheckable(true);
	exchangeAction->setIcon(QIcon(":/icons/notsynced"));
	exchangeAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	exchangeAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	
	tabGroup->addButton(exchangeAction);
	connect(exchangeAction, SIGNAL(clicked()), this, SLOT(showExchange()));

	// End of BBP

    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(clicked()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(clicked()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(clicked()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(clicked()), this, SLOT(gotoSendCoinsPage()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
	if (1==0)
	{
		connect(privateSendCoinsAction, SIGNAL(clicked()), this, SLOT(showNormalIfMinimized()));
		connect(privateSendCoinsAction, SIGNAL(clicked()), this, SLOT(gotoPrivateSendCoinsPage()));
		connect(privateSendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
		connect(privateSendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoPrivateSendCoinsPage()));
	}
    connect(receiveCoinsAction, SIGNAL(clicked()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(clicked()), this, SLOT(gotoReceiveCoinsPage()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(clicked()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(clicked()), this, SLOT(gotoHistoryPage()));

    for (auto button : tabGroup->buttons()) {
        GUIUtil::setFont({button}, GUIUtil::FontWeight::Normal, 16);
        if (walletFrame == nullptr) {
            // hide buttons when there is no wallet
            button->setVisible(false);
        }
    }
    GUIUtil::updateFonts();

    // Give the selected tab button a bolder font.
    connect(tabGroup, SIGNAL(buttonToggled(QAbstractButton *, bool)), this, SLOT(highlightTabButton(QAbstractButton *, bool)));

    quitAction = new QAction(tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);

	// BBP

	sinnerAction = new QAction(QIcon(":/icons/address-book"), tr("The Sinners Prayer"), this);
    sinnerAction->setStatusTip(tr("Show the Sinners Prayer"));
    sinnerAction->setMenuRole(QAction::AboutRole);

    TheLordsPrayerAction = new QAction(QIcon(":/icons/address-book"), tr("The Lords Prayer"), this);
    TheLordsPrayerAction->setStatusTip(tr("Show the Lords Prayer"));

    TheApostlesCreedAction = new QAction(QIcon(":/icons/address-book"), tr("The Apostles Creed"), this);
    TheApostlesCreedAction->setStatusTip(tr("Show the Lords Prayer"));

    TheNiceneCreedAction = new QAction(QIcon(":/icons/address-book"), tr("The Nicene Creed"), this);
    TheNiceneCreedAction->setStatusTip(tr("Show the Nicene Creed"));

    TheTenCommandmentsAction = new QAction(QIcon(":/icons/address-book"), tr("The Ten Commandments"), this);
    TheTenCommandmentsAction->setStatusTip(tr("Show the Ten Commandments"));

    JesusConciseCommandmentsAction = new QAction(QIcon(":/icons/address-book"), tr("Jesus Concise Commandments"), this);
    JesusConciseCommandmentsAction->setStatusTip(tr("Show Jesus Concise Commandments"));

    ReadBibleAction = new QAction(QIcon(":/icons/address-book"), tr("Read Bible"), this);
    ReadBibleAction->setStatusTip(tr("Read Bible"));

    OneClickMiningAction = new QAction(QIcon(":/icons/editpaste"), tr("One Click Mining Configuration"), this);
    OneClickMiningAction->setStatusTip(tr("One Click Mining Configuration"));

	// Chat
    openChatGeneralAction = new QAction(QIcon(":/icons/chat"), tr("Chat Room - General"), this);
    openChatGeneralAction->setStatusTip(tr("Open General Chat Room"));
    openChatGeneralAction->setEnabled(false);

    openChatPMAction = new QAction(QIcon(":/icons/chat"), tr("Private Message"), this);
    openChatPMAction->setStatusTip(tr("Open Private Message Window"));
    openChatPMAction->setEnabled(false);

	openChatPMEncryptedAction = new QAction(QIcon(":/icons/chat"), tr("Private Message - Encrypted"), this);
    openChatPMEncryptedAction->setStatusTip(tr("Open Private Message (With Encryption) Window"));
    openChatPMEncryptedAction->setEnabled(false);

	// BBP-U
	univAction = new QAction(QIcon(":/icons/address-book"), tr("BiblePay University"), this);
    univAction->setStatusTip(tr("Show the BiblePay University"));
    univAction->setEnabled(false);

    // Add submenu for Proposal Add
    proposalAddMenuAction = new QAction(QIcon(":/icons/address-book"), tr("Proposal &Add"), this);
    proposalAddMenuAction->setStatusTip(tr("Add Proposal"));
    proposalAddMenuAction->setEnabled(false);

	nftAddMenuAction = new QAction(QIcon(":/icons/address-book"), tr("NFT &Add"), this);
    nftAddMenuAction->setStatusTip(tr("Add NFT"));
    nftAddMenuAction->setEnabled(false);

	utxoAddMenuAction = new QAction(QIcon(":/icons/address-book"), tr("Portfolio Builder"), this);
    utxoAddMenuAction->setStatusTip(tr("Add UTXO"));
    utxoAddMenuAction->setEnabled(false);
	
	listNFTMenuAction = new QAction(QIcon(":/icons/address-book"), tr("List My NFTs"), this);
    listNFTMenuAction->setStatusTip(tr("List My NFTs"));
    listNFTMenuAction->setEnabled(false);

	mailSendMenuAction = new QAction(QIcon(":/icons/address-book"), tr("Send Greeting Card"), this);
    mailSendMenuAction->setStatusTip(tr("Send Greeting Card"));
    mailSendMenuAction->setEnabled(false);

	// User Edit Action

    userEditMenuAction = new QAction(QIcon(":/icons/address-book"), tr("My User Record"), this);
    userEditMenuAction->setStatusTip(tr("My User Record"));
    userEditMenuAction->setEnabled(false);

	memorizeScriptureMenuAction = new QAction(QIcon(":/icons/address-book"), tr("Memorize Scriptures"), this);
    memorizeScriptureMenuAction->setStatusTip(tr("Memorize Scriptures"));
    memorizeScriptureMenuAction->setEnabled(false);

	// End of BBP

    aboutAction = new QAction(tr("&About %1").arg(tr(PACKAGE_NAME)), this);
    aboutAction->setStatusTip(tr("Show information about BiblePay Core"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(tr(PACKAGE_NAME)));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    toggleHideAction = new QAction(tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    backupWalletAction = new QAction(tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(tr("&Lock Wallet"), this);
    signMessageAction = new QAction(tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your BiblePay addresses to prove you own them"));
    verifyMessageAction = new QAction(tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified BiblePay addresses"));

    openInfoAction = new QAction(tr("&Information"), this);
    openInfoAction->setStatusTip(tr("Show diagnostic information"));
    openRPCConsoleAction = new QAction(tr("&Debug console"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging console"));
    openGraphAction = new QAction(tr("&Network Monitor"), this);
    openGraphAction->setStatusTip(tr("Show network monitor"));
    openPeersAction = new QAction(tr("&Peers list"), this);
    openPeersAction->setStatusTip(tr("Show peers info"));
    openRepairAction = new QAction(tr("Wallet &Repair"), this);
    openRepairAction->setStatusTip(tr("Show wallet repair options"));
    openConfEditorAction = new QAction(tr("Open Wallet &Configuration File"), this);
    openConfEditorAction->setStatusTip(tr("Open configuration file"));
    // override TextHeuristicRole set by default which confuses this action with application settings
    openConfEditorAction->setMenuRole(QAction::NoRole);
    showBackupsAction = new QAction(tr("Show Automatic &Backups"), this);
    showBackupsAction->setStatusTip(tr("Show automatically created wallet backups"));
    // initially disable the debug window menu items
    openInfoAction->setEnabled(false);
    openRPCConsoleAction->setEnabled(false);
    openGraphAction->setEnabled(false);
    openPeersAction->setEnabled(false);
    openRepairAction->setEnabled(false);

    usedSendingAddressesAction = new QAction(tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a biblepay: URI or payment request"));

    showHelpMessageAction = new QAction(tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible BiblePay command-line options").arg(tr(PACKAGE_NAME)));

    showPrivateSendHelpAction = new QAction(tr("&PrivateSend information"), this);
    showPrivateSendHelpAction->setMenuRole(QAction::NoRole);
    showPrivateSendHelpAction->setStatusTip(tr("Show the PrivateSend basic information"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
    connect(showPrivateSendHelpAction, SIGNAL(triggered()), this, SLOT(showPrivateSendHelpClicked()));

    // Jump directly to tabs in RPC-console
    connect(openInfoAction, SIGNAL(triggered()), this, SLOT(showInfo()));
    connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showConsole()));
    connect(openGraphAction, SIGNAL(triggered()), this, SLOT(showGraph()));
    connect(openPeersAction, SIGNAL(triggered()), this, SLOT(showPeers()));
    connect(openRepairAction, SIGNAL(triggered()), this, SLOT(showRepair()));

    // Open configs and backup folder from menu
    connect(openConfEditorAction, SIGNAL(triggered()), this, SLOT(showConfEditor()));
    connect(showBackupsAction, SIGNAL(triggered()), this, SLOT(showBackups()));

    // Get restart command-line parameters and handle restart
    connect(rpcConsole, SIGNAL(handleRestart(QStringList)), this, SLOT(handleRestart(QStringList)));

    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));
	// BBP
	// Read Bible Actions
	connect(sinnerAction, SIGNAL(triggered()), this, SLOT(sinnerClicked()));
    connect(TheLordsPrayerAction, SIGNAL(triggered()), this, SLOT(TheLordsPrayerClicked()));
    connect(TheApostlesCreedAction, SIGNAL(triggered()), this, SLOT(TheApostlesCreedClicked()));
    connect(TheNiceneCreedAction, SIGNAL(triggered()), this, SLOT(TheNiceneCreedClicked()));
    connect(ReadBibleAction, SIGNAL(triggered()), this, SLOT(ReadBibleClicked()));
    connect(OneClickMiningAction, SIGNAL(triggered()), this, SLOT(OneClickMiningClicked()));
    connect(TheTenCommandmentsAction, SIGNAL(triggered()), this, SLOT(TheTenCommandmentsClicked()));
    connect(JesusConciseCommandmentsAction, SIGNAL(triggered()), this, SLOT(JesusConciseCommandmentsClicked()));
	connect(proposalListAction, SIGNAL(triggered()), this, SLOT(gotoProposalListPage()));
	connect(proposalAddMenuAction, SIGNAL(triggered()), this, SLOT(gotoProposalAddPage()));
	connect(nftAddMenuAction, SIGNAL(triggered()), this, SLOT(gotoNFTAddPage()));
	connect(utxoAddMenuAction, SIGNAL(triggered()), this, SLOT(gotoUTXOAddPage()));
	connect(listNFTMenuAction, SIGNAL(triggered()), this, SLOT(gotoNFTListPage()));
	connect(mailSendMenuAction, SIGNAL(triggered()), this, SLOT(gotoMailSendPage()));
	connect(userEditMenuAction, SIGNAL(triggered()), this, SLOT(gotoUserEditPage()));
	connect(memorizeScriptureMenuAction, SIGNAL(triggered()), this, SLOT(gotoMemorizeScripturePage()));

	// CHAT
	connect(openChatGeneralAction, SIGNAL(triggered()), this, SLOT(openChatGeneralClicked()));
    connect(openChatPMAction, SIGNAL(triggered()), this, SLOT(openChatPMClicked()));
	connect(openChatPMEncryptedAction, SIGNAL(triggered()), this, SLOT(openChatPMEncryptedClicked()));

	// BBP Univ Actions
	connect(univAction, SIGNAL(triggered()), this, SLOT(univClicked()));
	// End of BBP

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered()), walletFrame, SLOT(encryptWallet()));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
        connect(unlockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(unlockWallet()));
        connect(lockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(lockWallet()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));
    }
#endif // ENABLE_WALLET

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I), this, SLOT(showInfo()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this, SLOT(showConsole()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G), this, SLOT(showGraph()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P), this, SLOT(showPeers()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R), this, SLOT(showRepair()));
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addSeparator();
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(lockWalletAction);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    if(walletFrame)
    {
        QMenu *tools = appMenuBar->addMenu(tr("&Tools"));
        tools->addAction(openInfoAction);
        tools->addAction(openRPCConsoleAction);
        tools->addAction(openGraphAction);
        tools->addAction(openPeersAction);
        tools->addAction(openRepairAction);
        tools->addSeparator();
        tools->addAction(openConfEditorAction);
        tools->addAction(showBackupsAction);
		tools->addAction(OneClickMiningAction);
    }

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(showHelpMessageAction);
    help->addAction(showPrivateSendHelpAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
	help->addSeparator();
	help->addAction(sendCoinsMenuAction);
	help->addAction(privateSendCoinsMenuAction);
 	help->addAction(receiveCoinsMenuAction);


	// BBP - Prayers, Jesus' Commandments, and Reading the Bible
	if (walletFrame)
	{
		QMenu *menuBible = appMenuBar->addMenu(tr("&Bible"));
		menuBible->addAction(sinnerAction);
		menuBible->addAction(TheLordsPrayerAction);
		menuBible->addAction(TheApostlesCreedAction);
		menuBible->addAction(TheNiceneCreedAction);
		menuBible->addAction(TheTenCommandmentsAction);
		menuBible->addAction(JesusConciseCommandmentsAction);
		menuBible->addAction(ReadBibleAction);

		QMenu *proposals = appMenuBar->addMenu(tr("&Proposals"));
		//proposals->addAction(proposalListAction);
		proposals->addAction(proposalAddMenuAction);

		QMenu *nfts = appMenuBar->addMenu(tr("&NFTs"));
		nfts->addAction(nftAddMenuAction);
		nfts->addAction(listNFTMenuAction);

		QMenu *misc = appMenuBar->addMenu(tr("Misc."));
		misc->addAction(mailSendMenuAction);
		misc->addAction(utxoAddMenuAction);

		QMenu *useredit = appMenuBar->addMenu(tr("User Record"));
		useredit->addAction(userEditMenuAction);
		useredit->addAction(userEditMenuAction);

		QMenu *qmmemorizescriptures = appMenuBar->addMenu(tr("Memorize Scriptures"));
		qmmemorizescriptures->addAction(memorizeScriptureMenuAction);
		qmmemorizescriptures->addAction(memorizeScriptureMenuAction);

        QMenu *menuChat = appMenuBar->addMenu(tr("&Chat"));
        menuChat->addAction(openChatGeneralAction);
        menuChat->addAction(openChatPMAction);
		menuChat->addAction(openChatPMEncryptedAction);

		QMenu *menuUniv = appMenuBar->addMenu(tr("&BBP University"));
		menuUniv->addAction(univAction);

		QMenu *help = appMenuBar->addMenu(tr("&Help"));
		help->addAction(showHelpMessageAction);
		help->addAction(showPrivateSendHelpAction);
		help->addSeparator();
		help->addAction(aboutAction);
		help->addAction(aboutQtAction);
	}
	// End of BBP
}

void BitcoinGUI::createToolBars()
{
#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        QToolBar *toolbar = new QToolBar(tr("Tabs toolbar"));

		toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        overviewAction->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sendCoinsAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        privateSendCoinsAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        receiveCoinsAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        historyAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		proposalListAction->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
 		orphanAction->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
		leaderboardAction->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

 		exchangeAction->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
 		webAction->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

        toolbar->addWidget(overviewAction);
        toolbar->addWidget(sendCoinsAction);
		if (1==0)
		{
			toolbar->addWidget(privateSendCoinsAction);
		}
        toolbar->addWidget(receiveCoinsAction);
        toolbar->addWidget(historyAction);
		toolbar->addWidget(proposalListAction);
		toolbar->addWidget(orphanAction);
		toolbar->addWidget(leaderboardAction);
        QSettings settings;
        if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
            masternodeAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            toolbar->addWidget(masternodeAction);
        }
		toolbar->addWidget(exchangeAction);
		toolbar->addWidget(webAction);


        toolbar->setMovable(false); // remove unused icon in upper left corner
        overviewAction->setChecked(true);

		// Maybe we can add our logo to the bottom left of the toolbar if we need to fill the space:
		// QLabel *logoLabel = new QLabel();
        // logoLabel->setObjectName("lblToolbarLogo");
        // logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	    // toolbar->addWidget(logoLabel);

		toolbar->setOrientation(Qt::Vertical);
		addToolBar(Qt::LeftToolBarArea, toolbar);

        /** Create additional container for toolbar and walletFrame and make it the central widget.
            This is a workaround mostly for toolbar styling on Mac OS but should work fine for every other OSes too.
        */
		// toolbar->setSizePolicy(QSizePolicy::Expanding,  QSizePolicy::Expanding);
		// walletFrame->setSizePolicy(QSizePolicy::Expanding,  QSizePolicy::Expanding);
        QHBoxLayout *layout = new QHBoxLayout;
		layout->setContentsMargins(0,0,0,0);
	    layout->setSpacing(0);
        layout->addWidget(toolbar);
	    layout->setAlignment(toolbar, Qt::AlignTop);

        layout->addWidget(walletFrame);
		layout->setAlignment(walletFrame, Qt::AlignTop);
		layout->addStretch(0);

        QWidget *containerWidget = new QWidget();
        containerWidget->setLayout(layout);
        setCentralWidget(containerWidget);
    }
#endif // ENABLE_WALLET
}

void BitcoinGUI::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
    if(_clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        if (trayIcon) {
            // do so only if trayIcon is already set
            trayIconMenu = new QMenu(this);
            trayIcon->setContextMenu(trayIconMenu);
            createIconMenu(trayIconMenu);

#ifndef Q_OS_MAC
            // Show main window on tray icon click
            // Note: ignore this on Mac - this is not the way tray should work there
            connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                    this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
            // Note: On Mac, the dock icon is also used to provide menu functionality
            // similar to one for tray icon
            MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
            connect(dockIconHandler, SIGNAL(dockIconClicked()), this, SLOT(macosDockIconActivated()));

            dockIconMenu = new QMenu(this);
            dockIconMenu->setAsDockMenu();

            createIconMenu(dockIconMenu);
#endif
        }

        // Keep up to date with client
        updateNetworkState();
        setNumConnections(_clientModel->getNumConnections());
        connect(_clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));
        connect(_clientModel, SIGNAL(networkActiveChanged(bool)), this, SLOT(setNetworkActive(bool)));

        modalOverlay->setKnownBestHeight(_clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(_clientModel->getHeaderTipTime()));
        setNumBlocks(_clientModel->getNumBlocks(), _clientModel->getLastBlockDate(), _clientModel->getLastBlockHash(), _clientModel->getVerificationProgress(nullptr), false);
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,QString,double,bool)), this, SLOT(setNumBlocks(int,QDateTime,QString,double,bool)));

        connect(_clientModel, SIGNAL(additionalDataSyncProgressChanged(double)), this, SLOT(setAdditionalDataSyncProgress(double)));

        // Receive and report messages from client model
        connect(_clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(_clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        rpcConsole->setClientModel(_clientModel);
#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(_clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(_clientModel->getOptionsModel());

        OptionsModel* optionsModel = _clientModel->getOptionsModel();
        if(optionsModel)
        {
            // be aware of the tray icon disable state change reported by the OptionsModel object.
            connect(optionsModel,SIGNAL(hideTrayIconChanged(bool)),this,SLOT(setTrayIconVisible(bool)));

            // initialize the disable state of the tray icon with the current value in the model.
            setTrayIconVisible(optionsModel->getHideTrayIcon());

            connect(optionsModel, SIGNAL(privateSendEnabledChanged()), this, SLOT(updatePrivateSendVisibility()));
        }
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
        // Propagate cleared model to child objects
        rpcConsole->setClientModel(nullptr);
#ifdef ENABLE_WALLET
        if (walletFrame)
        {
            walletFrame->setClientModel(nullptr);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(nullptr);

#ifdef Q_OS_MAC
        if(dockIconMenu)
        {
            // Disable context menu on dock icon
            dockIconMenu->clear();
        }
#endif
    }

    updatePrivateSendVisibility();
}

#ifdef ENABLE_WALLET
bool BitcoinGUI::addWallet(const QString& name, WalletModel *walletModel)
{
    if(!walletFrame)
        return false;
    setWalletActionsEnabled(true);
    return walletFrame->addWallet(name, walletModel);
}

bool BitcoinGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

void BitcoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void BitcoinGUI::setWalletActionsEnabled(bool enabled)
{

	std::cout << "\nUI1.." << std::endl;

    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
#ifdef ENABLE_WALLET
		privateSendCoinsAction->setEnabled(enabled && privateSendClient.fEnablePrivateSend);
		privateSendCoinsMenuAction->setEnabled(enabled && privateSendClient.fEnablePrivateSend);
#else
		privateSendCoinsAction->setEnabled(enabled);
		privateSendCoinsMenuAction->setEnabled(enabled);
#endif // ENABLE_WALLET
    privateSendCoinsAction->setEnabled(false);
	privateSendCoinsMenuAction->setEnabled(false);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
        masternodeAction->setEnabled(enabled);
    }
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
	//BBP
	sinnerAction->setEnabled(enabled);
    TheLordsPrayerAction->setEnabled(enabled);
    TheApostlesCreedAction->setEnabled(enabled);
    TheNiceneCreedAction->setEnabled(enabled);
    ReadBibleAction->setEnabled(enabled);
    TheTenCommandmentsAction->setEnabled(enabled);
    JesusConciseCommandmentsAction->setEnabled(enabled);

	proposalAddMenuAction->setEnabled(enabled);
	nftAddMenuAction->setEnabled(enabled);
	utxoAddMenuAction->setEnabled(enabled);
	listNFTMenuAction->setEnabled(enabled);
	mailSendMenuAction->setEnabled(enabled);

	userEditMenuAction->setEnabled(enabled);
	memorizeScriptureMenuAction->setEnabled(enabled);
	univAction->setEnabled(enabled);
	OneClickMiningAction->setEnabled(enabled);
	leaderboardAction->setEnabled(enabled);
	proposalListAction->setEnabled(enabled);
	orphanAction->setEnabled(enabled);

	//End of BBP
}

void BitcoinGUI::createTrayIcon(const NetworkStyle *networkStyle)
{
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("%1 client").arg(tr(PACKAGE_NAME)) + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getTrayAndWindowIcon());
    trayIcon->hide();
    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void BitcoinGUI::createIconMenu(QMenu *pmenu)
{
    // Configuration of the tray icon (or dock icon) icon menu
    pmenu->addAction(toggleHideAction);
    pmenu->addSeparator();
    pmenu->addAction(sendCoinsMenuAction);
    pmenu->addAction(privateSendCoinsMenuAction);
    pmenu->addAction(receiveCoinsMenuAction);
    pmenu->addSeparator();
    pmenu->addAction(signMessageAction);
    pmenu->addAction(verifyMessageAction);
    pmenu->addSeparator();
    pmenu->addAction(optionsAction);
    pmenu->addAction(openInfoAction);
    pmenu->addAction(openRPCConsoleAction);
    pmenu->addAction(openGraphAction);
    pmenu->addAction(openPeersAction);
    pmenu->addAction(openRepairAction);
    pmenu->addSeparator();
    pmenu->addAction(openConfEditorAction);
    pmenu->addAction(showBackupsAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    pmenu->addSeparator();
    pmenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#else
void BitcoinGUI::macosDockIconActivated()
{
    showNormalIfMinimized();
    activateWindow();
}
#endif

void BitcoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setModel(clientModel->getOptionsModel());
    connect(&dlg, &OptionsDialog::appearanceChanged, [=]() {
        updateWidth();
    });
    dlg.exec();

    updatePrivateSendVisibility();
}

void BitcoinGUI::univClicked()
{
    if(!clientModel)
		return;
    MenuDialog dlg(this);
    dlg.exec();
}

void BitcoinGUI::sinnerClicked()
{
    if(!clientModel) return;
    HelpMessageDialog dlg(this, HelpMessageDialog::prayer, 0, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::TheLordsPrayerClicked()
{
    if(!clientModel) return;
    HelpMessageDialog dlg(this, HelpMessageDialog::prayer, 1, uint256S("0x0"), "");
    dlg.exec();
}

void CenterWidget(QWidget *widget, bool bTile)
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - widget->width()) / 2;
    int y = (screenGeometry.height() - widget->height()) / 2;
    if (bTile)
    {
            double iX = (rand() % 190) / 100;
            double iY = (rand() % 190) / 100;
            x = iX * x;
            y = iY * y;
    }
    widget->move(x, y);
}

void BitcoinGUI::openChatGeneralClicked()
{
    if(!clientModel)
		return;
	UserRecord myUser = GetMyUserRecord();

    ChatDialog *dlg = new ChatDialog(this, false, false, myUser.NickName, "General");
    dlg->setClientModel(this->clientModel);
    dlg->show();
    CenterWidget(dlg, true);
}

void BitcoinGUI::openChatPMClicked()
{
    if(!clientModel) return;
	UserRecord myUser = GetMyUserRecord();

    ChatDialog *dlg = new ChatDialog(this, true, false, myUser.NickName, "");
    dlg->setClientModel(this->clientModel);
    dlg->show();
    CenterWidget(dlg, true);
}

void BitcoinGUI::openChatPMEncryptedClicked()
{
   if(!clientModel)
	   return;
   UserRecord myRec = GetMyUserRecord();
   ChatDialog *dlg = new ChatDialog(this, true, true, myRec.NickName, "");
   dlg->setClientModel(this->clientModel);
   dlg->show();
   CenterWidget(dlg, true);
}

void BitcoinGUI::TheApostlesCreedClicked()
{
    if(!clientModel)
		return;
    HelpMessageDialog dlg(this, HelpMessageDialog::prayer, 2, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::TheNiceneCreedClicked()
{
    if(!clientModel) return;
    HelpMessageDialog dlg(this, HelpMessageDialog::prayer, 3, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::ReadBibleClicked()
{
    if(!clientModel) return;
    HelpMessageDialog dlg(this, HelpMessageDialog::readbible, 0, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::TheTenCommandmentsClicked()
{
    if(!clientModel) return;
    HelpMessageDialog dlg(this, HelpMessageDialog::prayer, 4, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::JesusConciseCommandmentsClicked()
{
    if(!clientModel) return;
    HelpMessageDialog dlg(this, HelpMessageDialog::prayer, 5, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::OneClickMiningClicked()
{
   if(!clientModel) return;
    std::string sNarr = "Are you sure you would like to configure Mining (this means the wallet will search for new blocks in the background, and will resume during each restart)?";
    int ret = QMessageBox::warning(this, tr("Modify the Configuration File for Mining?"), GUIUtil::TOQS(sNarr), QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
    if (ret==QMessageBox::Ok)
    {
        // Modify user configuration file
        bool fSuccess = InstantiateOneClickMiningEntries();
        sNarr = fSuccess ? "Configuration Succeeded" : "Configuration Failed.";
        QMessageBox::warning(this, GUIUtil::TOQS(sNarr), GUIUtil::TOQS(sNarr), QMessageBox::Ok, QMessageBox::Ok);
    }
}

void BitcoinGUI::showAccountability()
{
	std::string sURL = "https://foundation.biblepay.org/Accountability";
	QDesktopServices::openUrl(QUrl(GUIUtil::TOQS(sURL)));
}

void BitcoinGUI::showExchange()
{
	std::string sURL = "https://www.southxchange.com/Market/Book/BBP/BTC";
	QDesktopServices::openUrl(QUrl(GUIUtil::TOQS(sURL)));
}


void BitcoinGUI::gotoUserEditPage()
{
	if (!clientModel)
		return;
    userEditMenuAction->setChecked(true);
    if (walletFrame)
		walletFrame->gotoUserEditPage();
}

void BitcoinGUI::gotoMemorizeScripturePage()
{
	if (!clientModel)
		return;
	memorizeScriptureMenuAction->setChecked(true);
	if (walletFrame)
		walletFrame->gotoMemorizeScripturePage();
}

void BitcoinGUI::gotoProposalAddPage()
{
	if (!clientModel)
		return;
    proposalAddMenuAction->setChecked(true);
    if (walletFrame) walletFrame->gotoProposalAddPage();
}

void BitcoinGUI::gotoNFTAddPage()
{
	if (!clientModel)
		return;
    nftAddMenuAction->setChecked(true);
    if (walletFrame)
		walletFrame->gotoNFTAddPage("CREATE", uint256S("0x0"));
}

void BitcoinGUI::gotoUTXOAddPage()
{
	if (!clientModel)
		return;
    utxoAddMenuAction->setChecked(true);
    if (walletFrame)
		walletFrame->gotoUTXOAddPage();
}

void BitcoinGUI::gotoMailSendPage()
{
	if (!clientModel)
		return;
	mailSendMenuAction->setChecked(true);
    if (walletFrame)
		walletFrame->gotoMailSendPage("Christmas");
}

void BitcoinGUI::gotoNFTListPage()
{
	if (!clientModel)
		return;
	listNFTMenuAction->setChecked(true);
	
	if (walletFrame)
		walletFrame->gotoNFTListPage();
}

void BitcoinGUI::gotoProposalListPage()
{
	if (!clientModel) return;
    proposalListAction->setChecked(true);
    if (walletFrame) walletFrame->gotoProposalListPage();
}

void BitcoinGUI::gotoBusinessObjectListPage()
{
	//if (!clientModel) return;
    leaderboardAction->setChecked(true);
    if (walletFrame)
		walletFrame->gotoBusinessObjectListPage();
}

void BitcoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(this, HelpMessageDialog::about, 0, uint256S("0x0"), "");
    dlg.exec();
}

void BitcoinGUI::showDebugWindow()
{
    GUIUtil::bringToFront(rpcConsole);
}

void BitcoinGUI::showInfo()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_INFO);
    showDebugWindow();
}

void BitcoinGUI::showConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_CONSOLE);
    showDebugWindow();
	// BBP:
	rpcConsole->setWindowModality(Qt::NonModal);
}

void BitcoinGUI::showGraph()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_GRAPH);
    showDebugWindow();
}

void BitcoinGUI::showPeers()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_PEERS);
    showDebugWindow();
}

void BitcoinGUI::showRepair()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_REPAIR);
    showDebugWindow();
}

void BitcoinGUI::showConfEditor()
{
    GUIUtil::openConfigfile();
}

void BitcoinGUI::showBackups()
{
    GUIUtil::showBackups();
}

void BitcoinGUI::showHelpMessageClicked()
{
    helpMessageDialog->show();
}

void BitcoinGUI::showPrivateSendHelpClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(this, HelpMessageDialog::pshelp, 0, uint256S("0x0"), "");
    dlg.exec();
}

#ifdef ENABLE_WALLET
void BitcoinGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void BitcoinGUI::highlightTabButton(QAbstractButton *button, bool checked)
{
    GUIUtil::setFont({button}, checked ? GUIUtil::FontWeight::Bold : GUIUtil::FontWeight::Normal, 16);
    GUIUtil::updateFonts();
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void BitcoinGUI::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
        masternodeAction->setChecked(true);
        if (walletFrame) walletFrame->gotoMasternodePage();
    }
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void BitcoinGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void BitcoinGUI::gotoPrivateSendCoinsPage(QString addr)
{
    privateSendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoPrivateSendCoinsPage(addr);
}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif // ENABLE_WALLET

void BitcoinGUI::updateNetworkState()
{
    static int nCountPrev{0};
    static bool fNetworkActivePrev{false};
    int count = clientModel->getNumConnections();
    bool fNetworkActive = clientModel->getNetworkActive();
    QString icon;
    GUIUtil::ThemedColor color = GUIUtil::ThemedColor::ORANGE;
    switch(count)
    {
    case 0: icon = "connect_4"; color = GUIUtil::ThemedColor::ICON_ALTERNATIVE_COLOR; break;
    case 1: case 2: icon = "connect_1"; break;
    case 3: case 4: case 5: icon = "connect_2"; break;
    case 6: case 7: icon = "connect_3"; break;
    default: icon = "connect_4"; color = GUIUtil::ThemedColor::GREEN; break;
    }

    labelBlocksIcon->setVisible(count > 0);
    updateProgressBarVisibility();

    bool fNetworkBecameActive = (!fNetworkActivePrev && fNetworkActive) || (nCountPrev == 0 && count > 0);
    bool fNetworkBecameInactive = (fNetworkActivePrev && !fNetworkActive) || (nCountPrev > 0 && count == 0);

    if (fNetworkBecameActive) {
        // If the sync process still signals synced after five seconds represent it in the UI.
        if (masternodeSync.IsSynced()) {
            QTimer::singleShot(5000, this, [&]() {
                if (clientModel->getNumConnections() > 0 && masternodeSync.IsSynced()) {
                    setAdditionalDataSyncProgress(1);
                }
            });
        }
        startSpinner();
    } else if (fNetworkBecameInactive) {
        labelBlocksIcon->hide();
        stopSpinner();
    }

    if (fNetworkBecameActive || fNetworkBecameInactive) {
        setNumBlocks(clientModel->getNumBlocks(), clientModel->getLastBlockDate(), clientModel->getLastBlockHash(), clientModel->getVerificationProgress(nullptr), false);
    }

    nCountPrev = count;
    fNetworkActivePrev = fNetworkActive;

    if (fNetworkActive) {
        labelConnectionsIcon->setToolTip(tr("%n active connection(s) to BiblePay network", "", count));
    } else {
        labelConnectionsIcon->setToolTip(tr("Network activity disabled"));
        icon = "connect_4";
        color = GUIUtil::ThemedColor::RED;
    }

    if (fNetworkActive && count == 0) {
        startConnectingAnimation();
    }
    if (!fNetworkActive || count > 0) {
        stopConnectingAnimation();
        labelConnectionsIcon->setPixmap(GUIUtil::getIcon(icon, color).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
}

void BitcoinGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void BitcoinGUI::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void BitcoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void BitcoinGUI::updateProgressBarVisibility()
{
    if (clientModel == nullptr) {
        return;
    }
    // Show the progress bar label if the network is active + we are out of sync or we have no connections.
    bool fShowProgressBarLabel = clientModel->getNetworkActive() && (!masternodeSync.IsSynced() || clientModel->getNumConnections() == 0);
    // Show the progress bar only if the the network active + we are not synced + we have any connection. Unlike with the label
    // which gives an info text about the connecting phase there is no reason to show the progress bar if we don't have connections
    // since it will not get any updates in this case.
    bool fShowProgressBar = clientModel->getNetworkActive() && !masternodeSync.IsSynced() && clientModel->getNumConnections() > 0;
    progressBarLabel->setVisible(fShowProgressBarLabel);
    progressBar->setVisible(fShowProgressBar);
}

void BitcoinGUI::updatePrivateSendVisibility()
{
#ifdef ENABLE_WALLET
    bool fEnabled = privateSendClient.fEnablePrivateSend;
#else
    bool fEnabled = false;
#endif
	fEnabled = false;
    // PrivateSend button is the third QToolButton, show/hide the underlying QAction
    // Hiding the QToolButton itself doesn't work.
    if (centralWidget()->layout()->itemAt(0)->widget() != nullptr) {
        qobject_cast<QToolBar*>(centralWidget()->layout()->itemAt(0)->widget())->actions()[2]->setVisible(fEnabled);
    }
    privateSendCoinsMenuAction->setVisible(fEnabled);
    showPrivateSendHelpAction->setVisible(fEnabled);

    updateToolBarShortcuts();
    updateWidth();
}

void BitcoinGUI::updateWidth()
{
    if (walletFrame == nullptr) {
        return;
    }
    if (windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
        return;
    }
    int nWidthWidestButton{0};
    int nButtonsVisible{0};
    for (QAbstractButton* button : tabGroup->buttons()) {
        if (!button->isEnabled()) {
            continue;
        }
        QFontMetrics fm(button->font());
        nWidthWidestButton = std::max<int>(nWidthWidestButton, fm.width(button->text()));
        ++nButtonsVisible;
    }
    // Add 30 per button as padding and use minimum 980 which is the minimum required to show all tab's contents
    // Use nButtonsVisible + 1 <- for the biblepay logo
    int nWidth = std::max<int>(980, (nWidthWidestButton + 30) * (nButtonsVisible + 1));
	// This is a nagging and frustrating issue!  I found if the user sets a montserrat font size that is bigger than medium, the minimum width of the wallet is huge.  We have to figure out how to deal with this.
	if (nWidth > 1200)
		nWidth = 1200;
    setMinimumWidth(nWidth);
	int nHeight = height();
	if (nHeight > 800)
		nHeight = 800;
	LogPrintf("\nBitcoinGui::UpdateWidth and height %f x %f", nWidth, nHeight);

    resize(nWidth, nHeight);
}

void BitcoinGUI::updateToolBarShortcuts()
{
    if (walletFrame == nullptr) {
        return;
    }
#ifdef Q_OS_MAC
    auto modifier = Qt::CTRL;
#else
    auto modifier = Qt::ALT;
#endif
    int nKey = 0;
    for (int i = 0; i < tabGroup->buttons().size(); ++i) {
        if (qobject_cast<QToolBar*>(centralWidget()->layout()->itemAt(0)->widget())->actions()[i]->isVisible()) {
            tabGroup->buttons()[i]->setShortcut(QKeySequence(modifier + Qt::Key_1 + nKey++));
        } else {
            tabGroup->buttons()[i]->setShortcut(QKeySequence());
        }
    }
}

void BitcoinGUI::setNumBlocks(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool header)
{
#ifdef Q_OS_MAC
    // Disabling macOS App Nap on initial sync, disk, reindex operations and mixing.
    bool disableAppNap = !masternodeSync.IsSynced();
#ifdef ENABLE_WALLET
    disableAppNap |= privateSendClient.fPrivateSendRunning;
#endif // ENABLE_WALLET
    if (disableAppNap) {
        m_app_nap_inhibitor->disableAppNap();
    } else {
        m_app_nap_inhibitor->enableAppNap();
    }
#endif // Q_OS_MAC

    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }
    if (!clientModel)
        return;

    updateProgressBarVisibility();

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network..."));
            updateHeadersSyncProgressLabel();
            break;
        case BLOCK_SOURCE_DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk..."));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk..."));
            }
            break;
        case BLOCK_SOURCE_REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting to peers..."));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
#ifdef ENABLE_WALLET
    if (walletFrame)
    {
        if(secs < 25*60) // 90*60 in bitcoin
        {
            modalOverlay->showHide(true, true);
            // TODO instead of hiding it forever, we should add meaningful information about MN sync to the overlay
            modalOverlay->hideForever();
        }
        else
        {
            modalOverlay->showHide();
        }
    }
#endif // ENABLE_WALLET

    if(!masternodeSync.IsBlockchainSynced())
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(true);
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    } else if (fDisableGovernance) {
        setAdditionalDataSyncProgress(1);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::setAdditionalDataSyncProgress(double nSyncProgress)
{
    if(!clientModel)
        return;

    // If masternodeSync.Reset() has been called make sure status bar shows the correct information.
    if (nSyncProgress == -1) {
        setNumBlocks(clientModel->getNumBlocks(), clientModel->getLastBlockDate(), clientModel->getLastBlockHash(), clientModel->getVerificationProgress(nullptr), false);
        if (clientModel->getNumConnections()) {
            labelBlocksIcon->show();
            startSpinner();
        }
        return;
    }

    // No additional data sync should be happening while blockchain is not synced, nothing to update
    if(!masternodeSync.IsBlockchainSynced())
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    QString tooltip;

    // Set icon state: spinning if catching up, tick otherwise
    QString strSyncStatus;
    tooltip = tr("Up to date") + QString(".<br>") + tooltip;

#ifdef ENABLE_WALLET
    if(walletFrame)
        walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

    updateProgressBarVisibility();

    if(masternodeSync.IsSynced()) {
        stopSpinner();
        labelBlocksIcon->setPixmap(GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    } else {
        progressBar->setFormat(tr("Synchronizing additional data: %p%"));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nSyncProgress * 1000000000.0 + 0.5);
    }

    strSyncStatus = QString(masternodeSync.GetSyncStatus().c_str());
    progressBarLabel->setText(strSyncStatus);
    tooltip = strSyncStatus + QString("<br>") + tooltip;

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("BiblePay Core"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;

    // Prefer supplied title over style based title
    if (!title.isEmpty()) {
        msgType = title;
    }
    else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            break;
        default:
            break;
        }
    }
    // Append title to "BiblePay Core - "
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons, this);
        mBox.setTextFormat(Qt::PlainText);
        int r = mBox.exec();
        if (ret != nullptr)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
            else if((wsevt->oldState() & Qt::WindowMinimized) && !isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(show()));
                e->ignore();
            }
        }
    }
#endif
    if (e->type() == QEvent::StyleChange) {
        updateNetworkState();
#ifdef ENABLE_WALLET
        updateWalletStatus();
#endif
        if (masternodeSync.IsSynced()) {
            labelBlocksIcon->setPixmap(GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        }
    }
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            QApplication::quit();
        }
        else
        {
            QMainWindow::showMinimized();
            event->ignore();
        }
    }
#else
    QMainWindow::closeEvent(event);
#endif
}

void BitcoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openInfoAction->setEnabled(true);
    openRPCConsoleAction->setEnabled(true);
    openGraphAction->setEnabled(true);
    openPeersAction->setEnabled(true);
    openRepairAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
	openChatGeneralAction->setEnabled(true);
    openChatPMAction->setEnabled(true);
	openChatPMEncryptedAction->setEnabled(true);
    if (!event->spontaneous()) {
        updateWidth();
    }
}

#ifdef ENABLE_WALLET
void BitcoinGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label)
{
    IncomingTransactionMessage itx = {
            date, unit, amount, type, address, label
    };
    incomingTransactions.emplace_back(itx);

    if (incomingTransactions.size() == 1) {
        // first TX since we last showed pending messages, let's wait 100ms and then show each individual message
        incomingTransactionsTimer->start(100);
    } else if (incomingTransactions.size() == 10) {
        // we seem to have received 10 TXs in 100ms and we can expect even more, so let's pause for 1 sec and
        // show a "Multiple TXs sent/received!" message instead of individual messages
        incomingTransactionsTimer->start(1000);
    }
}
void BitcoinGUI::showIncomingTransactions()
{
    auto txs = std::move(this->incomingTransactions);

    if (txs.empty()) {
        return;
    }

    if (txs.size() >= 100) {
        // Show one balloon for all transactions instead of showing one for each individual one
        // (which would kill some systems)

        CAmount sentAmount = 0;
        CAmount receivedAmount = 0;
        int sentCount = 0;
        int receivedCount = 0;
        for (auto& itx : txs) {
            if (itx.amount < 0) {
                sentAmount += itx.amount;
                sentCount++;
            } else {
                receivedAmount += itx.amount;
                receivedCount++;
            }
        }

        QString title;
        if (sentCount > 0 && receivedCount > 0) {
            title = tr("Received and sent multiple transactions");
        } else if (sentCount > 0) {
            title = tr("Sent multiple transactions");
        } else if (receivedCount > 0) {
            title = tr("Received multiple transactions");
        } else {
            return;
        }

        // Use display unit of last entry
        int unit = txs.back().unit;

        QString msg;
        if (sentCount > 0) {
            msg += tr("Sent Amount: %1\n").arg(BitcoinUnits::formatWithUnit(unit, sentAmount, true));
        }
        if (receivedCount > 0) {
            msg += tr("Received Amount: %1\n").arg(BitcoinUnits::formatWithUnit(unit, receivedAmount, true));
        }

        message(title, msg, CClientUIInterface::MSG_INFORMATION);
    } else {
        for (auto& itx : txs) {
            // On new transaction, make an info balloon
            QString msg = tr("Date: %1\n").arg(itx.date) +
                          tr("Amount: %1\n").arg(BitcoinUnits::formatWithUnit(itx.unit, itx.amount, true)) +
                          tr("Type: %1\n").arg(itx.type);
            if (!itx.label.isEmpty())
                msg += tr("Label: %1\n").arg(itx.label);
            else if (!itx.address.isEmpty())
                msg += tr("Address: %1\n").arg(itx.address);
            message((itx.amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
                    msg, CClientUIInterface::MSG_INFORMATION);
        }
    }
}
#endif // ENABLE_WALLET

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        for (const QUrl &uri : event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool BitcoinGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

#ifdef ENABLE_WALLET
bool BitcoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void BitcoinGUI::setHDStatus(int hdEnabled)
{
    if (hdEnabled) {
        labelWalletHDStatusIcon->setPixmap(GUIUtil::getIcon("hd_enabled", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletHDStatusIcon->setToolTip(tr("HD key generation is <b>enabled</b>"));
    }
    labelWalletHDStatusIcon->setVisible(hdEnabled);
}

void BitcoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_open", GUIUtil::ThemedColor::RED).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>unencrypted</b>"));
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_open", GUIUtil::ThemedColor::RED).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::UnlockedForMixingOnly:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_open", GUIUtil::ThemedColor::ORANGE).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for mixing only"));
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_closed", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void BitcoinGUI::updateWalletStatus()
{
    if (!walletFrame) {
        return;
    }
    WalletView * const walletView = walletFrame->currentWalletView();
    if (!walletView) {
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();
    setEncryptionStatus(walletModel->getEncryptionStatus());
    setHDStatus(walletModel->hdEnabled());
}
#endif // ENABLE_WALLET

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    if (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this) && fToggleHidden) {
        hide();
    } else {
        GUIUtil::bringToFront(this);
    }
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

static int nProposalModulus = 0;
void BitcoinGUI::detectShutdown()
{
    if (ShutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }

	// Governance - Check to see if we should submit a proposal
    nProposalModulus++;
    if (nProposalModulus % 10 == 0 && fWarmBootFinished)
    {
        nProposalModulus = 0;
		if (!msQuestion.empty())
		{
			// Display message box
			SendConfirmationDialog confirmationDialog(tr("Please Confirm"), GUIUtil::TOQS(msQuestion), 3, this);
			msQuestion = "";
			confirmationDialog.exec();
			QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();
			msQuestionAnswer = (retval == QMessageBox::Yes) ? "1" : "0";
	    }

		// Chat - If someone is paging us...

        if (mlPaged > 1 && !msPagedFrom.empty() && clientModel)
        {
			UserRecord myRec = GetMyUserRecord();

            mlPaged = 0;
            ChatDialog *dlg = new ChatDialog(this, true, false, myRec.NickName, msPagedFrom);
            dlg->setClientModel(this->clientModel);
            dlg->show();
            msPagedFrom = "";
        }

		if (mlPagedEncrypted > 1 && !msPagedFrom.empty() && clientModel)
        {
			UserRecord myRec = GetMyUserRecord();

            mlPagedEncrypted = 0;
            ChatDialog *dlg = new ChatDialog(this, true, true, myRec.NickName, msPagedFrom);
            dlg->setClientModel(this->clientModel);
            dlg->show();
            msPagedFrom = "";
        }

		// Opening a web resource
		if (!msURL.empty())
		{
			QString qNav = GUIUtil::TOQS(msURL);
			msURL = std::string();
			QDesktopServices::openUrl(QUrl(qNav));
		}
		// Proposals
		for (int i = 0; i < mvQueuedProposals.size(); i++)
		{
			QueuedProposal q = mvQueuedProposals[i];
			if (q.Submitted == false && !q.Hex.empty())
			{
				int nGFC = 3;  // ToDO: Import GOVERNANCE_FEE_CONFIRMATIONS
		        if(masternodeSync.IsSynced() && chainActive.Tip() && chainActive.Tip()->nHeight > (q.PrepareHeight + nGFC + q.SubmissionCount))
			    {
					std::string sError;
					std::string sGovObj;
					bool fSubmitted = SubmitProposalToNetwork(q.TXID, q.StartTime, q.Hex, sError, sGovObj);
					q.SubmissionCount++;
					if (q.SubmissionCount > 4)
						q.Submitted = true;
					if (!sGovObj.empty())
						q.Submitted = true;
					q.Error = sError;
					q.GovObj = uint256S(sGovObj);
					mvQueuedProposals[i] = q;
					LogPrintf("\nProposal Submission %s = %s", q.TXID.GetHex(), q.Error);
				}
            }
		}
    }
}

void BitcoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100, this);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

void BitcoinGUI::setTrayIconVisible(bool fHideTrayIcon)
{
    if (trayIcon)
    {
        trayIcon->setVisible(!fHideTrayIcon);
    }
}

void BitcoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(BitcoinGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    return ret;
}

void BitcoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    uiInterface.ThreadSafeQuestion.connect(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
}

void BitcoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    uiInterface.ThreadSafeQuestion.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
}

void BitcoinGUI::toggleNetworkActive()
{
    if (clientModel) {
        clientModel->setNetworkActive(!clientModel->getNetworkActive());
    }
}

/** Get restart command-line parameters and request restart */
void BitcoinGUI::handleRestart(QStringList args)
{
    if (!ShutdownRequested())
        Q_EMIT requestedRestart(args);
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl() :
    optionsModel(0),
    menu(0)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<BitcoinUnits::Unit> units = BitcoinUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(GUIUtil::getFontNormal());
    for (const BitcoinUnits::Unit unit : units)
    {
        max_width = qMax(max_width, fm.width(BitcoinUnits::name(unit)));
    }
    setMinimumSize(max_width, 0);
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	//	setStyleSheet(QString("QLabel { color : gold }"));

}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *event)
{
    onDisplayUnitsClicked(event->pos());
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu(this);
    for (BitcoinUnits::Unit u : BitcoinUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(BitcoinUnits::name(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }
    connect(menu,SIGNAL(triggered(QAction*)),this,SLOT(onMenuSelection(QAction*)));
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *_optionsModel)
{
    if (_optionsModel)
    {
        this->optionsModel = _optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(_optionsModel,SIGNAL(displayUnitChanged(int)),this,SLOT(updateDisplayUnit(int)));

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(_optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    setText(BitcoinUnits::name(newUnits));
}

/** Shows context menu with Display Unit options by the mouse coordinates */
void UnitDisplayStatusBarControl::onDisplayUnitsClicked(const QPoint& point)
{
    QPoint globalPos = mapToGlobal(point);
    menu->exec(globalPos);
}

/** Tells underlying optionsModel to update its current display unit. */
void UnitDisplayStatusBarControl::onMenuSelection(QAction* action)
{
    if (action)
    {
        optionsModel->setDisplayUnit(action->data());
    }
}

