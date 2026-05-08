#pragma once

#include "Audit/IAuditSink.h"

#include <QFile>
#include <QString>

/**
 * @brief Append-only JSONL audit sink.
 *
 * Responsibilities:
 * - Serialize audit events as one JSON object per line.
 * - Keep a simple, inspectable local audit trail for development and early QMS
 *   traceability work.
 *
 * Assumptions:
 * - This sink is a practical foundation, not a complete regulated audit trail
 *   with tamper-evidence or access control.
 */
class JsonlAuditSink final : public IAuditSink
{
public:
    /**
     * @brief Creates a JSONL sink for a local file.
     * @param filePath Destination audit log path.
     * @param maxAttributeLength Maximum stored length for attribute values.
     */
    explicit JsonlAuditSink(QString filePath, qsizetype maxAttributeLength = 2048);

    /**
     * @brief Appends one event to the JSONL log.
     * @param event Event to serialize.
     */
    void record(const AuditEvent& event) override;

private:
    /** @brief Reduces unbounded or unsafe text before serialization. */
    QString sanitizeValue(const QString& value) const;
    /** @brief Converts severity enum values to stable log strings. */
    static QString severityName(AuditSeverity severity);
    /** @brief Converts event type enum values to stable log strings. */
    static QString typeName(AuditEventType type);

private:
    QString m_filePath;
    qsizetype m_maxAttributeLength{2048};
};
