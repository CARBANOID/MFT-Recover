#ifndef BitMap_HPP
#define BitMap_HPP
#include "Resources.hpp"

std :: vector<bool>Bitmap ;

void GetVolumeBitmap()
{
    STARTING_VCN_INPUT_BUFFER input = {};
    input.StartingVcn.QuadPart = 0;
    const DWORD outBufferSize = 1024 * 1024 * 80; // 80MB
    std::vector<BYTE> buffer(outBufferSize);

    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hVolume,
        FSCTL_GET_VOLUME_BITMAP,
        &input,
        sizeof(input),
        buffer.data(),
        outBufferSize,
        &bytesReturned,
        NULL
    );

    if (!success) throw std::runtime_error("DeviceIoControl FSCTL_GET_VOLUME_BITMAP failed.");

    auto* pBitmap = reinterpret_cast<PVOLUME_BITMAP_BUFFER>(buffer.data());
    ULONGLONG startingLcn  = pBitmap->StartingLcn.QuadPart;
    ULONGLONG clusterCount = pBitmap->BitmapSize.QuadPart;
    BYTE* bitData = pBitmap->Buffer;

    std::cout << "Bitmap covers LCNs " << startingLcn << " through " << (startingLcn + clusterCount - 1) << std::endl;
    Bitmap.clear();
    Bitmap.reserve(static_cast<size_t>(startingLcn + clusterCount));

    for (ULONGLONG i = 0; i < startingLcn; ++i) Bitmap.push_back(true);

    for (ULONGLONG i = 0; i < clusterCount; ++i) {
        size_t byteIndex = static_cast<size_t>(i / 8);
        size_t bitOffset = static_cast<size_t>(i % 8);
        bool isAllocated = (bitData[byteIndex] & (1 << (bitOffset))) != 0 ;
        Bitmap.push_back(isAllocated);    
    }
}
#endif