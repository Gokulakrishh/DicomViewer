#include "Utilities/CrashReportService.h"

#include "AppVersion.h"
#include "Utilities/ApplicationPaths.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageLogContext>
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
#if defined(__has_include)
#if __has_include(<dbghelp.h>)
#define CROSSAXIAL_HAS_DBGHELP 1
#include <dbghelp.h>
#else
#define CROSSAXIAL_HAS_DBGHELP 0
#endif
#else
#define CROSSAXIAL_HAS_DBGHELP 1
#include <dbghelp.h>
#endif
#else
#include <execinfo.h>
#include <unistd.h>
#endif

namespace
{
constexpr qsizetype kMaxStoredPathLength = 2048;

QtMessageHandler g_previousQtMessageHandler = nullptr;
std::mutex g_logMutex;
std::atomic_bool g_crashReportInProgress{false};
std::atomic_bool g_consoleLoggingEnabled{false};
QString g_applicationLogPath;
QString g_crashReportDirectory;
std::array<char, kMaxStoredPathLength> g_crashDirectoryBytes{};
std::array<char, kMaxStoredPathLength> g_applicationLogPathBytes{};

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

bool environmentFlagEnabled(const char* name)
{
    const char* value = std::getenv(name);
    if (!value)
    {
        return false;
    }

    const QByteArray normalized = QByteArray(value).trimmed().toLower();
    return !normalized.isEmpty() &&
           normalized != "0" &&
           normalized != "false" &&
           normalized != "no" &&
           normalized != "off";
}

bool shouldForwardToConsole(QtMsgType type)
{
    return g_consoleLoggingEnabled.load(std::memory_order_relaxed) ||
           type == QtCriticalMsg ||
           type == QtFatalMsg;
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

    if (shouldForwardToConsole(type) && g_previousQtMessageHandler)
    {
        g_previousQtMessageHandler(type, context, message);
    }
    else if (shouldForwardToConsole(type))
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
#if defined(Q_OS_WIN) && defined(_MSC_VER)
    std::tm utcTime{};
    gmtime_s(&utcTime, &now);
    std::strftime(buffer, bufferSize, "%Y%m%d-%H%M%S", &utcTime);
#else
    std::tm utcTime{};
    gmtime_r(&now, &utcTime);
    std::strftime(buffer, bufferSize, "%Y%m%d-%H%M%S", &utcTime);
#endif
}

[[noreturn]] void exitImmediately(int code)
{
#if defined(Q_OS_WIN)
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));
    std::abort();
#else
    std::_Exit(code);
#endif
}

#if defined(Q_OS_WIN)
const char* windowsExceptionName(unsigned long exceptionCode)
{
    switch (exceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        return "access_violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "array_bounds_exceeded";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "datatype_misalignment";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "float_divide_by_zero";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "illegal_instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "integer_divide_by_zero";
    case EXCEPTION_IN_PAGE_ERROR:
        return "in_page_error";
    case EXCEPTION_STACK_OVERFLOW:
        return "stack_overflow";
    default:
        return "unknown";
    }
}

void modulePathForAddress(void* address, char* buffer, std::size_t bufferSize)
{
    if (!address || !buffer || bufferSize == 0)
    {
        return;
    }

    HMODULE moduleHandle = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(address),
            &moduleHandle))
    {
        return;
    }

    GetModuleFileNameA(moduleHandle, buffer, static_cast<DWORD>(bufferSize));
}

bool writeWindowsMiniDump(EXCEPTION_POINTERS* exceptionPointers, const char* timestamp, char* dumpPath, std::size_t dumpPathSize)
{
    if (!exceptionPointers || !timestamp || !dumpPath || dumpPathSize == 0)
    {
        return false;
    }

#if CROSSAXIAL_HAS_DBGHELP
    std::snprintf(
        dumpPath,
        dumpPathSize,
        "%s\\crash-%s-%lu.dmp",
        g_crashDirectoryBytes.data(),
        timestamp,
        currentProcessId());

    HANDLE dumpFile = CreateFileA(dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile == INVALID_HANDLE_VALUE)
    {
        dumpPath[0] = '\0';
        return false;
    }

    HMODULE dbgHelpModule = LoadLibraryA("dbghelp.dll");
    if (!dbgHelpModule)
    {
        CloseHandle(dumpFile);
        DeleteFileA(dumpPath);
        dumpPath[0] = '\0';
        return false;
    }

    using MiniDumpWriteDumpFunction = BOOL(WINAPI*)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION);

    const auto miniDumpWriteDump = reinterpret_cast<MiniDumpWriteDumpFunction>(
        GetProcAddress(dbgHelpModule, "MiniDumpWriteDump"));
    if (!miniDumpWriteDump)
    {
        FreeLibrary(dbgHelpModule);
        CloseHandle(dumpFile);
        DeleteFileA(dumpPath);
        dumpPath[0] = '\0';
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = exceptionPointers;
    exceptionInfo.ClientPointers = FALSE;

    const BOOL success = miniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        dumpFile,
        MiniDumpNormal,
        &exceptionInfo,
        nullptr,
        nullptr);

    FreeLibrary(dbgHelpModule);
    CloseHandle(dumpFile);
    if (!success)
    {
        DeleteFileA(dumpPath);
        dumpPath[0] = '\0';
        return false;
    }

    return true;
#else
    dumpPath[0] = '\0';
    return false;
#endif
}
#endif

#if !defined(Q_OS_WIN)
void writeLinuxBacktrace(std::FILE* file)
{
    if (!file)
    {
        return;
    }

    void* frames[64]{};
    const int frameCount = ::backtrace(frames, static_cast<int>(std::size(frames)));
    std::fprintf(file, "\nBacktrace:\n");
    if (frameCount <= 0)
    {
        std::fprintf(file, "unavailable\n");
        return;
    }

    ::backtrace_symbols_fd(frames, frameCount, fileno(file));
}
#endif

void writeCrashReport(
    const char* reason,
    const char* detail
#if defined(Q_OS_WIN)
    ,
    EXCEPTION_POINTERS* exceptionPointers = nullptr
#endif
)
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

#if defined(Q_OS_WIN)
    char minidumpPath[kMaxStoredPathLength]{};
    const bool minidumpWritten = writeWindowsMiniDump(exceptionPointers, timestamp, minidumpPath, sizeof(minidumpPath));
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
#if defined(Q_OS_WIN)
    if (exceptionPointers && exceptionPointers->ExceptionRecord)
    {
        char modulePath[kMaxStoredPathLength]{};
        modulePathForAddress(
            exceptionPointers->ExceptionRecord->ExceptionAddress,
            modulePath,
            sizeof(modulePath));
        if (modulePath[0] != '\0')
        {
            std::fprintf(file, "Fault module: %s\n", modulePath);
        }
    }
    if (minidumpWritten)
    {
        std::fprintf(file, "Minidump: %s\n", minidumpPath);
    }
    else if (exceptionPointers)
    {
        std::fprintf(file, "Minidump: unavailable\n");
    }
#endif
    std::fprintf(file, "Application log: %s\n", g_applicationLogPathBytes.data());
#if !defined(Q_OS_WIN)
    writeLinuxBacktrace(file);
#endif
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
    exitImmediately(128 + signalNumber);
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
    exitImmediately(EXIT_FAILURE);
}

#if defined(Q_OS_WIN)
LONG WINAPI windowsUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    char detail[512]{};
    if (exceptionPointers && exceptionPointers->ExceptionRecord)
    {
        const unsigned long exceptionCode = static_cast<unsigned long>(exceptionPointers->ExceptionRecord->ExceptionCode);
        std::snprintf(
            detail,
            sizeof(detail),
            "exception=%s exception_code=0x%08lx exception_address=%p",
            windowsExceptionName(exceptionCode),
            exceptionCode,
            exceptionPointers->ExceptionRecord->ExceptionAddress);
    }
    else
    {
        std::snprintf(detail, sizeof(detail), "exception_record=unavailable");
    }

    writeCrashReport("windows unhandled exception", detail, exceptionPointers);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif
}

void CrashReportService::install()
{
    g_crashReportDirectory = ApplicationPaths::crashReportDirectory();
    g_applicationLogPath = ApplicationPaths::applicationLogFilePath();
    g_consoleLoggingEnabled.store(
        environmentFlagEnabled("DICOMVIEWER_CONSOLE_LOGGING"),
        std::memory_order_relaxed);

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
    ULONG stackGuaranteeBytes = 64 * 1024;
    SetThreadStackGuarantee(&stackGuaranteeBytes);
    SetUnhandledExceptionFilter(windowsUnhandledExceptionFilter);
#endif

    qInfo() << "Crash reports:" << g_crashReportDirectory;
    qInfo() << "Application log:" << g_applicationLogPath;
}

QString CrashReportService::crashReportDirectory()
{
    if (g_crashReportDirectory.isEmpty())
    {
        return ApplicationPaths::crashReportDirectory();
    }

    return g_crashReportDirectory;
}

QString CrashReportService::applicationLogPath()
{
    if (g_applicationLogPath.isEmpty())
    {
        return ApplicationPaths::applicationLogFilePath();
    }

    return g_applicationLogPath;
}
