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

    ui.patternsIncludedTextEdit->setText("*");
    ui.patternsIncludedTextEdit->setToolTip("<b>ALLOWED URL PATTERNS</b>\n"
                                            "Enter, in separate lines, one or more "
                                            "url patterns to <b>include</b> while crawling. "
                                            "\nI.e. example.com/pattern/*"
                                            "\n\nDo not enter spaces."
                                            "\n\nLeave * to crawl all urls.");

    ui.patternsExcludedTextEdit->setText("");
    ui.patternsExcludedTextEdit->setToolTip("<b>NOT ALLOWED URL PATTERNS</b>\n"
                                            "Enter, in separate lines, one or more "
                                            "url patterns to <b>exclude</b> while crawling. "
                                            "\nI.e. example.com/pattern/*"
                                            "\n\nDo not enter spaces."
                                            "\n\nLeave empty to crawl all urls.");


    ui.classesIncludedTextEdit->setText("*");
    ui.classesIncludedTextEdit->setToolTip("<b>ALLOWED LINK CLASSES</b>\n"
                                           "Enter, in separate lines, one or more link classes to crawl."
                                           "\nI.e. \nlink \ngraph"
                                           "\n\nDo not enter spaces."
                                           "\n\nLeave * to crawl all urls no matter what class they have.");


    connect (ui.seedUrlEdit, &QLineEdit::textChanged,
                     this, &DialogWebCrawler::checkErrors);

    connect (ui.patternsIncludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.patternsExcludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);


    connect (ui.classesIncludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.extLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.intLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(gatherData()) );

    extLinks=false;
    intLinks=true;

    ui.extLinksCheckBox->setChecked (extLinks);
    ui.intLinksCheckBox->setChecked (intLinks);


}

/**
 * @brief Checks crawler form for user input errors
 */
void DialogWebCrawler::checkErrors(){
    qDebug()<< "DialogWebCrawler::checkErrors...";

    /* FLAGS  */

    bool urlError  = false;
    bool patternsInError = false;
    bool patternsExError = false;
    bool classesError  = false;
    bool checkboxesError = false;

    // CHECK URL

    seedUrl = (ui.seedUrlEdit)->text();

    qDebug()<< "DialogWebCrawler::checkErrors() initial seed url "
            << seedUrl
            << " simplifying and lowering it";

    seedUrl = seedUrl.simplified().toLower() ;

    if (!seedUrl.endsWith("/")) {
        qDebug()<< "DialogWebCrawler::checkErrors() adding / to seed url ";
        //seedUrl = seedUrl + "/";
    }

    QUrl newUrl(seedUrl);

    qDebug()<< "DialogWebCrawler::checkErrors()  - QUrl " << newUrl.toString()
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
        qDebug()<< "DialogWebCrawler::checkErrors() - seedUrl not valid.";
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.seedUrlEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        urlError  = true;
    }
    else {
        if (!patternsInError && !patternsExError && !classesError  && !checkboxesError ) {
            ui.seedUrlEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            urlError  = false;
            seedUrl = newUrl.toString();
            qDebug()<< "DialogWebCrawler::checkErrors()  final seed url " << newUrl
                    << " scheme " << newUrl.scheme()
                    << " host " << newUrl.host()
                    << " path " << newUrl.path();

        }
    }

    // CHECK MAX LIMITS  SPIN BOXES

    maxLinksPerPage = (ui.maxLinksPerPageSpinBox) -> value();
    totalUrlsToCrawl = (ui.totalUrlsToCrawlSpinBox) -> value();


    // CHECK CHECKBOXES (AT LEAST ONE SHOULD BE ENABLED)

    if ( !ui.extLinksCheckBox->isChecked()  && !ui.intLinksCheckBox->isChecked() )
    {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        checkboxesError = true;
    }
    else {
        if (!patternsInError && !patternsExError && !classesError  && !urlError ) {
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            checkboxesError = false;
            extLinks = ui.extLinksCheckBox->isChecked();
            intLinks = ui.intLinksCheckBox->isChecked();
        }
    }


    // CHECK URL PATTERNS TO INCLUDE TEXTEDIT

    urlPatternsIncluded = parseTextEditInput (ui.patternsIncludedTextEdit->toHtml());


    if (urlPatternsIncluded.size() == 0 ) {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.patternsIncludedTextEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        patternsInError = true;
    }
    else {
        if (!classesError  && !patternsExError && !urlError  && !checkboxesError) {
            ui.patternsIncludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            patternsInError = false;
        }
    }


    // CHECK URL PATTERNS TO EXCLUDE TEXTEDIT

    urlPatternsExcluded = parseTextEditInput (ui.patternsExcludedTextEdit->toHtml());


    if (urlPatternsExcluded.size() == 1 ) {
        if (urlPatternsExcluded.at(0) == "*") {
            QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
            effect->setColor(QColor("red"));
            ui.patternsExcludedTextEdit->setGraphicsEffect(effect);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
            patternsExError = true;
        }
    }
    else {
        if (!classesError && !patternsInError && !urlError && !checkboxesError) {
            ui.patternsExcludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            patternsExError = false;
        }
    }

    // CHECK LINK CLASSES TO INCLUDE TEXTEDIT

    linkClasses  = parseTextEditInput (ui.classesIncludedTextEdit->toHtml());

    if (linkClasses.size() == 0 ) {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.classesIncludedTextEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        classesError  = true;
    }
    else {
        if (!patternsInError && !patternsExError && !urlError  && !checkboxesError) {
            ui.classesIncludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            classesError  = false;
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

            userInput = html.split("<p");
            for (int i = 0; i < userInput.size(); ++i){
               if (i==0) continue;
               data = userInput.at(i).toLocal8Bit().constData();
               //qDebug () << "split " << i << ":: " << data << endl;
               //qDebug () << "first char > at ::" <<  data.indexOf('>',0) << endl;
               str = data.mid ( data.indexOf('>',0) +1, data.indexOf("</p>",0) - (data.indexOf('>',0) +1) );
               qDebug () << "str ::" << str ;
               str.remove("<br />");
               str=str.simplified();
               // remove wildcard
               if (str.contains("*")) {
                   str.remove("*");
               }

               qDebug () << "str fin ::"  << str;

               // urls and classes cannot contain spaces...
               if (str.contains(" ")) {
                   userInputParsed.clear();
                   qDebug () << "urls and classes cannot contain spaces... Break." ;
                   break;
               }

               userInputParsed << str;


            }


    }
    else {
        userInputParsed.clear();
    }
    qDebug () << "DialogWebCrawler::parseTextEditInput() - stringlist size"
              << userInputParsed.size()<< endl;
    return userInputParsed;

}


/**
 * @brief gathers data from web crawler form
 */
void DialogWebCrawler::gatherData(){
    qDebug()<< "DialogWebCrawler::gatherData() - Emitting" << endl
            << "	seedUrl: " << seedUrl << endl
            << "	maxLinksPerPage " << maxLinksPerPage << endl
            << "	totalUrlsToCrawl " << totalUrlsToCrawl << endl
            << "	urlPatternsIncluded" << urlPatternsIncluded << endl
            << "	urlPatternsExcluded" << urlPatternsExcluded << endl
            << "	linkClasses" << linkClasses << endl;

    emit userChoices( seedUrl,
                      urlPatternsIncluded,
                      urlPatternsExcluded,
                      linkClasses,
                      totalUrlsToCrawl,
                      maxLinksPerPage,
                      extLinks,
                      intLinks
                      );
}
