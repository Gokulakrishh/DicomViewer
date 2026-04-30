#include "ViewportSession.h"

void ViewportSession::clear()
{
    currentSeries.reset();
    singleImage.reset();
    currentImageIndex = -1;
    currentWindowLevel = 0;
    currentWindowWidth = 100;
    currentPreset = ViewportWindowPreset::Custom;
    currentDicomWindowPresetIndex = -1;
    windowStateInitialized = false;
    cinePlaying = false;
}
