#pragma once

#include <QString>

/**
 * @brief Installs local diagnostic logging and best-effort crash reporting.
 *
 * Responsibilities:
 * - Write Qt diagnostic messages to a local application log.
 * - Create a timestamped crash report for fatal signals, terminate calls, and
 *   Windows structured exceptions where supported.
 * - Keep crash reporting local to the workstation and avoid network upload.
 *
 * Assumptions:
 * - Reports are engineering diagnostics for non-diagnostic demo and test builds.
 * - Crash handlers are best-effort only; severe process corruption can still
 *   prevent report creation.
 * - Reports may contain local file paths and should be handled as potentially
 *   sensitive when patient DICOM paths include identifying information.
 */
class CrashReportService
{
public:
    /**
     * @brief Installs Qt message logging and process-level crash handlers.
     *
     * This should be called once, after QApplication has been constructed and
     * product metadata has been set.
     */
    static void install();

    /**
     * @brief Returns the directory where crash report files are written.
     * @return Absolute writable crash-report directory, or a fallback path beside the application.
     */
    [[nodiscard]] static QString crashReportDirectory();

    /**
     * @brief Returns the application diagnostic log path.
     * @return Absolute path to the rolling text log used for Qt messages.
     */
    [[nodiscard]] static QString applicationLogPath();
};

