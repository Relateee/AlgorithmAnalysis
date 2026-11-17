#include "FileDialog.h"

#ifdef _WIN32
// Kept in its own file so <windows.h> macros (min, max, ...) don't leak into other code.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

std::optional<std::string> openImageFileDialog() {
    char file[MAX_PATH] = "";

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0"
                      "All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = "Open image";
    // NOCHANGEDIR: don't let the dialog change the process working directory.
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
        return std::string(file);
    return std::nullopt;
}

#else

std::optional<std::string> openImageFileDialog() {
    return std::nullopt;
}

#endif
