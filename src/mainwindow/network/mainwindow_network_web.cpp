/**
 * @file mainwindow_network_web.cpp
 * @brief Implements MainWindow Network menu web crawler dialog/launch and the QNetworkAccessManager request/reply/SSL error handlers.
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
#include "webcrawler.h"
#include "forms/dialogpreviewfile.h"
#include "forms/dialogdatasetselect.h"
#include "forms/dialogranderdosrenyi.h"
#include "forms/dialograndsmallworld.h"
#include "forms/dialograndscalefree.h"
#include "forms/dialograndregular.h"
#include "forms/dialograndlattice.h"
#include "forms/dialogwebcrawler.h"
#include "forms/dialogexportimage.h"
#include "forms/dialogexportpdf.h"

#include <QtWidgets>
#include <QTextCodec>
#include <QNetworkReply>
#include <QNetworkAccessManager>

/**
 * @brief Shows the web crawler dialog
 */
void MainWindow::slotNetworkWebCrawlerDialog()
{

    // Close the current network
    if (!this->slotNetworkClose())
    {
        // User cancelled. Do not proceed.
        return;
    }

    qCDebug(lcMainWindow) << "Opening web crawler dialog...";

    m_WebCrawlerDialog = new DialogWebCrawler(this);

    connect(m_WebCrawlerDialog, &DialogWebCrawler::userChoices,
            this, &MainWindow::slotNetworkWebCrawler);

    m_WebCrawlerDialog->exec();
}

/**
 * @brief Starts the web crawler with the user options
 *
 * @param startUrl
 * @param urlPatternsIncluded
 * @param urlPatternsExcluded
 * @param linkClasses
 * @param maxNodes
 * @param maxLinksPerPage
 * @param intLinks
 * @param childLinks
 * @param parentLinks
 * @param selfLinks
 * @param extLinks
 * @param extLinksCrawl
 * @param socialLinks
 * @param delayedRequests
 */
void MainWindow::slotNetworkWebCrawler(const QUrl &startUrl,
                                       const QStringList &urlPatternsIncluded,
                                       const QStringList &urlPatternsExcluded,
                                       const QStringList &linkClasses,
                                       const int &maxNodes,
                                       const int &maxLinksPerPage,
                                       const bool &intLinks,
                                       const bool &childLinks,
                                       const bool &parentLinks,
                                       const bool &selfLinks,
                                       const bool &extLinks,
                                       const bool &extLinksCrawl,
                                       const bool &socialLinks,
                                       const bool &delayedRequests)
{

    // Check ssl
    if (!QSslSocket::supportsSsl())
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL, tr("No SSL support."),
                              tr("I cannot verify that your computer Operating System has OpenSSL support. \n\n"
                                 "OpenSSL is an  Open Source software library for the Transport Layer Security (TLS) protocol (aka SSL), for applications that secure communications over computer networks. It is widely used by Internet servers, including the majority of HTTPS websites. \n\n"
                                 "Without OpenSSL libraries installed in your computer, I cannot crawl webpages/URLs using https:// \n\n"
                                 "So, please download and install OpenSSL in your OS and try again."),
                              tr("Hint: Go to Help > System Information to see which OpenSSL version you need to install."));
        return;
    }

    // Start the web crawler
    qCDebug(lcMainWindow) << "Calling Graph::startWebCrawler() to start the crawler process.";
    activeGraph->startWebCrawler(
        startUrl,
        urlPatternsIncluded,
        urlPatternsExcluded,
        linkClasses,
        maxNodes,
        maxLinksPerPage,
        intLinks,
        childLinks,
        parentLinks,
        selfLinks,
        extLinks,
        extLinksCrawl,
        socialLinks,
        delayedRequests);
}

/**
 * @brief Makes a network request to the given url
 *
 * Creates the QNetworkReply object to handle the reply.
 *
 * @param url
 * @param requestType
 */
void MainWindow::slotNetworkManagerRequest(const QUrl &url, const NetworkRequestType &requestType)
{

    qCDebug(lcMainWindow) << "New network request for url:" << url.toString() << "requestType:" << requestType;

    // Create a network request object
    QNetworkRequest request;

    // Set request url
    request.setUrl(url);

    // Set request headers
    request.setRawHeader(
        "User-Agent",
        "SocNetV harmless spider - see https://socnetv.org");

    // Create a network reply object through which we will make the call and handle the reply content
    qCDebug(lcMainWindow) << "Creating a network reply object and making the call...";
    QNetworkReply *reply = networkManager->get(request);

    // Connect signals and slots
    switch (requestType)
    {
    case NetworkRequestType::Crawler:
        // Wire the reply to the activeGraph, which in turn will pass it to the web crawler
        connect(reply, &QNetworkReply::finished, activeGraph, &Graph::slotHandleCrawlerRequestReply);
        break;
    case NetworkRequestType::CheckUpdate:
        connect(reply, &QNetworkReply::finished, this, &MainWindow::slotHelpCheckUpdateParse);
    default:
        break;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(reply, &QNetworkReply::errorOccurred,
            this, &MainWindow::slotNetworkManagerReplyError);
#endif
}

/**
 * @brief Shows a message box to the user when a NetworkReply encounters errors.
 *
 * The message box contains info about the error code.
 *
 * @param code
 */
void MainWindow::slotNetworkManagerReplyError(const QNetworkReply::NetworkError &code)
{

    // Get network reply from the sender
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    // Get reply error string
    QString replyErrorMsg = reply->errorString();

    // Will store the Qt description of the error
    QString errorMsg;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    switch (code)
    {
    case QNetworkReply::NoError:
        errorMsg = "No error message!";
        break;
    case QNetworkReply::ConnectionRefusedError:
        errorMsg = "the remote server refused the connection (the server is not accepting requests)";
        break;
    case QNetworkReply::RemoteHostClosedError:
        errorMsg = "the remote server closed the connection prematurely, before the entire reply was received and processed";
        break;
    case QNetworkReply::HostNotFoundError:
        errorMsg = "the remote host name was not found (invalid hostname)";
        break;
    case QNetworkReply::TimeoutError:
        errorMsg = "the connection to the remote server timed out";
        break;
    case QNetworkReply::OperationCanceledError:
        errorMsg = "the operation was canceled via calls to abort() or close() before it was finished.";
        break;
    case QNetworkReply::SslHandshakeFailedError:
        errorMsg = "the SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() signal should have been emitted.";
        break;
    case QNetworkReply::TemporaryNetworkFailureError:
        errorMsg = "the connection was broken due to disconnection from the network, however the system has initiated roaming to another access point. The request should be resubmitted and will be processed as soon as the connection is re-established.";
        break;
    case QNetworkReply::NetworkSessionFailedError:
        errorMsg = "the connection was broken due to disconnection from the network or failure to start the network.";
        break;
    case QNetworkReply::BackgroundRequestNotAllowedError:
        errorMsg = "the background request is not currently allowed due to platform policy.";
        break;
    case QNetworkReply::TooManyRedirectsError:
        errorMsg = "while following redirects, the maximum limit was reached. The limit is by default set to 50 or as set by QNetworkRequest::setMaxRedirectsAllowed(). (This value was introduced in 5.6.)";
        break;
    case QNetworkReply::InsecureRedirectError:
        errorMsg = "while following redirects, the network access API detected a redirect from a encrypted protocol (https) to an unencrypted one (http). (This value was introduced in 5.6.)";
        break;
    case QNetworkReply::ProxyConnectionRefusedError:
        errorMsg = "the connection to the proxy server was refused (the proxy server is not accepting requests)";
        break;
    case QNetworkReply::ProxyConnectionClosedError:
        errorMsg = "the proxy server closed the connection prematurely, before the entire reply was received and processed";
        break;
    case QNetworkReply::ProxyNotFoundError:
        errorMsg = "the proxy host name was not found (invalid proxy hostname)";
        break;
    case QNetworkReply::ProxyTimeoutError:
        errorMsg = "the connection to the proxy timed out or the proxy did not reply in time to the request sent";
        break;
    case QNetworkReply::ProxyAuthenticationRequiredError:
        errorMsg = "the proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)";
        break;
    case QNetworkReply::ContentAccessDenied:
        errorMsg = "the access to the remote content was denied (similar to HTTP error 403)";
        break;
    case QNetworkReply::ContentOperationNotPermittedError:
        errorMsg = "the operation requested on the remote content is not permitted";
        break;
    case QNetworkReply::ContentNotFoundError:
        errorMsg = "the remote content was not found at the server (similar to HTTP error 404)";
        break;
    case QNetworkReply::AuthenticationRequiredError:
        errorMsg = "the remote server requires authentication to serve the content but the credentials provided were not accepted (if any)";
        break;
    case QNetworkReply::ContentReSendError:
        errorMsg = "the request needed to be sent again, but this failed for example because the upload data could not be read a second time.";
        break;
    case QNetworkReply::ContentConflictError:
        errorMsg = "the request could not be completed due to a conflict with the current state of the resource.";
        break;
    case QNetworkReply::ContentGoneError:
        errorMsg = "the requested resource is no longer available at the server.";
        break;
    case QNetworkReply::InternalServerError:
        errorMsg = "the server encountered an unexpected condition which prevented it from fulfilling the request.";
        break;
    case QNetworkReply::OperationNotImplementedError:
        errorMsg = "the server does not support the functionality required to fulfill the request.";
        break;
    case QNetworkReply::ServiceUnavailableError:
        errorMsg = "the server is unable to handle the request at this time.";
        break;
    case QNetworkReply::ProtocolUnknownError:
        errorMsg = "the Network Access API cannot honor the request because the protocol is not known";
        break;
    case QNetworkReply::ProtocolInvalidOperationError:
        errorMsg = "the requested operation is invalid for this protocol";
        break;
    case QNetworkReply::UnknownNetworkError:
        errorMsg = "an unknown network-related error was detected";
        break;
    case QNetworkReply::UnknownProxyError:
        errorMsg = "an unknown proxy-related error was detected";
        break;
    case QNetworkReply::UnknownContentError:
        errorMsg = "an unknown error related to the remote content was detected";
        break;
    case QNetworkReply::ProtocolFailure:
        errorMsg = "a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)";
        break;
    case QNetworkReply::UnknownServerError:
        errorMsg = "an unknown error related to the server response was detected";
        break;
    }
#endif

    slotHelpMessageToUserError("Network Error!  \n\n"
                               "Request to: '" +
                               reply->request().url().toString() + "' encountered this error: \n\n" +
                               replyErrorMsg + "\n\n" +
                               "Error description: \n\n" + errorMsg +
                               "\n\nPlease, try again. ");
}

/**
 * @brief Shows a message box to the user when the Network Manager encounters any SSL error.
 *
 * @param reply
 * @param errors
 */
void MainWindow::slotNetworkManagerSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{

    QString sslErrorString;

    // Read errors and get the error decriptions.
    foreach (QSslError error, errors)
    {
        sslErrorString = error.errorString();
    }

    // Show the user a message box
    slotHelpMessageToUserError("SSL Error! \n\n"
                               "Request to: '" +
                               reply->request().url().toString() + "' encountered this SSL error: \n\n" + sslErrorString +
                               "\n\n Please, try again later. ");
}
