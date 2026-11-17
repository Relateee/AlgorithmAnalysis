#pragma once

#include <optional>
#include <string>

// Opens the native "Open file" dialog filtered to images. Returns the chosen path,
// or nothing if the user cancelled (or on non-Windows platforms).
std::optional<std::string> openImageFileDialog();
