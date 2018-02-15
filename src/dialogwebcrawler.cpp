/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt
 
                         dialogwebcrawler.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2017 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/



#include "dialogwebcrawler.h"
#include <QDebug>
#include <QTextEdit>
#include <QPushButton>
#include <QGraphicsColorizeEffect>
#include <QUrl>

DialogWebCrawler::DialogWebCrawler(QWidget *parent) : QDialog (parent)
{
    ui.setupUi(this);

    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui.seedUrlEdit)->setFocus();

    connect (ui.seedUrlEdit, &QLineEdit::textChanged,
                     this, &DialogWebCrawler::checkErrors);


    ui.patternsIncludedTextEdit->setText("*");
    ui.patternsIncludedTextEdit->setToolTip("Enter, in separate lines, one or more "
                                            "url patterns to include while crawling. "
                                            "\nI.e. example.com/pattern/*"
                                            "\n\nDo not enter spaces."
                                            "\n\nLeave * to crawl all urls.");

    ui.classesIncludedTextEdit->setText("*");
    ui.classesIncludedTextEdit->setToolTip("Enter, in separate lines, one or more link classes to crawl."
                                           "\nI.e. \nlink \ngraph"
                                           "\n\nDo not enter spaces."
                                           "\n\nLeave * to crawl all urls no matter what class they have.");




    connect (ui.patternsIncludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.classesIncludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.extLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.intLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );

    if ( !ui.extLinksCheckBox->isChecked()  &&!ui.intLinksCheckBox->isChecked() )
    {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
    }

}

/**
 * @brief Checks crawler form for user input errors
 */
void DialogWebCrawler::checkErrors(){
    qDebug()<< "DialogWebCrawler::checkErrors...";


    bool patternErrorFlag = false;
    bool classesErrorFlag = false;
    bool urlErrorFlag = false;
    bool checkboxesErrorFlag = false;

    QString seedUrl = (ui.seedUrlEdit)->text();

    qDebug()<< "DialogWebCrawler::checkErrors() initial seed url "
            << seedUrl
            << " simplifying and lowering it";

    seedUrl = seedUrl.simplified().toLower() ;

    qDebug()<< "DialogWebCrawler::checkErrors() adding / to seed url ";
    seedUrl = seedUrl + "/";

    QUrl newUrl(seedUrl);

    qDebug()<< "DialogWebCrawler::checkErrors()  QUrl " << newUrl.toString()
            << " scheme " << newUrl.scheme()
            << " host " << newUrl.host()
            << " path " << newUrl.path();

    if ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  &&
         newUrl.scheme() != "ftp" && newUrl.scheme() != "ftps") {
        qDebug()<< "DialogWebCrawler::checkErrors()  URL scheme missing "
                << newUrl.scheme()
                << " setting the default scheme http ";
        newUrl = QUrl ("http://" + seedUrl);
        qDebug() << newUrl;
    }

    if (! newUrl.isValid() || newUrl.host() == "" || !newUrl.host().contains(".") ) {
        qDebug()<< "DialogWebCrawler::checkErrors()  not valid URL";
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.seedUrlEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        urlErrorFlag = true;
    }
    else {
        if (!patternErrorFlag && !classesErrorFlag && !checkboxesErrorFlag ) {
            ui.seedUrlEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            urlErrorFlag = false;
        }
    }


    if ( !ui.extLinksCheckBox->isChecked()  && !ui.intLinksCheckBox->isChecked() )
    {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        checkboxesErrorFlag = true;
    }
    else {
        if (!patternErrorFlag && !classesErrorFlag && !urlErrorFlag) {
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            checkboxesErrorFlag = false;
        }
    }


    QStringList patternsToInclude = parseTextEditInput (ui.patternsIncludedTextEdit->toHtml());


    if (patternsToInclude.size() == 0 ) {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.patternsIncludedTextEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        patternErrorFlag = true;
    }
    else {
        if (!classesErrorFlag && !urlErrorFlag && !checkboxesErrorFlag) {
            ui.patternsIncludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            patternErrorFlag = false;
        }
    }

    QStringList classesToInclude  = parseTextEditInput (ui.classesIncludedTextEdit->toHtml());

    if (classesToInclude.size() == 0 ) {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.classesIncludedTextEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        classesErrorFlag = true;
    }
    else {
        if (!patternErrorFlag && !urlErrorFlag && !checkboxesErrorFlag) {
            ui.classesIncludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            classesErrorFlag = false;
        }
    }



}

/**
 * @brief Parses HTML-formatted input string and returns a list of all strings inside <p> ... </p>
 * @param html
 * @return
 */
QStringList DialogWebCrawler::parseTextEditInput(const QString &html){

    qDebug () << "DialogWebCrawler::parseTextEditInput";

    QStringList userInputParsed;

    if ( ! html.isEmpty() ) {

        QStringList userInput ;
        QString data;
        QString str;

            //qDebug () << "DialogWebCrawler::parseTextEditInput";
            userInput = html.split("<p");
            for (int i = 0; i < userInput.size(); ++i){
               if (i==0) continue;
               data = userInput.at(i).toLocal8Bit().constData();
               //qDebug () << "split " << i << ":: " << data << endl;
               //qDebug () << "first char > at ::" <<  data.indexOf('>',0) << endl;
               str = data.mid ( data.indexOf('>',0) +1, data.indexOf("</p>",0) - (data.indexOf('>',0) +1) );
               qDebug () << "str ::" << str << endl;
               str.remove("<br />");
               str=str.simplified();
               qDebug () << "str fin ::"  << str;
               if (str.contains(" ")) {
                   userInputParsed.clear();
                   break;
               }
               if (!str.isEmpty()) {
                   userInputParsed << str;
               }

            }


    }
    else {
        userInputParsed.clear();
    }
    qDebug () << "DialogWebCrawler::parseTextEditInput() - stringlist size"<< userInputParsed.size();
    return userInputParsed;

}


/**
 * @brief gathers data from web crawler form
 */
void DialogWebCrawler::gatherData(){
    qDebug()<< "DialogWebCrawler::gatherData()...";

    bool extLinks=true, intLinks=false;

    QString seedUrl = (ui.seedUrlEdit)->text();

    qDebug()<< "DialogWebCrawler::gatherData() initial seed url "
            << seedUrl
            << " simplifying and lowering it";

    seedUrl = seedUrl.simplified().toLower() ;

    qDebug()<< "DialogWebCrawler::gatherData() adding / to seed url ";
    seedUrl = seedUrl + "/";

    QUrl newUrl(seedUrl);

    qDebug()<< "DialogWebCrawler::gatherData()  QUrl " << newUrl.toString()
            << " scheme " << newUrl.scheme()
            << " host " << newUrl.host()
            << " path " << newUrl.path();

    if ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  &&
         newUrl.scheme() != "ftp" && newUrl.scheme() != "ftps") {
        qDebug()<< "DialogWebCrawler::gatherData()  URL scheme missing "
                << newUrl.scheme()
                << " setting the default scheme http ";
        newUrl = QUrl ("http://" + seedUrl);
        qDebug() << newUrl;
    }

    if (! newUrl.isValid() || newUrl.host() == "") {
        emit webCrawlerDialogError(seedUrl);
        qDebug()<< "DialogWebCrawler::gatherData()  not valid URL";
        return;
    }

    seedUrl = newUrl.toString();

    qDebug()<< "DialogWebCrawler::gatherData()  final seed url " << newUrl
            << " scheme " << newUrl.scheme()
            << " host " << newUrl.host()
            << " path " << newUrl.path();

    int maxLinksPerPage = (ui.maxLinksPerPageSpinBox) -> value();
    int totalUrlsToCrawl = (ui.totalUrlsToCrawlSpinBox) -> value();

    if ( ui.extLinksCheckBox -> isChecked() ) {
        qDebug()<< "	External links will be crawled... " ;
        extLinks = true;
    }
    else {
        qDebug()<< "	No external links... ";
        extLinks = false;
    }
    if ( ui.intLinksCheckBox -> isChecked() ) {
        qDebug()<< "	Internal links will be crawled too. " ;
        intLinks = true;
    }
    else {
        qDebug()<< "	No internal links. ";
        intLinks = false;
        if (!intLinks && !extLinks)
            return;
    }

    qDebug()<< "	seedUrl: " << seedUrl;
    qDebug()<< "	maxLinksPerPage " << maxLinksPerPage
            << "  totalUrlsToCrawl " << totalUrlsToCrawl  ;
    emit userChoices( seedUrl, totalUrlsToCrawl, maxLinksPerPage, extLinks, intLinks );
}
