#include "graph.h"

#include <QNetworkReply>



/**
 * @brief called from Graph, when closing network, to terminate all crawler processes
 * Also called indirectly when wc_spider finishes
 * @param reason
 */
void Graph::webCrawlTerminateThreads(QString reason)
{
    qDebug() << "Terminating webCrawler threads - reason " << reason
             << "Checking webcrawlerThread...";

    while (webcrawlerThread.isRunning())
    {
        qDebug() << "webcrawlerThread running. "
                    "Calling webcrawlerThread.quit()";
        webcrawlerThread.requestInterruption();
        webcrawlerThread.quit();
        webcrawlerThread.wait();
    }
}

/**
 * @brief
 * Creates a new WebCrawler, that will parse the downloaded HTML code of each webpage
 * we download. Moves the WebCrawler to a new thread and starts the thread.
 * Then creates the fist node (initial url),
 * and starts the web spider to download the first HTML page.
 * Called by MW with user options.
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
 * @param extLinksIncluded
 * @param extLinksCrawl
 * @param socialLinks
 * @param delayedRequests
 */
void Graph::startWebCrawler(
    const QUrl &startUrl,
    const QStringList &urlPatternsIncluded,
    const QStringList &urlPatternsExcluded,
    const QStringList &linkClasses,
    const int &maxNodes,
    const int &maxLinksPerPage,
    const bool &intLinks,
    const bool &childLinks,
    const bool &parentLinks,
    const bool &selfLinks,
    const bool &extLinksIncluded,
    const bool &extLinksCrawl,
    const bool &socialLinks,
    const bool &delayedRequests)
{

    qDebug() << "Setting up a new WebCrawler for url:" << startUrl.toString()
             << "graph thread:" << thread();

    // Rename current relation
    relationCurrentRename(tr("web"), true);

    // Initialize variables
    m_crawler_max_urls = maxNodes; // Store maximum urls we'll visit (max nodes in the resulted network)
    m_crawler_visited_urls = 0;    // Init counter of visited urls

    // Check if we need to add delay between requests
    int delayBetween = 0;
    if (delayedRequests)
    {
        delayBetween = 500; // half second
    }

    // Create our url queue
    urlQueue = new QQueue<QUrl>;

    // Enqueue the start QUrl
    urlQueue->enqueue(startUrl);

    qDebug() << "Creating new WebCrawler...";

    // Create the WebCrawler
    web_crawler = new WebCrawler(
        urlQueue,
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
        extLinksIncluded,
        extLinksCrawl,
        socialLinks,
        delayBetween);

    // Just in case, we reach this place and the thread is still running
    if (webcrawlerThread.isRunning())
    {
        qDebug() << "webcrawlerThread is already running - calling requestInterruption()...";
        webCrawlTerminateThreads("startWebCrawler() to start a new WebCrawler but webcrawlerThread is running...");
    }

    // Move the crawler to another thread
    web_crawler->moveToThread(&webcrawlerThread);

    qDebug() << "WebCrawler created and moved to its own thread:"
             << web_crawler->thread();

    // Connect signals and slots
    qDebug() << "Connect signals/slots with WebCrawler...";
    connect(this, &Graph::signalWebCrawlParse,
            web_crawler, &WebCrawler::parse);

    connect(web_crawler, &WebCrawler::signalStartSpider,
            this, &Graph::webSpider);

    connect(web_crawler, &WebCrawler::signalCreateNode,
            this, &Graph::vertexCreateAtPosRandomWithLabel);

    connect(web_crawler, &WebCrawler::signalCreateEdge,
            this, &Graph::edgeCreateWebCrawler);

    connect(web_crawler, &WebCrawler::finished,
            this, &Graph::webCrawlTerminateThreads);

    connect(&webcrawlerThread, &QThread::finished,
            web_crawler, &QObject::deleteLater);

    // Start the crawler thread...
    qDebug() << "Starting WebCrawler thread...";
    webcrawlerThread.start();

    // Create the initial vertex for the starting url
    qDebug() << "Creating initial node 1, initialUrlStr:" << startUrl.toString();
    vertexCreateAtPosRandomWithLabel(1, startUrl.toString(), false);

    // Call the spider to download the html code of the starting url .
    qDebug() << "Calling webSpider()...";
    this->webSpider();

    qDebug("web crawler and spider started. See the thread running? ");
}

/**
 * @brief
 * A loop, that takes urls awaiting in front of the urlQueue,
 * and signals to the MW to make the network request
 */
void Graph::webSpider()
{

    // repeat while urlQueue has items
    do
    {

        //  Until we crawl all urls in urlQueue.
        if (urlQueue->size() == 0)
        {
            qDebug() << "webSpider - urlQueue is empty. Break for now... ";
            break;
        }

        // or until we have reached m_maxNodes
        if (m_crawler_max_urls > 0 && m_crawler_visited_urls == m_crawler_max_urls)
        {
            qDebug() << "webSpider - reached m_crawler_max_urls. Break.";
            break;
        }

        // Take the first url awaiting in the queue
        qDebug() << "webSpider - urlQueue size: " << urlQueue->size()
                 << " - Taking the first url from the urlQueue  ";
        QUrl currentUrl = urlQueue->dequeue();

        qDebug() << "webSpider - url to download: "
                 << currentUrl
                 << "Increasing m_crawler_visited_urls to:" << m_crawler_visited_urls + 1
                 << "and emitting signal signalNetworkManagerRequest to MW...";

        // Signal MW to make the network request
        emit signalNetworkManagerRequest(currentUrl, NetworkRequestType::Crawler);

        // increase visited urls counter
        m_crawler_visited_urls++;

    } while (urlQueue->size());
}

/**
 * @brief
 * Gets the reply of a MW network request made by Web Crawler,
 * and emits that reply as is to the Web Crawler.
 */
void Graph::slotHandleCrawlerRequestReply()
{

    qDebug() << "Got reply from MW network manager request. Emitting signal to Web Crawler to parse the reply...";

    // Get network reply from the sender
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    // Emit signal to web crawler to parse the reply
    emit signalWebCrawlParse(reply);
}

