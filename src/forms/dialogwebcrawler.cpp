/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt
 
                         dialogwebcrawler.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2018 by Dimitris B. Kalamaras
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
    ui.patternsExcludedTextEdit->setText("");

    extLinksIncluded=false;
    socialLinks=false;
    extLinks=false;
    intLinks=true;
    childLinks=true;
    parentLinks=false;

    ui.intLinksCheckBox->setChecked (intLinks);
    ui.childLinksCheckBox->setChecked(childLinks);
    ui.parentLinksCheckBox->setChecked(parentLinks);

    ui.extLinksIncludedCheckBox->setChecked(extLinksIncluded);
    ui.extLinksCheckBox->setChecked (extLinks);

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


    connect (ui.extLinksIncludedCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);

    connect (ui.extLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);


    connect (ui.socialLinksCheckBox, &QCheckBox::stateChanged,
             this, &DialogWebCrawler::checkErrors);


    connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );


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
    bool checkboxesError = false;

    // CHECK URL

    seedUrl = (ui.seedUrlEdit)->text();

    qDebug()<< "DialogWebCrawler::checkErrors() initial seed url "
            << seedUrl
            << " simplifying and lowering it";

    seedUrl = seedUrl.simplified().toLower() ;

    QUrl newUrl(seedUrl);

    qDebug()<< "DialogWebCrawler::checkErrors()  - QUrl " << newUrl.toString()
            << " scheme " << newUrl.scheme()
            << " host " << newUrl.host()
            << " path " << newUrl.path();


    if ( newUrl.scheme().isEmpty() ||
         ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  )) {
        qDebug()<< "DialogWebCrawler::checkErrors()  URL scheme missing "
                << newUrl.scheme()
                << " setting the default scheme (http) ";
        newUrl.setUrl("//" + seedUrl);
        newUrl.setScheme("http");
        qDebug() << newUrl;
    }



    if (newUrl.path().isEmpty() ) {
        qDebug()<< "DialogWebCrawler::checkErrors() - seed url is a domain without a path. "
                   "Adding default path / to seed url";

        newUrl.setPath("/");
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
        if (!patternsInError && !patternsExError   && !checkboxesError ) {
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
    maxUrlsToCrawl = (ui.maxUrlsToCrawlSpinBox) -> value();


    // CHECK CHECKBOXES (AT LEAST ONE SHOULD BE ENABLED)

    if ( !ui.extLinksIncludedCheckBox->isChecked()  && !ui.intLinksCheckBox->isChecked() )
    {
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);

        checkboxesError = true;
        QGraphicsColorizeEffect *effect1 = new QGraphicsColorizeEffect;
        QGraphicsColorizeEffect *effect2 = new QGraphicsColorizeEffect;
        effect1->setColor(QColor("red"));
        effect2->setColor(QColor("red"));
        ui.extLinksIncludedCheckBox->setGraphicsEffect(effect1);
        ui.intLinksCheckBox->setGraphicsEffect(effect2);

        ui.selfLinksCheckBox->setEnabled(false);
        ui.parentLinksCheckBox->setEnabled(false);
        ui.childLinksCheckBox->setEnabled(false);
        ui.selfLinksCheckBox->setChecked(false),
        ui.parentLinksCheckBox->setChecked(false);
        ui.childLinksCheckBox->setChecked(false);

        ui.extLinksCheckBox->setEnabled(false);
        ui.extLinksCheckBox->setChecked(false);
        ui.socialLinksCheckBox->setChecked(false);
        ui.socialLinksCheckBox->setEnabled(false);

    }
    else {
        ui.extLinksIncludedCheckBox->setGraphicsEffect(0);
        ui.intLinksCheckBox->setGraphicsEffect(0);

        if (!patternsInError && !patternsExError && !urlError ) {
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            checkboxesError = false;

            intLinks = ui.intLinksCheckBox->isChecked();
            extLinksIncluded = ui.extLinksIncludedCheckBox->isChecked();
            extLinks = ui.extLinksCheckBox->isChecked();
            socialLinks = ui.socialLinksCheckBox->isChecked();

            if (!intLinks) {
                ui.selfLinksCheckBox->setChecked(false),
                ui.parentLinksCheckBox->setChecked(false);
                ui.childLinksCheckBox->setChecked(false);
                ui.selfLinksCheckBox->setEnabled(false);
                ui.parentLinksCheckBox->setEnabled(false);
                ui.childLinksCheckBox->setEnabled(false);
            }
            else {
                ui.selfLinksCheckBox->setEnabled(true);
                ui.parentLinksCheckBox->setEnabled(true);
                ui.childLinksCheckBox->setEnabled(true);
                ui.selfLinksCheckBox->setCheckable(true);
                ui.parentLinksCheckBox->setCheckable(true);
                ui.childLinksCheckBox->setCheckable(true);

            }

            if (!extLinksIncluded) {
                ui.extLinksCheckBox->setEnabled(false);
                ui.extLinksCheckBox->setChecked(false);
                ui.socialLinksCheckBox->setChecked(false);
                ui.socialLinksCheckBox->setEnabled(false);
            }
            else {
                ui.extLinksCheckBox->setEnabled(true);
                ui.socialLinksCheckBox->setEnabled(true);
                ui.extLinksCheckBox->setCheckable(true);
                ui.socialLinksCheckBox->setCheckable(true);

            }
            if (extLinks) {
                ui.extLinksIncludedCheckBox->setChecked(true);
            }

        }
    }


    // CHECK URL PATTERNS TO INCLUDE TEXTEDIT
    qDebug()<< "DialogWebCrawler::checkErrors()  - checking allowed patterns ";
    urlPatternsIncluded = parseTextEditInput (ui.patternsIncludedTextEdit->toHtml());


    if (urlPatternsIncluded.size() == 0 ) {
        QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
        effect->setColor(QColor("red"));
        ui.patternsIncludedTextEdit->setGraphicsEffect(effect);
        (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setDisabled(true);
        patternsInError = true;
    }
    else {
        if (urlPatternsIncluded.size() == 1 && urlPatternsIncluded.at(0) =="" ) {
            urlPatternsIncluded.clear();
            qDebug() << "DialogWebCrawler::checkErrors() - return empty urlPatterns (ALL)";
        }

        if ( !patternsExError && !urlError  && !checkboxesError) {
            ui.patternsIncludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            patternsInError = false;
        }
    }


    // CHECK URL PATTERNS TO EXCLUDE TEXTEDIT
    qDebug()<< "DialogWebCrawler::checkErrors()  - checking disallowed patterns ";
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
        if ( !patternsInError && !urlError && !checkboxesError) {
            ui.patternsExcludedTextEdit->setGraphicsEffect(0);
            (ui.buttonBox) -> button (QDialogButtonBox::Ok)->setEnabled(true);
            patternsExError = false;
        }
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
               //qDebug () << "split " << i << ":: " << data << endl;
               //qDebug () << "first char > at ::" <<  data.indexOf('>',0) << endl;
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
    qDebug () << "DialogWebCrawler::parseTextEditInput() - stringlist size"
              << userInputParsed.size()<< endl;
    return userInputParsed;

}


/**
 * @brief gathers data from web crawler form
 */
void DialogWebCrawler::getUserChoices(){
    qDebug()<< "DialogWebCrawler::getUserChoices() - Emitting" << endl
            << "	seedUrl: " << seedUrl << endl
            << "	maxLinksPerPage " << maxLinksPerPage << endl
            << "	totalUrlsToCrawl " << maxUrlsToCrawl << endl
            << "	urlPatternsIncluded" << urlPatternsIncluded << endl
            << "	urlPatternsExcluded" << urlPatternsExcluded << endl
            << "	linkClasses" << linkClasses << endl;

    emit userChoices( seedUrl,
                      urlPatternsIncluded,
                      urlPatternsExcluded,
                      linkClasses,
                      maxUrlsToCrawl,
                      maxLinksPerPage,
                      intLinks,
                      ui.childLinksCheckBox->isChecked(),
                      ui.parentLinksCheckBox->isChecked(),
                      ui.selfLinksCheckBox->isChecked(),
                      ui.waitCheckBox ->isChecked(),
                      ui.extLinksIncludedCheckBox->isChecked(),
                      ui.extLinksCheckBox->isChecked(),
                      ui.socialLinksCheckBox ->isChecked()
                      );
}
