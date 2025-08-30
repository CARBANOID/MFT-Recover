#ifndef GetAttributes_HPP
#define GetAttributes_HPP
#include "Resources.hpp"
#include "BitMap.hpp"

bool isClusterUsed(uint64_t &clusterNum) {
    if (clusterNum >= (uint64_t)Bitmap.size()) return true; 
    return Bitmap[clusterNum];
}


bool getdata(DWORD &DAoffset,DWORD &DASize,BYTE *mftEntry,WORD &MFT_ENTRY_SIZE){  //  Data Attribute
	BYTE non_resident = mftEntry[DAoffset + 8] ; 
	BYTE name_length =  mftEntry[DAoffset +9] ; 
	WORD name_offset = *(WORD*)&mftEntry[DAoffset + 10] ;
	
	if(name_length){
		std :: string fname ; 
		wchar_t* name = (wchar_t*)&mftEntry[DAoffset + name_offset];
		for (int j = 0; j < name_length; j++) fname.push_back(name[j]);
		return 1 ;
 	}
	 	 
	if(non_resident){
		WORD dataListStartpos = *(WORD*)&mftEntry[DAoffset + 32] ;
		DWORD bytesToRead= DASize - dataListStartpos ;
		DWORD dataList_offset = DAoffset + dataListStartpos ; 

		ULONGLONG RealSize = *(ULONGLONG*)&mftEntry[DAoffset + 48] ; 
		ULONGLONG BW = 0 ;
		
		DWORD read = 0 ; 
		int64_t previous_cluster = 0 ;
		int n = 0 ;
		
	   	LARGE_INTEGER savePos;  
	    SetFilePointerEx(hVolume, {0}, &savePos, FILE_CURRENT);

		while(read < bytesToRead){	
			BYTE FB = mftEntry[dataList_offset + read] ; 	 
			read ++ ;
			
			if (FB == 0x00) break ;  
			
			BYTE ones_place = FB & 0x0F;   // Mask the lower 4 bits (0x0F = 00001111)
			BYTE tens_place = (FB & 0xF0) >> 4; // Mask the upper 4 bits (0xF0 = 11110000) and shift 4 bits right
		
			if (read + ones_place + tens_place > bytesToRead) {
			    SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
		        return false;
			}
					
			uint64_t cluster_length = 0 ;
			int64_t signed_starting_cluster = 0 ;

			for(int i = 0; i<ones_place; ++i){
			 cluster_length |= ((uint64_t)mftEntry[dataList_offset + read]) << (i * 8) ;
			 read ++ ;
			}
			
			for (int i = 0 ; i <tens_place; ++i) {
			    signed_starting_cluster |= ((uint64_t)mftEntry[dataList_offset + read]) << (i * 8);
			    read ++ ;
			}
				
			if (tens_place > 0 && (mftEntry[dataList_offset + read-1] & 0x80)) {
			    signed_starting_cluster |= (~0ULL) << (tens_place * 8);
			}
			
			uint64_t starting_cluster = signed_starting_cluster ;	
			starting_cluster += previous_cluster ;
			previous_cluster = starting_cluster ; 
			
	   		if (cluster_length == 0 ) continue ;
	   		
	   		/*
			fbug << "Cluster " << n++ << " : " << std :: endl ;	
			fbug << "Cluster Length : "<< cluster_length<<"  -> Clusters Size : "<< cluster_length*ClusterSize <<" BYTES" << std :: endl ; 
			fbug << "Starting Cluster : " << starting_cluster << std :: endl  ; 
			*/
			
			BYTE cluster[ClusterSize] ; 
			DWORD bytesRead;
			
			uint64_t cn = starting_cluster ; 
			
			LONG64 clusterOffset = starting_cluster * ClusterSize ;
			LARGE_INTEGER clusterPointer ; 
			clusterPointer.QuadPart = clusterOffset ;
			SetFilePointerEx(hVolume, clusterPointer, NULL, FILE_BEGIN);

			for (uint64_t i = 0; i < cluster_length; i++) {
			    if (isClusterUsed(cn)){
			    	// fbug << "Problem Cluster : " << cn << "  " ;
			    	SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
					return false ; 
				}
				
			
			    if (!ReadFile(hVolume, cluster, ClusterSize, &bytesRead, NULL)) {
			        std :: cerr << "Failed to read cluster. Error: " << GetLastError() << std :: endl;
			       	SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
			        return false;
			    }
			
			    DWORD toWrite = (DWORD)std :: min((ULONGLONG)bytesRead, RealSize);
			    if(!debugMode) frecover.write((const char*)cluster, toWrite) ;
			    BW += toWrite ;
			    RealSize -= toWrite ;
	
			    if (RealSize == 0) break;
			    cn++;
			}
			SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
		}
		
	}
	else
	{
		DWORD data_length = *(DWORD*)&mftEntry[DAoffset +16] ;
		if (data_length == 0) {
		    std :: cerr << "No data in resident attribute. Skipping.\n";
		    return false;
		}
		WORD data_offset = *(WORD*)&mftEntry[DAoffset +20] ;
		if (data_offset + data_length > MFT_ENTRY_SIZE) {
		    std :: cerr << "Resident data exceeds MFT entry bounds. Skipping.\n";
		    return false;
		}
		BYTE* rawdata = (BYTE*)&mftEntry[DAoffset + data_offset];
	    if(debugMode) frecover.write((const char*)rawdata, data_length);
	}
	
	return true ; 
}


std :: string getfilename(DWORD &FNoffset,DWORD &FNSize,BYTE *mftEntry,WORD &MFT_ENTRY_SIZE){  // File Name Attribute
	if(FNoffset == 0 || FNoffset + 22 >= MFT_ENTRY_SIZE) {
        return "" ;
    }

    WORD contentOffset = *(WORD*)&mftEntry[FNoffset + 20] ;
    DWORD fileNameLengthPos = FNoffset + contentOffset + 64 ;

    if (fileNameLengthPos >= MFT_ENTRY_SIZE) {
        return "" ;
    }

    BYTE fileNameLength = mftEntry[fileNameLengthPos];
    if (fileNameLength == 0) {
        return "";
    }

    DWORD fileNameStart = FNoffset + contentOffset + 66;

	wchar_t* fileName = (wchar_t*)&mftEntry[fileNameStart];
	std :: string fname = "" ; 
	for (int j = 0; j < fileNameLength; j++) fname.push_back(fileName[j]);
	return fname ;
}
#endif