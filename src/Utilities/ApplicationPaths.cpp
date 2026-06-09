#include "Utilities/ApplicationPaths.h"

#include "AppVersion.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

namespace
{
QString ensureDirectory(const QString& path)
{
    QDir directory(path);
    directory.mkpath(".");
    return directory.absolutePath();
}

QString productDirectoryName()
{
    QString directoryName = QString::fromUtf8(AppVersion::kDisplayName).trimmed();
    if (directoryName.isEmpty())
    {
        directoryName = QString::fromUtf8(AppVersion::kProductName).trimmed();
    }
    if (directoryName.isEmpty())
    {
        directoryName = QStringLiteral("Cross Axial Dicom Viewer");
    }

    static const QRegularExpression invalidPathCharacters(QStringLiteral(R"([<>:"/\\|?*])"));
    directoryName.replace(invalidPathCharacters, QStringLiteral("_"));
    return directoryName;
}

QString documentsDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (path.trimmed().isEmpty())
    {
        path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    if (path.trimmed().isEmpty())
    {
        path = QCoreApplication::applicationDirPath();
    }

    return ensureDirectory(path);
}

QString subdirectory(const QString& name)
{
    return ensureDirectory(QDir(ApplicationPaths::applicationRootDirectory()).filePath(name));
}
}

QString ApplicationPaths::applicationRootDirectory()
{
    return ensureDirectory(QDir(documentsDirectory()).filePath(productDirectoryName()));
}

QString ApplicationPaths::dataDirectory()
{
    return subdirectory(QStringLiteral("data"));
}

QString ApplicationPaths::databaseFilePath()
{
    return QDir(dataDirectory()).filePath(QStringLiteral("crossaxial.sqlite"));
}

QString ApplicationPaths::auditDirectory()
{
    return subdirectory(QStringLiteral("audit"));
}

QString ApplicationPaths::auditFilePath()
{
    return QDir(auditDirectory()).filePath(QStringLiteral("audit.jsonl"));
}

QString ApplicationPaths::diagnosticsDirectory()
{
    return subdirectory(QStringLiteral("diagnostics"));
}

QString ApplicationPaths::applicationLogFilePath()
{
    return QDir(diagnosticsDirectory()).filePath(QStringLiteral("application.log"));
}

QString ApplicationPaths::crashReportDirectory()
{
    return subdirectory(QStringLiteral("diagnostics/crash-reports"));
}

QString ApplicationPaths::configDirectory()
{
    return subdirectory(QStringLiteral("config"));
}

QString ApplicationPaths::configFilePath()
{
    return QDir(configDirectory()).filePath(QStringLiteral("config.ini"));
}
