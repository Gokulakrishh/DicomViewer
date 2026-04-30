#include "ViewerTools/Measurements/MeasurementService.h"

#include <QUuid>
#include <algorithm>
#include <QtGlobal>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;

const QVector<QColor>& measurementPalette()
{
    static const QVector<QColor> colors{
        QColor(255, 214, 10),
        QColor(0, 200, 255),
        QColor(255, 90, 95),
        QColor(80, 220, 120),
        QColor(190, 120, 255),
        QColor(255, 145, 0),
        QColor(0, 230, 190),
        QColor(255, 110, 190),
        QColor(165, 220, 60),
        QColor(110, 170, 255)};
    return colors;
}
}

const QVector<MeasurementAnnotation>& MeasurementService::measurements() const
{
    return m_measurements;
}

std::optional<MeasurementAnnotation> MeasurementService::activeMeasurement() const
{
    if (!m_activeMeasurement)
    {
        return std::nullopt;
    }

    return previewAnnotation(*m_activeMeasurement, m_previewPoint);
}

bool MeasurementService::canCreateMeasurement() const
{
    return m_measurements.size() < kMaxMeasurementsPerSeries;
}

int MeasurementService::measurementCount() const
{
    return m_measurements.size();
}

MeasurementAnnotation MeasurementService::previewAnnotation(
    const MeasurementAnnotation& annotation,
    const std::optional<MeasurementPoint>& previewPoint)
{
    MeasurementAnnotation preview = annotation;
    if (previewPoint)
    {
        switch (preview.type)
        {
        case MeasurementType::Distance:
            if (preview.points.size() == 1)
            {
                preview.points.append(*previewPoint);
            }
            break;
        case MeasurementType::Polyline:
            preview.points.append(*previewPoint);
            break;
        case MeasurementType::Angle:
            if (preview.points.size() <= 2)
            {
                preview.points.append(*previewPoint);
            }
            break;
        case MeasurementType::RectangleRoi:
            if (preview.points.size() == 1)
            {
                preview.points.append(*previewPoint);
            }
            break;
        }
    }

    if (preview.type == MeasurementType::Distance || preview.type == MeasurementType::Polyline)
    {
        preview.lengthMm = totalLengthMm(preview.points);
    }
    return preview;
}

void MeasurementService::clear()
{
    m_measurements.clear();
    m_activeMeasurement.reset();
    m_previewPoint.reset();
}

void MeasurementService::setMeasurements(const QVector<MeasurementAnnotation>& measurements)
{
    m_measurements = measurements;
    m_activeMeasurement.reset();
    m_previewPoint.reset();
}

void MeasurementService::cancelActiveMeasurement()
{
    m_activeMeasurement.reset();
    m_previewPoint.reset();
}

bool MeasurementService::beginDistance(const MeasurementPoint& point)
{
    if (!canCreateMeasurement())
    {
        return false;
    }

    m_activeMeasurement = MeasurementAnnotation{
        nextId(),
        MeasurementType::Distance,
        QVector<MeasurementPoint>{point},
        nextColor(),
        0.0};
    m_previewPoint.reset();
    return true;
}

void MeasurementService::updateDistancePreview(const MeasurementPoint& point)
{
    if (m_activeMeasurement && m_activeMeasurement->type == MeasurementType::Distance)
    {
        m_previewPoint = point;
    }
}

bool MeasurementService::completeDistance(const MeasurementPoint& point)
{
    if (!m_activeMeasurement || m_activeMeasurement->type != MeasurementType::Distance)
    {
        return false;
    }

    if (m_activeMeasurement->points.size() == 1)
    {
        m_activeMeasurement->points.append(point);
    }
    m_previewPoint.reset();
    return commitActiveMeasurement();
}

bool MeasurementService::beginPolyline(const MeasurementPoint& point)
{
    if (!canCreateMeasurement())
    {
        return false;
    }

    m_activeMeasurement = MeasurementAnnotation{
        nextId(),
        MeasurementType::Polyline,
        QVector<MeasurementPoint>{point},
        nextColor(),
        0.0};
    m_previewPoint.reset();
    return true;
}

bool MeasurementService::appendPolylinePoint(const MeasurementPoint& point)
{
    if (!m_activeMeasurement || m_activeMeasurement->type != MeasurementType::Polyline)
    {
        return beginPolyline(point);
    }

    m_activeMeasurement->points.append(point);
    m_previewPoint.reset();
    return true;
}

void MeasurementService::updatePolylinePreview(const MeasurementPoint& point)
{
    if (m_activeMeasurement && m_activeMeasurement->type == MeasurementType::Polyline)
    {
        m_previewPoint = point;
    }
}

bool MeasurementService::completePolyline()
{
    if (!m_activeMeasurement || m_activeMeasurement->type != MeasurementType::Polyline)
    {
        return false;
    }

    m_previewPoint.reset();
    if (m_activeMeasurement->points.size() < 2)
    {
        cancelActiveMeasurement();
        return false;
    }

    return commitActiveMeasurement();
}

bool MeasurementService::beginAngle(const MeasurementPoint& point)
{
    if (!canCreateMeasurement())
    {
        return false;
    }

    m_activeMeasurement = MeasurementAnnotation{
        nextId(),
        MeasurementType::Angle,
        QVector<MeasurementPoint>{point},
        nextColor(),
        0.0};
    m_previewPoint.reset();
    return true;
}

bool MeasurementService::appendAnglePoint(const MeasurementPoint& point)
{
    if (!m_activeMeasurement || m_activeMeasurement->type != MeasurementType::Angle)
    {
        return beginAngle(point);
    }

    if (m_activeMeasurement->points.size() >= 3)
    {
        return false;
    }

    m_activeMeasurement->points.append(point);
    m_previewPoint.reset();
    if (m_activeMeasurement->points.size() == 3)
    {
        return commitActiveMeasurement();
    }
    return true;
}

void MeasurementService::updateAnglePreview(const MeasurementPoint& point)
{
    if (m_activeMeasurement && m_activeMeasurement->type == MeasurementType::Angle)
    {
        m_previewPoint = point;
    }
}

bool MeasurementService::beginRectangleRoi(const MeasurementPoint& point)
{
    if (!canCreateMeasurement())
    {
        return false;
    }

    m_activeMeasurement = MeasurementAnnotation{
        nextId(),
        MeasurementType::RectangleRoi,
        QVector<MeasurementPoint>{point},
        nextColor(),
        0.0};
    m_previewPoint.reset();
    return true;
}

void MeasurementService::updateRectangleRoiPreview(const MeasurementPoint& point)
{
    if (m_activeMeasurement && m_activeMeasurement->type == MeasurementType::RectangleRoi)
    {
        m_previewPoint = point;
    }
}

bool MeasurementService::completeRectangleRoi(const MeasurementPoint& point)
{
    if (!m_activeMeasurement || m_activeMeasurement->type != MeasurementType::RectangleRoi)
    {
        return false;
    }

    if (m_activeMeasurement->points.size() == 1)
    {
        m_activeMeasurement->points.append(point);
    }
    m_previewPoint.reset();

    if (m_activeMeasurement->points.size() < 2)
    {
        cancelActiveMeasurement();
        return false;
    }

    return commitActiveMeasurement();
}

QColor MeasurementService::nextColor() const
{
    const auto& colors = measurementPalette();
    if (colors.isEmpty())
    {
        return Qt::yellow;
    }
    return colors[m_measurements.size() % colors.size()];
}

QString MeasurementService::nextId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

double MeasurementService::totalLengthMm(const QVector<MeasurementPoint>& points)
{
    if (points.size() < 2)
    {
        return 0.0;
    }

    double length = 0.0;
    for (int index = 1; index < points.size(); ++index)
    {
        length += segmentLengthMm(points[index - 1], points[index]);
    }
    return length;
}

double MeasurementService::segmentLengthMm(const MeasurementPoint& first, const MeasurementPoint& second)
{
    const double dx = second.x - first.x;
    const double dy = second.y - first.y;
    const double dz = second.z - first.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

QString MeasurementService::formattedLength(double lengthMm)
{
    if (lengthMm >= 100.0)
    {
        return QString("%1 cm").arg(lengthMm / 10.0, 0, 'f', 1);
    }
    return QString("%1 mm").arg(lengthMm, 0, 'f', 1);
}

double MeasurementService::angleDegrees(const QVector<MeasurementPoint>& points)
{
    if (points.size() < 3)
    {
        return 0.0;
    }

    const MeasurementPoint& first = points[0];
    const MeasurementPoint& vertex = points[1];
    const MeasurementPoint& third = points[2];

    const double ux = first.x - vertex.x;
    const double uy = first.y - vertex.y;
    const double uz = first.z - vertex.z;
    const double vx = third.x - vertex.x;
    const double vy = third.y - vertex.y;
    const double vz = third.z - vertex.z;

    const double uLength = std::sqrt((ux * ux) + (uy * uy) + (uz * uz));
    const double vLength = std::sqrt((vx * vx) + (vy * vy) + (vz * vz));
    if (uLength <= 1e-6 || vLength <= 1e-6)
    {
        return 0.0;
    }

    const double cosine = std::clamp(((ux * vx) + (uy * vy) + (uz * vz)) / (uLength * vLength), -1.0, 1.0);
    return std::acos(cosine) * 180.0 / kPi;
}

QString MeasurementService::formattedAngle(double angleDegrees)
{
    return QString("%1°").arg(angleDegrees, 0, 'f', 1);
}

QString MeasurementService::formattedArea(double areaMm2)
{
    if (areaMm2 >= 100.0)
    {
        return QString("%1 cm²").arg(areaMm2 / 100.0, 0, 'f', 2);
    }
    return QString("%1 mm²").arg(areaMm2, 0, 'f', 1);
}

bool MeasurementService::commitActiveMeasurement()
{
    if (!m_activeMeasurement || !canCreateMeasurement())
    {
        cancelActiveMeasurement();
        return false;
    }

    m_activeMeasurement->lengthMm = totalLengthMm(m_activeMeasurement->points);
    m_measurements.append(*m_activeMeasurement);
    cancelActiveMeasurement();
    return true;
}
