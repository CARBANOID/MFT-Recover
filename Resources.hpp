#ifndef Resources_HPP
#define Resources_HPP

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

HANDLE hVolume ;

WORD MFT_ENTRY_SIZE ;
WORD bytesPerSector ;
BYTE sectorsPerCluster ;
ULONGLONG ClusterSize ;
ULONGLONG mftCluster ;

std :: ofstream frecover ; 
std :: ofstream fbug ;
bool debugMode = false ;

#endif
