#ifndef UI_THEMES_H
#define UI_THEMES_H

#include <TFT_eSPI.h> // For uint16_t color definitions

// Enum for available themes
enum class Theme {
    DARK,
    LIGHT
};

// Structure to hold all theme-specific colors
struct ThemeColors {
    uint16_t background_dark;
    uint16_t background_light;
    uint16_t accent_primary;
    uint16_t accent_secondary;
    uint16_t text_primary;
    uint16_t text_secondary;
    uint16_t shadow_color;
    uint16_t error_color;
};

// Define color sets for each theme
const ThemeColors DARK_THEME_COLORS = {
    .background_dark    = 0x0841, // Dark grey (from previous revert)
    .background_light   = 0x18E3, // Slightly lighter blue-grey (from previous revert)
    .accent_primary     = 0x04F0, // Green
    .accent_secondary   = 0xFD20, // Orange
    .text_primary       = TFT_WHITE,
    .text_secondary     = 0xAD55, // Light grey
    .shadow_color       = TFT_BLACK,
    .error_color        = TFT_RED
};

const ThemeColors LIGHT_THEME_COLORS = {
    .background_dark    = TFT_WHITE,
    .background_light   = 0xF7BE, // Light grey-blue
    .accent_primary     = 0x04F0, // Green (can be adjusted for light theme)
    .accent_secondary   = 0xFD20, // Orange (can be adjusted for light theme)
    .text_primary       = TFT_BLACK,
    .text_secondary     = 0x4A69, // Dark grey
    .shadow_color       = 0xCE79, // Light grey for shadows on light background
    .error_color        = TFT_RED
};

#endif // UI_THEMES_H
