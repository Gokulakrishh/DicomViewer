#pragma once

#include <QDialog>

class QLabel;
class QPushButton;

/**
 * @brief Startup splash / regulatory disclaimer dialog shown before the main window.
 *
 * Purpose:
 * - Provide an explicit, user-acknowledged entry point for intended use / regulatory
 *   disclaimers (e.g., non-diagnostic, research-only, etc.).
 *
 * Responsibilities:
 * - Present application branding and disclaimer text.
 * - Require an explicit "OK" acknowledgement before continuing to the main UI.
 *
 * Assumptions:
 * - The detailed regulatory wording may be revised before controlled release.
 * - This dialog is short-lived and does not own any long-lived application state.
 */
class RegulatorySplashDialog final : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Creates the splash/disclaimer dialog.
     * @param parent Optional Qt parent widget.
     */
    explicit RegulatorySplashDialog(QWidget* parent = nullptr);

private:
    void buildUi();
    void updateBranding();
    void updateText();

    QLabel* m_logoLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_bodyLabel = nullptr;
    QPushButton* m_okButton = nullptr;
};
