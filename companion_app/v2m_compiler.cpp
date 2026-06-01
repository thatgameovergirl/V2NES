#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t type;           // Magic number "BM" (0x4D42)
    uint32_t size;           // File size in bytes
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;         // Offset to pixel data
};

struct BMPInfoHeader {
    uint32_t size;           // Info Header size (usually 40)
    int32_t width;           // Image width
    int32_t height;          // Image height
    uint16_t planes;         // Number of color planes
    uint16_t bitsPerPixel;   // Bits per pixel (24 or 32)
    uint32_t compression;    // Compression type (0 = uncompressed)
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

// Helper to sanitize title (spaces to underscores, remove special chars)
std::string sanitizeTitle(const std::string& name) {
    std::string clean = "";
    for (char c : name) {
        if (c == ' ') {
            clean += '_';
        } else if (isalnum(c) || c == '_' || c == '-') {
            clean += c;
        }
    }
    return clean;
}

// Extract base filename without extension
std::string getBaseName(const std::string& path) {
    size_t lastSlash = path.find_last_of("\\/");
    std::string file = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    size_t lastDot = file.find_last_of('.');
    return (lastDot == std::string::npos) ? file : file.substr(0, lastDot);
}

// Convert uncompressed BMP (24-bit or 32-bit) to 1024x1024 RGBA8
bool processBMPToV2M(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream file(inputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open BMP image: " << inputPath << std::endl;
        return false;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (header.type != 0x4D42) {
        std::cerr << "[ERROR] Invalid BMP format. Only standard uncompressed 24-bit or 32-bit BMPs are supported: " << inputPath << std::endl;
        return false;
    }

    int width = infoHeader.width;
    int height = std::abs(infoHeader.height);
    int bpp = infoHeader.bitsPerPixel;
    bool flipY = (infoHeader.height > 0); // Positive height means bottom-up BMP

    if (bpp != 24 && bpp != 32) {
        std::cerr << "[ERROR] Unsupported bits per pixel (" << bpp << "). Only 24-bit and 32-bit uncompressed BMPs are supported." << std::endl;
        return false;
    }

    // Read pixel data
    file.seekg(header.offset, std::ios::beg);
    std::vector<unsigned char> srcRGBA(width * height * 4);

    int rowSize = ((width * bpp + 31) / 32) * 4; // Row size padded to multiple of 4 bytes
    std::vector<unsigned char> rowBuffer(rowSize);

    for (int y = 0; y < height; y++) {
        file.read(reinterpret_cast<char*>(rowBuffer.data()), rowSize);
        
        int destY = flipY ? (height - 1 - y) : y;
        for (int x = 0; x < width; x++) {
            int destIdx = (destY * width + x) * 4;
            if (bpp == 24) {
                int srcIdx = x * 3;
                srcRGBA[destIdx]     = rowBuffer[srcIdx + 2]; // R
                srcRGBA[destIdx + 1] = rowBuffer[srcIdx + 1]; // G
                srcRGBA[destIdx + 2] = rowBuffer[srcIdx];     // B
                srcRGBA[destIdx + 3] = 255;                  // Alpha (Opaque)
            } else if (bpp == 32) {
                int srcIdx = x * 4;
                srcRGBA[destIdx]     = rowBuffer[srcIdx + 2]; // R
                srcRGBA[destIdx + 1] = rowBuffer[srcIdx + 1]; // G
                srcRGBA[destIdx + 2] = rowBuffer[srcIdx];     // B
                srcRGBA[destIdx + 3] = 255;                  // Alpha (Opaque)
            }
        }
    }
    file.close();

    // 1. Auto-Border Cropping
    int minX = width;
    int maxX = 0;
    int minY = height;
    int maxY = 0;
    bool hasContent = false;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            unsigned char r = srcRGBA[idx];
            unsigned char g = srcRGBA[idx + 1];
            unsigned char b = srcRGBA[idx + 2];

            // A pixel has content if it's not solid black (R, G, B > 15)
            if (r > 15 || g > 15 || b > 15) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
                hasContent = true;
            }
        }
    }

    if (!hasContent) {
        minX = 0;
        maxX = width - 1;
        minY = 0;
        maxY = height - 1;
    }

    int activeW = (maxX - minX) + 1;
    int activeH = (maxY - minY) + 1;

    // 2. Crisp Nearest-Neighbor Resizing with Integer Downscaling
    int intScaleX = static_cast<int>(std::ceil(static_cast<double>(activeW) / 1024.0));
    int intScaleY = static_cast<int>(std::ceil(static_cast<double>(activeH) / 1024.0));

    int targetW = static_cast<int>(std::round(static_cast<double>(activeW) / intScaleX));
    int targetH = static_cast<int>(std::round(static_cast<double>(activeH) / intScaleY));

    std::vector<unsigned char> destRGBA(1024 * 1024 * 4, 0); // Initialize with black

    for (int ty = 0; ty < targetH; ty++) {
        int srcY = minY + ty * intScaleY;
        if (srcY >= height) srcY = height - 1;

        for (int tx = 0; tx < targetW; tx++) {
            int srcX = minX + tx * intScaleX;
            if (srcX >= width) srcX = width - 1;

            int srcIdx = (srcY * width + srcX) * 4;
            int destIdx = (ty * 1024 + tx) * 4;

            destRGBA[destIdx + 0] = srcRGBA[srcIdx + 0]; // R
            destRGBA[destIdx + 1] = srcRGBA[srcIdx + 1]; // G
            destRGBA[destIdx + 2] = srcRGBA[srcIdx + 2]; // B
            destRGBA[destIdx + 3] = 255;                 // A (Opaque)
        }
    }

    // 3. Write Opaque RGBA8 binary data with 20-byte header
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "[ERROR] Could not create V2M output file: " << outputPath << std::endl;
        return false;
    }

    char magic[4] = {'V', 'M', '\x01', '\0'};
    uint32_t origW = activeW;
    uint32_t origH = activeH;
    uint32_t tW = targetW;
    uint32_t tH = targetH;

    outFile.write(magic, 4);
    outFile.write(reinterpret_cast<const char*>(&origW), 4);
    outFile.write(reinterpret_cast<const char*>(&origH), 4);
    outFile.write(reinterpret_cast<const char*>(&tW), 4);
    outFile.write(reinterpret_cast<const char*>(&tH), 4);

    outFile.write(reinterpret_cast<const char*>(destRGBA.data()), destRGBA.size());
    outFile.close();

    std::cout << "[SUCCESS] Exported V2M Map: " << outputPath << " (1024x1024 RGBA8)" << std::endl;
    return true;
}

// Copy guide text file
bool copyGuideText(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream src(inputPath, std::ios::binary);
    if (!src.is_open()) {
        std::cerr << "[ERROR] Could not open guide text file: " << inputPath << std::endl;
        return false;
    }

    std::ofstream dest(outputPath, std::ios::binary);
    if (!dest.is_open()) {
        std::cerr << "[ERROR] Could not create guide output file: " << outputPath << std::endl;
        return false;
    }

    dest << src.rdbuf();
    src.close();
    dest.close();

    std::cout << "[SUCCESS] Exported Guide Walkthrough: " << outputPath << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================" << std::endl;
    std::cout << "       V2NES COMPANION COMPILER v1.0" << std::endl;
    std::cout << "=========================================" << std::endl;

    if (argc < 2) {
        std::cout << "Usage: v2m_compiler.exe <rom_file> <guide_file.txt> [map1.bmp] [map2.bmp] ..." << std::endl;
        std::cout << "Note: Put all files in any order. The program will automatically classify them." << std::endl;
        return 1;
    }

    std::string romPath = "";
    std::string guidePath = "";
    std::vector<std::string> mapPaths;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        size_t dot = arg.find_last_of('.');
        if (dot == std::string::npos) continue;
        std::string ext = arg.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == "nes" || ext == "fds" || ext == "unf") {
            romPath = arg;
        } else if (ext == "txt") {
            guidePath = arg;
        } else if (ext == "bmp") {
            mapPaths.push_back(arg);
        } else {
            std::cout << "[WARNING] Skipped unsupported file: " << arg << " (only .NES, .FDS, .TXT, and uncompressed .BMP are supported)" << std::endl;
        }
    }

    if (romPath.empty()) {
        std::cerr << "[ERROR] No ROM file (.NES or .FDS) was provided! The ROM is mandatory to set the game name." << std::endl;
        return 1;
    }

    std::string rawTitle = getBaseName(romPath);
    std::string title = sanitizeTitle(rawTitle);

    std::cout << "[INFO] ROM detected: \"" << romPath << "\"" << std::endl;
    std::cout << "[INFO] Target Game Title: \"" << title << "\"" << std::endl;

    // Process Guide
    if (!guidePath.empty()) {
        std::string outGuide = title + ".txt";
        copyGuideText(guidePath, outGuide);
    } else {
        std::cout << "[INFO] No Guide walkthrough file (.txt) provided, skipping." << std::endl;
    }

    // Process Maps
    if (!mapPaths.empty()) {
        for (size_t i = 0; i < mapPaths.size(); i++) {
            std::string file = mapPaths[i];
            std::string outMap = "";
            if (mapPaths.size() == 1) {
                outMap = title + ".v2m";
            } else {
                // Check if original name has a level indicator like "1-1", "2-2", "3", etc.
                std::string base = getBaseName(file);
                size_t dash = base.find_last_of("-_");
                bool foundSuffix = false;
                if (dash != std::string::npos) {
                    std::string suffix = base.substr(dash + 1);
                    if (!suffix.empty() && std::all_of(suffix.begin(), suffix.end(), [](char c){ return isdigit(c) || c == '-' || c == '_'; })) {
                        outMap = title + "-" + suffix + ".v2m";
                        foundSuffix = true;
                    }
                }
                if (!foundSuffix) {
                    outMap = title + "-" + std::to_string(i + 1) + ".v2m";
                }
            }
            processBMPToV2M(file, outMap);
        }
    } else {
        std::cout << "[INFO] No Map images (.bmp) provided, skipping." << std::endl;
    }

    std::cout << "=========================================" << std::endl;
    std::cout << "          CONVERSION COMPLETED!" << std::endl;
    std::cout << "=========================================" << std::endl;
    return 0;
}
