#pragma once

#include "core/GameInfo.h"
#include "core/Package.h"

#include <QObject>
#include <QPointer>
#include <QTemporaryDir>

#include <memory>

class QProcess;

// "Report a problem": a zip on the Desktop (KytyPS5-report-<date>.zip) with the newest logs (the last 5 runs, and the
// last precompile's own logs), launcher-settings.json, system.txt (this PC, the game, the shaders, the optional files)
// and launch.txt (the last play command and switches).
class ProblemReport final: public QObject {
	Q_OBJECT

public:
	explicit ProblemReport(QObject* parent = nullptr);
	~ProblemReport() override;

	// `system`: system.txt; `launch`: launch.txt; `launcherLog`: the launcher's own log panel.
	void create(const Package& package, const QString& system, const QString& launch, const QString& launcherLog);
	[[nodiscard]] bool busy() const { return m_process != nullptr; }

	// system.txt for this PC, game and shader state.
	static QString systemText(const Package& package, const GameInfo& game, const QString& shaderState);

signals:
	void finished(bool ok, const QString& zipPath, const QString& error);

private:
	std::unique_ptr<QTemporaryDir> m_staging;
	QPointer<QProcess>             m_process;
	QString                        m_zip;
};
