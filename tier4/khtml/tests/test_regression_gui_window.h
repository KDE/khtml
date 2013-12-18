/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef TEST_REGRESSION_WINDOW_H
#define TEST_REGRESSION_WINDOW_H

#include <kio/job.h>

#include <QtCore/QQueue>
#include <QtCore/QProcess>
#include <QUrl>

#include "khtml_part.h"
#include "ui_test_regression_gui.h"

class TestRegressionWindow : public QMainWindow
{
Q_OBJECT

public:
	TestRegressionWindow(QWidget *parent = 0);
	virtual ~TestRegressionWindow();

private Q_SLOTS:
	void toggleJSTests(bool checked);
	void toggleHTMLTests(bool checked);
	void toggleDebugOutput(bool checked);
	void toggleNoXvfbUse(bool checked);

	void setTestsDirectory();
	void setKHTMLDirectory();
	void setOutputDirectory();

	void directoryListingResult(KIO::Job *job, const KIO::UDSEntryList &list);
	void directoryListingFinished(KJob *job);

	void pauseContinueButtonClicked();
	void saveLogButtonClicked();

	void treeWidgetContextMenuRequested(const QPoint &pos);

	void runTests();
	void runSingleTest();

	void processQueue();

	void testerExited(int exitCode, QProcess::ExitStatus exitStatus);
	void testerReceivedData();

	void addToIgnores();
	void removeFromIgnores();

private: // Helpers
	enum TestResult
	{
		Unknown			= 0,
		Crash			= 1,
		Pass			= 2,
		PassUnexpected	= 3,
		Fail			= 4,
		FailKnown		= 5
	};

	void initLegend();
	void initOutputBrowser();
	void initTestsDirectory();
	void initRegressionTesting(const QString &testFileName);
	void updateItemStatus(TestResult result, QTreeWidgetItem *item, const QString &testFileName);
	void updateLogOutput(const QString &data);
	void updateProgressBarRange() const;
	void parseRegressionTestingOutput(QString data, TestResult result, const QString &cacheName);

	unsigned long countLogLines() const;

	QString extractTestNameFromData(QString &data, TestResult &result) const;
	QStringList readListFile(const QString &fileName) const;
	void writeListFile(const QString &fileName, const QStringList &content) const;
	void loadOutputHTML() const;

	QString pathFromItem(const QTreeWidgetItem *item) const;

public:
	// Flags
	enum ProcessArgument
	{
		None		= 0x0,
		JSTests		= 0x1,
		HTMLTests	= 0x2,
		DebugOutput	= 0x4,
		NoXvfbUse	= 0x8
	};

	Q_DECLARE_FLAGS(ProcessArguments, ProcessArgument)

private:
	Ui::MainWindow m_ui;

	ProcessArguments m_flags;

	long m_runCounter;
	long m_testCounter;

	unsigned long m_totalTests;
	unsigned long m_totalTestsJS;
	unsigned long m_totalTestsDOMTS;

	QUrl m_khtmlUrl;
	QUrl m_testsUrl;
	QUrl m_outputUrl;
	QUrl m_saveLogUrl;

	// Temporary variables
	TestResult m_lastResult;
	QString m_lastName;

	// Status pixmaps
	QPixmap m_failPixmap;
	QPixmap m_failKnownPixmap;
	QPixmap m_passPixmap;
	QPixmap m_passUnexpectedPixmap;
	QPixmap m_crashPixmap;
	QPixmap m_ignorePixmap;
	QPixmap m_noBaselinePixmap;

	KHTMLPart *m_browserPart;
	QProcess *m_activeProcess;
	QTreeWidgetItem *m_activeTreeItem;

	// Caches
	QMap<QString, QTreeWidgetItem *> m_itemMap;

	QMap<QString, QStringList> m_ignoreMap;
	QMap<QString, QStringList> m_failureMap;
	QMap<QString, QStringList> m_directoryMap;

	bool m_suspended;

	// Processing queue
	bool m_justProcessingQueue;
	QQueue<QString> m_processingQueue;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TestRegressionWindow::ProcessArguments)

#endif

// vim:ts=4:tw=4:noet
