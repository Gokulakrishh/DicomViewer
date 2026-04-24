#include "ViewportSession.h"

void ViewportSession::clear()
{
    currentSeries.reset();
    singleImage.reset();
    currentImageIndex = -1;
    currentWindowLevel = 0;
    currentWindowWidth = 100;
    currentPreset = ViewportWindowPreset::Custom;
    windowStateInitialized = false;
    cinePlaying = false;
}
