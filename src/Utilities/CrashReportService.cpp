#include "Utilities/CrashReportService.h"

#include "AppVersion.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageLogContext>
#include <QStandardPaths>
#include <QTextStream>

#include <array>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <mutex>
#include <signal.h>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace
{
constexpr qsizetype kMaxStoredPathLength = 2048;

QtMessageHandler g_previousQtMessageHandler = nullptr;
std::mutex g_logMutex;
std::atomic_bool g_crashReportInProgress{false};
QString g_applicationLogPath;
QString g_crashReportDirectory;
std::array<char, kMaxStoredPathLength> g_crashDirectoryBytes{};
std::array<char, kMaxStoredPathLength> g_applicationLogPathBytes{};

QString applicationDataRoot()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.trimmed().isEmpty())
    {
        appDataPath = QDir(QCoreApplication::applicationDirPath()).filePath("diagnostics");
    }

    QDir().mkpath(appDataPath);
    return appDataPath;
}

void storePathForCrashHandler(const QString& path, std::array<char, kMaxStoredPathLength>& target)
{
    target.fill('\0');
    const QByteArray localPath = QDir::toNativeSeparators(path).toLocal8Bit();
    const qsizetype copyLength = std::min(localPath.size(), kMaxStoredPathLength - 1);
    std::memcpy(target.data(), localPath.constData(), static_cast<std::size_t>(copyLength));
}

const char* messageTypeName(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:
        return "debug";
    case QtInfoMsg:
        return "info";
    case QtWarningMsg:
        return "warning";
    case QtCriticalMsg:
        return "critical";
    case QtFatalMsg:
        return "fatal";
    }

    return "unknown";
}

void appendToApplicationLog(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (g_applicationLogPath.isEmpty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(g_logMutex);

    QFile file(g_applicationLogPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return;
    }

    QTextStream stream(&file);
    stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
           << "Z [" << messageTypeName(type) << "] " << message;

    if (context.file)
    {
        stream << " | " << context.file << ':' << context.line;
    }
    if (context.function)
    {
        stream << " | " << context.function;
    }
    if (context.category)
    {
        stream << " | category=" << context.category;
    }

    stream << '\n';
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    appendToApplicationLog(type, context, message);

    if (g_previousQtMessageHandler)
    {
        g_previousQtMessageHandler(type, context, message);
    }
    else
    {
        const QByteArray localMessage = message.toLocal8Bit();
        std::fprintf(stderr, "[%s] %s\n", messageTypeName(type), localMessage.constData());
        std::fflush(stderr);
    }

    if (type == QtFatalMsg)
    {
        std::abort();
    }
}

unsigned long currentProcessId()
{
#if defined(Q_OS_WIN)
    return static_cast<unsigned long>(GetCurrentProcessId());
#else
    return static_cast<unsigned long>(getpid());
#endif
}

void utcTimestamp(char* buffer, std::size_t bufferSize)
{
    const std::time_t now = std::time(nullptr);
#if defined(Q_OS_WIN)
    std::tm utcTime{};
    gmtime_s(&utcTime, &now);
    std::strftime(buffer, bufferSize, "%Y%m%d-%H%M%S", &utcTime);
#else
    std::tm utcTime{};
    gmtime_r(&now, &utcTime);
    std::strftime(buffer, bufferSize, "%Y%m%d-%H%M%S", &utcTime);
#endif
}

void writeCrashReport(const char* reason, const char* detail)
{
    bool expected = false;
    if (!g_crashReportInProgress.compare_exchange_strong(expected, true))
    {
        return;
    }

    char timestamp[32]{};
    utcTimestamp(timestamp, sizeof(timestamp));

    char path[kMaxStoredPathLength]{};
#if defined(Q_OS_WIN)
    std::snprintf(
        path,
        sizeof(path),
        "%s\\crash-%s-%lu.log",
        g_crashDirectoryBytes.data(),
        timestamp,
        currentProcessId());
#else
    std::snprintf(
        path,
        sizeof(path),
        "%s/crash-%s-%lu.log",
        g_crashDirectoryBytes.data(),
        timestamp,
        currentProcessId());
#endif

    std::FILE* file = std::fopen(path, "ab");
    if (!file)
    {
        return;
    }

    std::fprintf(file, "Product: %s\n", AppVersion::kDisplayName);
    std::fprintf(file, "Version: %s\n", AppVersion::kVersionString);
    std::fprintf(file, "Timestamp UTC: %s\n", timestamp);
    std::fprintf(file, "Process ID: %lu\n", currentProcessId());
    std::fprintf(file, "Reason: %s\n", reason ? reason : "unknown");
    if (detail && detail[0] != '\0')
    {
        std::fprintf(file, "Detail: %s\n", detail);
    }
    std::fprintf(file, "Application log: %s\n", g_applicationLogPathBytes.data());
    std::fprintf(file, "\nNotes:\n");
    std::fprintf(file, "- This is a best-effort local crash report.\n");
    std::fprintf(file, "- Check the application log for messages immediately before the crash.\n");
    std::fprintf(file, "- Windows Event Viewer may contain the faulting module and exception code.\n");
    std::fclose(file);
}

void signalHandler(int signalNumber)
{
    char detail[64]{};
    std::snprintf(detail, sizeof(detail), "signal=%d", signalNumber);
    writeCrashReport("fatal signal", detail);
    std::_Exit(128 + signalNumber);
}

[[noreturn]] void terminateHandler()
{
    const char* detail = "std::terminate called";
    if (const std::exception_ptr exception = std::current_exception())
    {
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const std::exception& error)
        {
            detail = error.what();
        }
        catch (...)
        {
            detail = "non-standard exception";
        }
    }

    writeCrashReport("terminate", detail);
    std::_Exit(EXIT_FAILURE);
}

#if defined(Q_OS_WIN)
LONG WINAPI windowsUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    char detail[256]{};
    if (exceptionPointers && exceptionPointers->ExceptionRecord)
    {
        std::snprintf(
            detail,
            sizeof(detail),
            "exception_code=0x%08lx exception_address=%p",
            static_cast<unsigned long>(exceptionPointers->ExceptionRecord->ExceptionCode),
            exceptionPointers->ExceptionRecord->ExceptionAddress);
    }
    else
    {
        std::snprintf(detail, sizeof(detail), "exception_record=unavailable");
    }

    writeCrashReport("windows unhandled exception", detail);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
}

void CrashReportService::install()
{
    const QString root = applicationDataRoot();
    g_crashReportDirectory = QDir(root).filePath("crash-reports");
    QDir().mkpath(g_crashReportDirectory);
    g_applicationLogPath = QDir(root).filePath("application.log");

    storePathForCrashHandler(g_crashReportDirectory, g_crashDirectoryBytes);
    storePathForCrashHandler(g_applicationLogPath, g_applicationLogPathBytes);

    g_previousQtMessageHandler = qInstallMessageHandler(qtMessageHandler);
    std::set_terminate(terminateHandler);
    ::signal(SIGABRT, signalHandler);
    ::signal(SIGFPE, signalHandler);
    ::signal(SIGILL, signalHandler);
    ::signal(SIGSEGV, signalHandler);

#if defined(SIGBUS)
    ::signal(SIGBUS, signalHandler);
#endif
#if defined(Q_OS_WIN)
    SetUnhandledExceptionFilter(windowsUnhandledExceptionFilter);
#endif

    qInfo() << "Crash reports:" << g_crashReportDirectory;
    qInfo() << "Application log:" << g_applicationLogPath;
}

QString CrashReportService::crashReportDirectory()
{
    if (g_crashReportDirectory.isEmpty())
    {
        return QDir(applicationDataRoot()).filePath("crash-reports");
    }

    return g_crashReportDirectory;
}

QString CrashReportService::applicationLogPath()
{
    if (g_applicationLogPath.isEmpty())
    {
        return QDir(applicationDataRoot()).filePath("application.log");
    }

    return g_applicationLogPath;
}
