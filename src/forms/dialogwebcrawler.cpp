/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0
 Written in Qt
 
                         dialogwebcrawler.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2021 by Dimitris B. Kalamaras
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
    (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDisabled(true);

    ui.seedUrlEdit->setFocus();
    ui.seedUrlEdit->setPlaceholderText("Please enter a url...");

    ui.patternsIncludedTextEdit->setText("*");
    ui.patternsExcludedTextEdit->setText("");

    // Set checkbox/options default values
    intLinks=true;
    childLinks=true;
    parentLinks=false;

    extLinksAllowed=false;
    socialLinks=false;
    extLinks=false;

    ui.intLinksCheckBox->setChecked (intLinks);
    ui.childLinksCheckBox->setChecked(childLinks);
    ui.parentLinksCheckBox->setChecked(parentLinks);

    ui.extLinksAllowedCheckBox->setChecked(extLinksAllowed);
    ui.extLinksCrawlCheckBox->setChecked (extLinks);
    ui.extLinksCrawlCheckBox->setEnabled(extLinksAllowed == true);

    ui.socialLinksCheckBox->setChecked(socialLinks);
    ui.selfLinksCheckBox->setChecked(false);
    ui.waitCheckBox ->setChecked(true);

    connect (ui.seedUrlEdit, &QLineEdit::textChanged,
                     this, &DialogWebCrawler::checkErrors);

    connect (ui.maxUrlsToCrawlSpinBox, &QSpinBox::editingFinished,
                     this, &DialogWebCrawler::checkErrors);

    connect (ui.maxLinksPerPageSpinBox, &QSpinBox::editingFinished,
                     this, &DialogWebCrawler::checkErrors);


    connect (ui.patternsIncludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.patternsExcludedTextEdit, &QTextEdit::textChanged,
             this, &DialogWebCrawler::checkErrors);



    connect (ui.intLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.childLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.parentLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);


    connect (ui.extLinksAllowedCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.extLinksCrawlCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);


    connect (ui.socialLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);


    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );


}


/**
 * @brief Checks crawler form for user input errors
 */
void DialogWebCrawler::checkErrors(){

    //
    // SETUP FLAGS
    //
    bool errorUrl  = false;
    bool errorPatternsIncl = false;
    bool errorPatternsExcl = false;
    bool errorCheckboxes = false;

    //
    // Get seed url, sanitize and validate it
    //
    seedUrlInputStr = (ui.seedUrlEdit)->text();

    qDebug()<< "seed url:" << seedUrlInputStr << "Sanitizing...";

    seedUrlInputStr = seedUrlInputStr.simplified().toLower() ;

    seedUrl = QUrl(seedUrlInputStr);

    qDebug()<< "seed url:" << seedUrl.toString()
            << " scheme " << seedUrl.scheme()
            << " host " << seedUrl.host()
            << " path " << seedUrl.path();


    if ( seedUrl.scheme().isEmpty() ||
         ( seedUrl.scheme() != "http"  && seedUrl.scheme() != "https"  )) {
        qDebug()<< "seed url has no scheme. Setting the default scheme (http) ";
        seedUrl.setUrl("//" + seedUrlInputStr);
        seedUrl.setScheme("http");
        qDebug() << seedUrl;
    }

    if (seedUrl.path().isEmpty() ) {
        qDebug()<< "seed url without path. Adding default path '/'...";
        seedUrl.setPath("/");
    }

    if (! seedUrl.isValid() || seedUrl.host() == "" || !seedUrl.host().contains(".") ) {
        qDebug()<< "Error. seed url not valid.";
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.seedUrlEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        errorUrl  = true;
    }
    else {
        ui.seedUrlEdit->setGraphicsEffect(0);
    }

    //
    // GET SPINBOX VALUES AND CHECKBOX OPTIONS
    //
    maxLinksPerPage = (ui.maxLinksPerPageSpinBox) -> value();
    maxUrlsToCrawl = (ui.maxUrlsToCrawlSpinBox) -> value();


    //
    // CHECK INTERNAL/EXTERNAL LINKS CHECKBOXES. AT LEAST ONE SHOULD BE ENABLED
    //

    intLinks = ui.intLinksCheckBox->isChecked();
    extLinksAllowed = ui.extLinksAllowedCheckBox->isChecked();

    if ( !intLinks && !extLinksAllowed )
    {
        // Both are disabled. Throw error
        errorCheckboxes = true;

        QGraphicsColorizeEffect *effect1 = new QGraphicsColorizeEffect;
        QGraphicsColorizeEffect *effect2 = new QGraphicsColorizeEffect;
        effect1->setColor(QColor("red"));
        effect2->setColor(QColor("red"));
        ui.extLinksAllowedCheckBox->setGraphicsEffect(effect1);
        ui.intLinksCheckBox->setGraphicsEffect(effect2);

        ui.parentLinksCheckBox->setEnabled(false);
        ui.childLinksCheckBox->setEnabled(false);

        ui.extLinksCrawlCheckBox->setEnabled(false);

        ui.selfLinksCheckBox->setEnabled(false);
        ui.socialLinksCheckBox->setEnabled(false);

    }
    else {
        // At least one of internal/external links checkboxes is enabled.

        // Remove any prior error
        ui.extLinksAllowedCheckBox->setGraphicsEffect(0);
        ui.intLinksCheckBox->setGraphicsEffect(0);

        errorCheckboxes = false;

        // If internal link crawling is disabled, disable related options
        ui.selfLinksCheckBox->setEnabled( intLinks == true);
        ui.parentLinksCheckBox->setEnabled(intLinks == true);
        ui.childLinksCheckBox->setEnabled(intLinks == true);

        ui.extLinksCrawlCheckBox->setEnabled(extLinksAllowed==true);
        ui.socialLinksCheckBox->setEnabled(extLinksAllowed==true);
    }


    //
    // CHECK URL PATTERNS TO INCLUDE TEXTEDIT
    //
    qDebug()<< "Checking included url patterns...";
    urlPatternsIncluded = parseTextEditInput (ui.patternsIncludedTextEdit->toHtml());

    if (urlPatternsIncluded.size() == 0 ) {
        // No pattern found. Throw error.
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.patternsIncludedTextEdit->setGraphicsEffect(effect);
        errorPatternsIncl = true;
    }
    else {
        if (urlPatternsIncluded.size() == 1 && urlPatternsIncluded.at(0) =="" ) {
            urlPatternsIncluded.clear();
            qDebug() << "return empty urlPatterns (ALL)";
        }
        ui.patternsIncludedTextEdit->setGraphicsEffect(0);
        errorPatternsIncl = false;

    }


    //
    // CHECK URL PATTERNS TO EXCLUDE TEXTEDIT
    //
    qDebug()<< "Checking excluded url patterns...";
    urlPatternsExcluded = parseTextEditInput (ui.patternsExcludedTextEdit->toHtml());

    if (urlPatternsExcluded.size() == 1 ) {
        if (urlPatternsExcluded.at(0) == "*") {
            QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
            effect->setColor(QColor("red"));
            ui.patternsExcludedTextEdit->setGraphicsEffect(effect);
            errorPatternsExcl = true;
        }
    }
    else {
        ui.patternsExcludedTextEdit->setGraphicsEffect(0);
        errorPatternsExcl = false;
    }


    //
    // ENABLE/DISABLE OK BUTTON
    //
    if ( errorUrl  || errorPatternsIncl || errorPatternsExcl || errorCheckboxes ) {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(false);
    }
    if ( !errorUrl  && !errorPatternsIncl && !errorPatternsExcl   && !errorCheckboxes ) {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
    }

}

/**
 * @brief Parses HTML-formatted input string and returns a list of all strings inside <p> ... </p>
 * @param html
 * @return
 */
QStringList DialogWebCrawler::parseTextEditInput(const QString &html){

    QStringList userInputParsed;

    if ( ! html.isEmpty() ) {

        QStringList userInput ;
        QString data;
        QString str;

            userInput = html.split("<p");
            for (int i = 0; i < userInput.size(); ++i){
               if (i==0) continue;
               data = userInput.at(i).toLocal8Bit().constData();
               //qDebug () << "split " << i << ":: " << data << "\n";
               //qDebug () << "first char > at ::" <<  data.indexOf('>',0) << "\n";
               str = data.mid ( data.indexOf('>',0) +1, data.indexOf("</p>",0) - (data.indexOf('>',0) +1) );
               qDebug () << "str ::" << str ;
               str.remove("<br />");
               str=str.simplified();


               // remove wildcards, if there are any
               if (str.contains("*")) {
                   str.remove("*");
               }

               qDebug () << "str fin ::"  << str;


               // urls and classes cannot contain spaces...
               if (str.contains(" ")) {
                   userInputParsed.clear();
                   qDebug () << "urls cannot contain spaces... Break." ;
                   break;
               }

               userInputParsed << str;


            }

    }
    else {
        userInputParsed.clear();
    }
    qDebug () << "stringlist size"
              << userInputParsed.size()<< "\n";
    return userInputParsed;

}


/**
 * @brief gathers data from web crawler form
 */
void DialogWebCrawler::getUserChoices(){

    qDebug()<< "Emitting user choices:" << "\n"
            << "seedUrl: " << seedUrl.toString() << "\n"
            << "urlPatternsIncluded" << urlPatternsIncluded << "\n"
            << "urlPatternsExcluded" << urlPatternsExcluded << "\n"
            << "linkClasses" << linkClasses << "\n"
            << "maxLinksPerPage " << maxLinksPerPage << "\n"
            << "totalUrlsToCrawl " << maxUrlsToCrawl << "\n"
            << "intLinks"<< ui.intLinksCheckBox->isChecked() << "\n"
            << "childLinks" << ui.childLinksCheckBox->isChecked() << "\n"
            << "parentLinks" << ui.parentLinksCheckBox->isChecked() << "\n"
            << "selfLinks" << ui.selfLinksCheckBox->isChecked() << "\n"
            << "extLinksAllowed"<< ui.extLinksAllowedCheckBox->isChecked()<< "\n"
            << "extLinksCrawl"<< ui.extLinksCrawlCheckBox->isChecked()<< "\n"
            << "socialLinks" << ui.socialLinksCheckBox->isChecked()<< "\n"
            << "delayedRequests" << ui.waitCheckBox->isChecked() << "\n";

    emit userChoices( seedUrl,
                      urlPatternsIncluded,
                      urlPatternsExcluded,
                      linkClasses,
                      maxUrlsToCrawl,
                      maxLinksPerPage,
                      ui.intLinksCheckBox->isChecked(),
                      ui.childLinksCheckBox->isChecked(),
                      ui.parentLinksCheckBox->isChecked(),
                      ui.selfLinksCheckBox->isChecked(),
                      ui.extLinksAllowedCheckBox->isChecked(),
                      ui.extLinksCrawlCheckBox->isChecked(),
                      ui.socialLinksCheckBox->isChecked(),
                      ui.waitCheckBox ->isChecked()
                      );
}
