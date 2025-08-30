#include <windows.h>
#include <winioctl.h>
#include "Resources.hpp"
#include "BitMap.hpp"
#include "MFTRuns.hpp"
#include "GetAttributes.hpp"
#include "AttributeList.hpp"

int n = 0 ;
bool all = false ;
std :: vector<std::string>File2Recover ;

bool accessMFT(BYTE* mftEntry,bool isAttributeList) {
    WORD fileStatusFlag = *(WORD*)&mftEntry[0x16] ;
    if(fileStatusFlag != 0x0000) return false;

	DWORD attrOffset = *(WORD*)&mftEntry[20];
	DWORD SIoffset = 0, FNoffset = 0, DAoffset = 0 ,AttrListoffset = 0;
	DWORD SIsize = 0, FNsize = 0, DAsize = 0,  AttrListSize = 0 ;

    std::string fname;
    bool FileFound = false ;  // all will false if all are not to be recovered
    bool getOut = false;
    int findex = -1;
    
    while (attrOffset + 8 < MFT_ENTRY_SIZE) {
        DWORD attrType = *(DWORD*)&mftEntry[attrOffset];
        if (attrType == 0xFFFFFFFF) break;

        DWORD attrLen = *(DWORD*)&mftEntry[attrOffset + 4] ;
		if (attrLen == 0) break;
        
        if (attrOffset + attrLen > MFT_ENTRY_SIZE) {
            std::cout << "  [!] Bad attribute length " << attrLen << " at offset " << attrOffset << std::endl;
            getOut = true;
            break;
        }

        switch (attrType) {
            
            /*
            The Attribute List Part is Yet to be Completed 
            ----------------------------------------------
            case 0x00000020 :
                AttrListoffset = attrOffset;
                AttrListSize = attrLen;
                if (!getAttrListData(AttrListoffset, AttrListSize, mftEntry, MFT_ENTRY_SIZE)) getOut = true;
                break;
            */

            case 0x00000030:  // File Name attribute
                FNoffset = attrOffset ;
                FNsize = attrLen ;
                fname = getfilename(FNoffset, FNsize, mftEntry, MFT_ENTRY_SIZE);

                if(fname.empty()){
                    FileFound = false ;
                    getOut = true ; 
                    break ; 
                }
                
                if(all){
                    if(frecover.is_open()) frecover.close();
                    frecover.clear();
                    frecover.open("Recovered/" + fname, std::ios::binary) ;
                    FileFound = true ;
                }
                else{
                    for (int i = 0; i < n; i++) {
                        if (File2Recover[i] == fname) {
                            if(frecover.is_open()) frecover.close();
                            frecover.clear();
                            findex = i;
                            frecover.open("Recovered/" + fname, std::ios::binary) ;
                            FileFound = true ;
                            break ;
                        }
                    }
                }
                break;
            case 0x00000080:  // Data attribute
                DAoffset = attrOffset;
                DAsize = attrLen;
                if (debugMode || FileFound) {
                    if (!getdata(DAoffset, DAsize, mftEntry, MFT_ENTRY_SIZE)) getOut = true ;
                }
                break;
        }
        if(getOut) break;
        attrOffset += attrLen;
    }

    if(debugMode && !getOut){
        if(fname.empty()) return false ;
        fbug << fname << std::endl ;
        return true ; 
    }

    if(FileFound && !getOut) {
        if(!all) File2Recover[findex] = "" ;
        std :: cout << fname << " : File Recovered Successfully" << std::endl;
        return true ;
    }

    return false ;
}

void RecoverFile(std :: string &Drive) {
	
    std :: string driveLetter = Drive.substr(0,Drive.length() - 1) ;

    if(debugMode){
        fbug.open("debug" + driveLetter + ".txt") ;
        fbug << "Recoverable Files : " << std::endl << std::endl ;
    }
    Drive = "\\\\.\\" + Drive ; 

    std::wstring FullPath(Drive.begin(), Drive.end());
    hVolume = CreateFileW(FullPath.c_str(), GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL) ;
	
    if (hVolume == INVALID_HANDLE_VALUE) {
        std :: cerr << "Failed to open volume. Error: " << GetLastError() << std :: endl;
        std :: cin.get();
        return ;
    }
	
    BYTE bootSector[512];
    DWORD bytesRead;

    if (!ReadFile(hVolume, bootSector, 512, &bytesRead, NULL)) {
        std :: cerr << "Failed to read boot sector. Error: " << GetLastError() << std :: endl;
        CloseHandle(hVolume);
        std :: cout << "Press Enter to exit...";
        std :: cin.get();
        return ;
    }

    bytesPerSector = *(WORD*)&bootSector[11];
    sectorsPerCluster = bootSector[13];
    mftCluster = *(ULONGLONG*)&bootSector[48];
	ClusterSize = bytesPerSector * sectorsPerCluster ; 

    ULONGLONG mftOffset = mftCluster * ClusterSize ;
	
	CHAR clustersPerMFTRecord = (CHAR)bootSector[64];  // CHAR is SIGNED , BYTE is UNSIGNED
	if (clustersPerMFTRecord >= 0) MFT_ENTRY_SIZE = clustersPerMFTRecord * ClusterSize ;
	else MFT_ENTRY_SIZE = 1 << (-clustersPerMFTRecord);
	  
    DWORD TotalEntries ;
	NTFS_VOLUME_DATA_BUFFER volData;
	DWORD ret;	
	if (DeviceIoControl(hVolume,FSCTL_GET_NTFS_VOLUME_DATA,NULL, 0,&volData, sizeof(volData),&ret, NULL))
	TotalEntries = (DWORD)volData.MftValidDataLength.QuadPart / MFT_ENTRY_SIZE;
	else TotalEntries = 0 ; 
    
    GetMftRunList(mftOffset) ;
	GetVolumeBitmap() ;
    
	DWORD EntriesToRead = 200 ;
    BYTE Entries[MFT_ENTRY_SIZE * EntriesToRead];
    ULONGLONG current_entry = 0;

	for(auto mftCluster : MftRunList){
		LARGE_INTEGER mftPtr	;
	    mftPtr.QuadPart = mftCluster.StartingLCN * ClusterSize;
	    SetFilePointerEx(hVolume, mftPtr, NULL, FILE_BEGIN);
		
		DWORD Entry = (mftCluster.ClusterCount * ClusterSize + MFT_ENTRY_SIZE - 1) / MFT_ENTRY_SIZE;
		
	    while (Entry > 0) {
	        EntriesToRead = std :: min(EntriesToRead, Entry);
	        ULONGLONG BytesToRead = EntriesToRead * MFT_ENTRY_SIZE;
	
	        if (!ReadFile(hVolume, Entries, BytesToRead, &bytesRead, NULL)) {
	            std :: cerr << "Failed to read MFT entries. Error: " << GetLastError() << std :: endl;
	            break;
	        }
	
	        WORD EntriesRead = bytesRead / MFT_ENTRY_SIZE ;   
	        for (WORD i = 0; i < EntriesRead; i++) {
	            BYTE* mftEntry = &Entries[i * MFT_ENTRY_SIZE] ;
	            current_entry++;
				accessMFT(mftEntry,false) ; 
			}
			Entry -= EntriesRead;
		}
	}
	
    fbug.close();
    CloseHandle(hVolume);
    return ;
} 

int main(int argc,char* argv[]) {
	if(argc >= 2){
        std :: string Drive ;
        std :: string status = argv[1] ;
        if(status == "--debug") {
            debugMode = true ;
            Drive = argv[2] ;
            RecoverFile(Drive) ; // Debugging mode
            std :: cout << "Debug Successful" << std :: endl ;
        }
		else if(status == "--recoverall") {
            CreateDirectoryA("Recovered", NULL);  
            Drive = argv[2] ;
            all = true ;
            RecoverFile(Drive) ;
        }
		else{
            CreateDirectoryA("Recovered", NULL);  
            std :: cout << "Enter the number of files to recover: " ;
            std :: cin >> n ; 
            getchar() ; 
            File2Recover.resize(n) ;
			for(int i = 0 ; i < n ; i++){
                std :: cout << "File " << i+1 << " : ";
                getline(std :: cin,File2Recover[i]) ;
            }
            Drive = argv[2] ;
			RecoverFile(Drive) ;
            for(auto fname : File2Recover){
                std :: cout << std :: endl ;
                if(!fname.empty()){
                    std :: cout << fname << " : File Cannot be Recovered" << std :: endl ; 
                }
            }
		}
	}
}
