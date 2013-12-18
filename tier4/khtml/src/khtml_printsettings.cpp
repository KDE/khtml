/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2002 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "khtml_printsettings.h"

#include <klocalizedstring.h>
#include <QCheckBox>
#include <QLayout>

KHTMLPrintSettings::KHTMLPrintSettings(QWidget *parent)
    : QWidget(parent)
{
	//WhatsThis strings.... (added by pfeifle@kde.org)
	QString whatsThisPrintImages = i18n( "<qt>"
		"<p><strong>'Print images'</strong></p>"
		"<p>"
		"If this checkbox is enabled, images contained in the HTML page will "
		"be printed. Printing may take longer and use more ink or toner."
		"</p>"
		"<p>"
		"If this checkbox is disabled, only the text of the HTML page will be "
		"printed, without the included images. Printing will be faster and use "
		"less ink or toner."
		"</p>"
						" </qt>" );
	QString whatsThisPrintHeader = i18n( "<qt>"
		"<p><strong>'Print header'</strong></p>"
		"<p>"
		"If this checkbox is enabled, the printout of the HTML document will "
		"contain a header line at the top of each page. This header contains "
		"the current date, the location URL of the printed page and the page "
		"number."
		"</p>"
		"<p>"
		"If this checkbox is disabled, the printout of the HTML document will "
		"not contain such a header line."
		"</p>"
						" </qt>" );
	QString whatsThisPrinterFriendlyMode = i18n( "<qt>"
		"<p><strong>'Printerfriendly mode'</strong></p>"
		"<p>"
		"If this checkbox is enabled, the printout of the HTML document will "
		"be black and white only, and all colored background will be converted "
		"into white. Printout will be faster and use less ink or toner."
		"</p>"
		"<p>"
		"If this checkbox is disabled, the printout of the HTML document will "
		"happen in the original color settings as you see in your application. "
		"This may result in areas of full-page color (or grayscale, if you use "
		"a black+white printer). Printout will possibly happen more slowly and "
		"will probably use more toner or ink."
		"</p>"
						" </qt>" );
	setWindowTitle(i18n("HTML Settings"));

	m_printfriendly = new QCheckBox(i18n("Printer friendly mode (black text, no background)"), this);
	m_printfriendly->setWhatsThis(whatsThisPrinterFriendlyMode);
	m_printfriendly->setChecked(true);
	m_printimages = new QCheckBox(i18n("Print images"), this);
	m_printimages->setWhatsThis(whatsThisPrintImages);
	m_printimages->setChecked(true);
	m_printheader = new QCheckBox(i18n("Print header"), this);
	m_printheader->setWhatsThis(whatsThisPrintHeader);
	m_printheader->setChecked(true);

	QVBoxLayout	*l0 = new QVBoxLayout(this);
	l0->addWidget(m_printfriendly);
	l0->addWidget(m_printimages);
	l0->addWidget(m_printheader);
	l0->addStretch(1);
}

KHTMLPrintSettings::~KHTMLPrintSettings()
{
}

bool KHTMLPrintSettings::printFriendly()
{
	return m_printfriendly->isChecked();
}

bool KHTMLPrintSettings::printImages()
{
	return m_printimages->isChecked();
}

bool KHTMLPrintSettings::printHeader()
{
	return m_printheader->isChecked();
}


