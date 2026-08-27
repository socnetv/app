/**
 * @file mainwindow_help.cpp
 * @brief Implements MainWindow Help menu and the shared user-message dialog helpers (slotHelpMessageToUser* - despite the name, used throughout the app, not just by Help actions).
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "mainwindow.h"
#include "graph.h"
#include "graphicswidget.h"
#include "texteditor.h"
#include "forms/dialogsettings.h"
#include "forms/dialogsysteminfo.h"

#include <QtWidgets>

/**
 * @brief Compares two version strings component-by-component.
 *
 * Handles versions with 1, 2, or 3 components (e.g. "3.3", "3.3.1", "3.10").
 * Returns:
 *   -1 if a < b
 *    0 if a == b
 *   +1 if a > b
 */
static int compareVersions(const QString &a, const QString &b)
{
    const QStringList aParts = a.split('.');
    const QStringList bParts = b.split('.');
    const int len = qMax(aParts.size(), bParts.size());
    for (int i = 0; i < len; ++i)
    {
        const int av = (i < aParts.size()) ? aParts[i].toInt() : 0;
        const int bv = (i < bParts.size()) ? bParts[i].toInt() : 0;
        if (av < bv)
            return -1;
        if (av > bv)
            return +1;
    }
    return 0;
}

/**
 * @brief Helper function to display a popup with useful info
 * @param text
 */
void MainWindow::slotHelpMessageToUserInfo(const QString text)
{
    slotHelpMessageToUser(USER_MSG_INFO, tr("Useful information"), text);
}

/**
 * @brief Helper function to display a popup with an error message
 * @param text
 */
void MainWindow::slotHelpMessageToUserError(const QString text)
{
    slotHelpMessageToUser(USER_MSG_CRITICAL, tr("Error"), text);
}

/**
 * @brief Displays a popup with the given text/info and a status message
 *
 * @param type
 * @param statusMsg
 * @param text
 * @param info
 * @param buttons
 * @param defBtn
 * @param btn1
 * @param btn2
 * @return
 */
int MainWindow::slotHelpMessageToUser(const int type,
                                      const QString statusMsg,
                                      const QString text,
                                      const QString info,
                                      QMessageBox::StandardButtons buttons,
                                      QMessageBox::StandardButton defBtn,
                                      const QString btn1,
                                      const QString btn2,
                                      const QString btn3)
{
    int response = 0;
    QMessageBox msgBox;
    msgBox.setMinimumWidth(400);
    QPushButton *pbtn1, *pbtn2;

    switch (type)
    {
    case USER_MSG_INFO:
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Information");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        msgBox.setIcon(QMessageBox::Information);
        if (buttons == QMessageBox::NoButton)
        {
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
        }
        else
        {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }
        msgBox.setDefaultButton(defBtn);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL:
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Error");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        // msgBox.setTextFormat(Qt::RichText);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_NETWORK:
        statusMessage(tr("Nothing to do! Load or create a social network first"));
        msgBox.setWindowTitle("Error");
        msgBox.setText(
            tr("No network! \n"
               "Load social network data or create a new social network first. \n"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_CRITICAL_NO_EDGES:
        statusMessage(tr("Nothing to do! Load social network data or create edges first"));
        msgBox.setWindowTitle("Error");
        msgBox.setText(
            tr("No edges! \n"
               "Load social network data or create some edges first. \n"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION:
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Question");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        if (buttons == QMessageBox::NoButton)
        {
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
        }
        else
        {
            msgBox.setStandardButtons(buttons);
            msgBox.setDefaultButton(defBtn);
        }

        msgBox.setIcon(QMessageBox::Question);
        response = msgBox.exec();

        break;

    case USER_MSG_QUESTION_CUSTOM: // a custom question with just two/three buttons
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setWindowTitle("Question");
        msgBox.setText(text);
        if (!info.isNull())
            msgBox.setInformativeText(info);
        pbtn1 = msgBox.addButton(btn1, QMessageBox::ActionRole);
        pbtn2 = msgBox.addButton(btn2, QMessageBox::ActionRole);
        if (!btn3.isNull() && !btn3.isEmpty())
        {
            QPushButton *pbtn3 = msgBox.addButton(btn3, QMessageBox::ActionRole);
            msgBox.setIcon(QMessageBox::Question);
            response = msgBox.exec();
            if (msgBox.clickedButton() == pbtn1)
                response = 1;
            else if (msgBox.clickedButton() == pbtn2)
                response = 2;
            else if (msgBox.clickedButton() == pbtn3)
                response = 3;
        }
        else
        {
            msgBox.setIcon(QMessageBox::Question);
            response = msgBox.exec();
            if (msgBox.clickedButton() == pbtn1)
                response = 1;
            else if (msgBox.clickedButton() == pbtn2)
                response = 2;
        }
        break;
    default: // just for sanity
        if (!statusMsg.isNull())
            statusMessage(statusMsg);
        msgBox.setText(text);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        response = msgBox.exec();
        break;
    }
    return response;
}

/**
 *  Displays a random tip
 */
void MainWindow::slotHelpTips()
{
    int randomTip = rand() % (tips.size()); // Pick a tip.
    QMessageBox::about(this, tr("Tip Of The Day"), tips[randomTip]);
}

/**
    Creates our tips.
*/
void MainWindow::slotHelpCreateTips()
{
    tips += tr("To create a new node: \n"
               "- double-click somewhere on the canvas \n"
               "- or press the keyboard shortcut CTRL+. (dot)\n"
               "- or press the Add Node button on the left panel");
    tips += tr("SocNetV can work with either undirected or directed data. "
               "When you start SocNetV for the first time, the application uses "
               "the 'directed data' mode; every edge you create is directed. "
               "To enter the 'undirected data' mode, press CTRL+E+U or enable the "
               "menu option Edit->Edges->Undirected Edges ");
    tips += tr("If your screen is small, and the canvas appears even smaller "
               "hide the Control and/or Statistics panel. Then the canvas "
               "will expand to the whole application window. "
               "Open the Settings/Preferences dialog->Window options and "
               "disable the two panels.");
    tips += tr("A scale-free network is a network whose degree distribution follows a power law. "
               "SocNetV generates random scale-free networks according to the "
               "Barabási–Albert (BA) model using a preferential attachment mechanism.");
    tips += tr("To delete a node permanently: \n"
               "- right-click on it and select Remove Node \n"
               "- or press CTRL+ALT+. and enter its number\n"
               "- or press the Remove Node button on the Control Panel");
    tips += tr("To rotate the network: \n"
               " - drag the bottom slider to left or right \n"
               " - or click the buttons on the corners of the bottom slider\n"
               " - or press CTRL and the left or right arrow.");
    tips += tr("To create a new edge between nodes A and B: \n"
               "- double-click on node A, then double-click on node B.\n"
               "- or middle-click on node A, and again on node B.\n"
               "- or right-click on the node, then select Add Edge from the popup.\n"
               "- or press the keyboard shortcut CTRL+/ \n"
               "- or press the Add Edge button on the Control Panel");
    tips += tr("Add a label to an edge by right-clicking on it "
               "and selecting Change Label.");
    tips += tr("You can change the background color of the canvas. "
               "Do it from the menu Options > View or "
               "permanently save this setting in Settings/Preferences.");
    tips += tr("Default node colors, shapes and sizes can be changed. "
               "Open the Settings/Preferences dialog and use the "
               "options on the Node tab.");
    tips += tr("The Statistics Panel shows network-level information (i.e. density) "
               "as well as info about any node you clicked on (inDegrees, "
               "outDegrees, clustering).");
    tips += tr("You can move any node by left-clicking and dragging it with your mouse. "
               "If you want you can move multiple nodes at once. Left-click on empty space "
               "on the canvas and drag to create a rectangle selection around them. "
               "Then left-click on one of the selected nodes and drag it.");
    tips += tr("To save the node positions in a network, you need to save your data "
               "in a format which supports node positions, suchs as GraphML or Pajek.");
    tips += tr("Embed visualization models on the network from the options in "
               "the Layout menu or the select boxes on the left Control Panel. ");
    tips += tr("To change the label of a node right-click on it, and click "
               "Selected Node Properties from the popup menu.");
    tips += tr("All basic operations of SocNetV are available from the left Control panel "
               "or by right-clicking on a Node or an Edge or on canvas empty space.");
    tips += tr("Node info (number, position, degree, etc) is displayed on the Status bar, "
               "when you left-click on it.");
    tips += tr("Edge information is displayed on the Status bar, when you left-click on it.");
    tips += tr("Save your work often, especially when working with large data sets. "
               "SocNetV alogorithms are faster when working with saved data. ");

    tips += tr("You can change the precision of real numbers in reports.  "
               "Go to Settings > General and change it under Reports > Real number precision. ");

    tips += tr("The Closeness Centrality (CC) of a node v, is the inverse sum of "
               "the shortest distances between v and every other node. CC is "
               "interpreted as the ability to access information through the "
               "\'grapevine\' of network members. Nodes with high closeness "
               "centrality are those who can reach many other nodes in few steps. "
               "This index can be calculated in both graphs and digraphs. "
               "It can also be calculated in weighted graphs although the weight of "
               "each edge (v,u) in E is always considered to be 1. ");

    tips += tr("The Information Centrality (IC) index counts all paths between "
               "nodes weighted by strength of tie and distance. "
               "This centrality  measure developed by Stephenson and Zelen (1989) "
               "focuses on how information might flow through many different paths. "
               "This index should be calculated only for undirected graphs. "
               "Note: To compute this index, SocNetV drops all isolated nodes.");
}

/**
 * @brief
 * Opens the system web browser to load the online Manual
 */
void MainWindow::slotHelp()
{
    statusMessage(tr("Opening the SocNetV Manual in your default web browser...."));
    QDesktopServices::openUrl(QUrl("https://socnetv.org/manual/?utm_source=application&utm_medium=banner&utm_campaign=socnetv" + VERSION));
}

/**
 * @brief On user demand, makes a network request to SocNetV website to
 * download the latest version text file.
 */
void MainWindow::slotHelpCheckUpdateDialog()
{

    QString url = "https://socnetv.org/latestversion.txt";

    qCDebug(lcMainWindow) << "Will make a 'check for updates' request to url:" << url;

    slotNetworkManagerRequest(QUrl(url), NetworkRequestType::CheckUpdate);
}

/**
 * @brief Parses the reply from the network request we do in slotHelpCheckUpdateDialog
 */
void MainWindow::slotHelpCheckUpdateParse()
{
    qCDebug(lcMainWindow) << "MW::slotHelpCheckUpdateParse()";

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QByteArray ba = reply->readAll();
    reply->deleteLater();

    QString replyContent = QString(ba).simplified();

    if (replyContent.isEmpty())
    {
        slotHelpMessageToUserError(
            "Empty response from https://socnetv.org. "
            "Could not get the latest version number. Please try again later...");
        return;
    }

    // Validate: remote version must look like digits and dots only
    static const QRegularExpression versionRx(R"(^\d+(\.\d+){0,2}$)");
    if (!versionRx.match(replyContent).hasMatch())
    {
        slotHelpMessageToUserError(
            "Could not understand the version number I got from https://socnetv.org. "
            "Please, try again later...");
        return;
    }

    const QString remoteVersion = replyContent;

    // Strip pre-release suffixes from local version (beta, rc, dev)
    QString localVersion = VERSION;
    static const QRegularExpression preReleaseSuffixRx(R"([-.]?(beta|rc|dev)\d*)", QRegularExpression::CaseInsensitiveOption);
    localVersion.remove(preReleaseSuffixRx);

    qCDebug(lcMainWindow) << "MW::slotHelpCheckUpdateParse() - localVersion:" << localVersion
             << "remoteVersion:" << remoteVersion;

    const int cmp = compareVersions(remoteVersion, localVersion);

    if (cmp > 0)
    {
        // Remote is newer
        switch (slotHelpMessageToUser(
            USER_MSG_QUESTION,
            tr("Newer SocNetV version available!"),
            tr("<p>Your version: ") + VERSION + "</p>" +
                tr("<p>Remote version: <b>") + remoteVersion + "</b></p>",
            tr("<p><b>There is a newer SocNetV version available!</b></p>"
               "<p>Do you want to download the latest version now?</p>"
               "<p>Press Yes, and I will open your default web browser for you "
               "to download the latest SocNetV package...</p>"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes))
        {
        case QMessageBox::Yes:
            statusMessage(tr("Opening SocNetV website in your default web browser...."));
            QDesktopServices::openUrl(QUrl(
                "https://socnetv.org/downloads"
                "?utm_source=application&utm_medium=banner&utm_campaign=socnetv" +
                VERSION));
            break;
        default:
            break;
        }
    }
    else
    {
        // Up to date (cmp == 0) or somehow ahead (cmp < 0, e.g. on a dev build)
        slotHelpMessageToUserInfo(
            tr("<p>Your version: ") + VERSION + "</p>" +
            tr("<p>Remote version: ") + remoteVersion + "</p>" +
            tr("<p>You are running the latest and greatest version of SocNetV.<br/>"
               "Nothing to do!</p>"));
    }
}

/**
 * @brief Shows a dialog with system information for bug reporting purposes
 */
void MainWindow::slotHelpSystemInfo()
{
    qCDebug(lcMainWindow) << "MW: slotHelpSystemInfo()";

    m_systemInfoDialog = new DialogSystemInfo(this);

    m_systemInfoDialog->exec();
}

/**
    Displays the following message!!
*/
void MainWindow::slotHelpAbout()
{
    int randomCookie = rand() % fortuneCookie.size();

    QMessageBox::about(
        this, tr("About SocNetV"),
        tr("<b>Soc</b>ial <b>Net</b>work <b>V</b>isualizer (SocNetV)") +
            tr("<p><b>Version</b>: ") + VERSION + "</p>" +

            tr("<p>Website: <a href=\"https://socnetv.org\">https://socnetv.org</a></p>") +

            tr("<p>(C) 2005-2026 by Dimitris B. Kalamaras</p>") +
            tr("<p><a href=\"https://socnetv.org/contact\">Have questions? Contact us!</a></p>") +

            tr("<p><b>Fortune cookie: </b><br> \"") + fortuneCookie[randomCookie] + "\"" +

            tr("<p><b>License:</b><p>") +

            tr("<p>This program is free software; you can redistribute it "
               "and/or modify it under the terms of the GNU General "
               "Public License as published by the Free Software Foundation; "
               "either version 3 of the License, or (at your option) "
               "any later version.</p>") +

            tr("<p>This program is distributed in the hope that it "
               "will be useful, but WITHOUT ANY WARRANTY; "
               "without even the implied warranty of MERCHANTABILITY "
               "or FITNESS FOR A PARTICULAR PURPOSE. "
               "See the GNU General Public License for more details.</p>") +

            tr("<p>You should have received a copy of the GNU "
               "General Public License along with this program; "
               "If not, see http://www.gnu.org/licenses/</p>"));
}

/**
    Creates the fortune cookies displayed on the above message.
*/
void MainWindow::createFortuneCookies()
{
    fortuneCookie += "sic itur ad astra / sic transit gloria mundi ? <br /> "
                     "--Unknown";
    fortuneCookie += "The truth is not my business. I am a statistician... I don’t like words like \"correct\" and \"truth\". "
                     "Statistics is about measuring against convention. <br /> "
                     "Walter Radermacher, Eurostat director, interview to NY Times, 2012.";
    fortuneCookie += "Losers of yesterday, the winners of tomorrow... <br /> "
                     "--B.Brecht";
    fortuneCookie += "I've seen things you people wouldn't believe. Attack ships on fire off the shoulder of Orion. "
                     "I watched C-beams glitter in the dark near the Tannhauser gate. "
                     "All those moments will be lost in time... like tears in rain... Time to die.<br />"
                     "Replicant Roy Batty, Blade Runner (1982)";
    fortuneCookie += "Patriotism is the virtue of the wicked... <br /> "
                     "--O. Wilde";
    fortuneCookie += "No tengo nunca mas, no tengo siempre. En la arena <br />"
                     "la victoria dejo sus piers perdidos.<br />"
                     "Soy un pobre hombre dispuesto a amar a sus semejantes.<br />"
                     "No se quien eres. Te amo. No doy, no vendo espinas. <br /> "
                     "--Pablo Neruda";
    fortuneCookie += "Man must not check reason by tradition, but contrawise, "
                     "must check tradition by reason.<br> --Leo Tolstoy";
    fortuneCookie += "Only after the last tree has been cut down, <br>"
                     "only after the last river has been poisoned,<br> "
                     "only after the last fish has been caught,<br>"
                     "only then will you realize that money cannot be eaten. <br> "
                     "--The Cree People";
    fortuneCookie += "Stat rosa pristina nomine, nomina nuda tenemus <br >"
                     " --Unknown";
    fortuneCookie += "Jupiter and Saturn, Oberon, Miranda <br />"
                     "And Titania, Neptune, Titan. <br />"
                     "Stars can frighten. <br /> Syd Barrett";
    fortuneCookie += "In theory, there is no difference between theory and practice. <br /> "
                     "In practice, there is. <br /> --Yogi Berra";
}

/**
    Displays a short message about the Qt Toolbox.
*/
void MainWindow::slotAboutQt()
{
    QMessageBox::aboutQt(this, "About Qt - SocNetV");
}
