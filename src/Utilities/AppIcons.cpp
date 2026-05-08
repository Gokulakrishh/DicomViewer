#include "Utilities/AppIcons.h"

QIcon AppIcons::applicationIcon()
{
    QIcon icon;
    icon.addFile(":/icons/icon_16.png", QSize(16, 16));
    icon.addFile(":/icons/icon_32.png", QSize(32, 32));
    icon.addFile(":/icons/icon_64.png", QSize(64, 64));
    icon.addFile(":/icons/icon_128.png", QSize(128, 128));
    icon.addFile(":/icons/icon_256.png", QSize(256, 256));
    icon.addFile(":/icons/icon_512.png", QSize(512, 512));
    return icon;
}

QPixmap AppIcons::logoPixmap(const QSize& targetSize)
{
    const QString resourcePath =
        targetSize.width() > 256 || targetSize.height() > 256
            ? QStringLiteral(":/icons/logo_512.png")
            : QStringLiteral(":/icons/logo_256.png");
    const QPixmap logo(resourcePath);
    if (logo.isNull() || targetSize.isEmpty()) {
        return {};
    }

    return logo.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
