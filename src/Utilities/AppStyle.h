#pragma once

class QApplication;

/**
 * @brief Applies global Qt widget styling for the application.
 */
class AppStyle
{
public:
    /**
     * @brief Applies the application stylesheet/palette.
     * @param app Qt application instance.
     */
    static void apply(QApplication& app);
};
