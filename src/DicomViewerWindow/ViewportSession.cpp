#include "ViewportSession.h"

void ViewportSession::clear()
{
    currentSeries.reset();
    singleImage.reset();
    currentImageIndex = -1;
    currentWindowLevel = 0;
    currentWindowWidth = 100;
    currentPresetIndex = 0;
    currentAutoWindowPresetIndex = 0;
    windowStateInitialized = false;
    toolIndex = 0;
    cinePlaying = false;
}
