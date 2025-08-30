#ifndef MFTRuns_HPP
#define MFTRuns_HPP
#include "Resources.hpp"

struct MftRun {
    ULONGLONG StartingLCN;
    ULONGLONG ClusterCount;
};

std::vector<MftRun> MftRunList ;
std::vector<DWORD> EntryRunNum ;

void GetMftRunList(ULONGLONG &mftOffset)
{    
    LARGE_INTEGER li;
    li.QuadPart = mftOffset;
	if (!SetFilePointerEx(hVolume, li, NULL, FILE_BEGIN)) throw std::runtime_error("Seek to MFT entry 0 failed");
   
    DWORD bytesRead;
    BYTE mft[MFT_ENTRY_SIZE] ; 

    if (!ReadFile(hVolume, mft, MFT_ENTRY_SIZE, &bytesRead, NULL) && bytesRead != MFT_ENTRY_SIZE) {
        throw std::runtime_error("Read MFT entry 0 failed");
    }
    
  	DWORD attrOffset = *(WORD*)&mft[20];
    DWORD DAoffset = 0, DASize = 0 ;
    

    while (attrOffset + 8 < MFT_ENTRY_SIZE) {
    	DWORD attrType = *(DWORD*)&mft[attrOffset];
        if (attrType == 0xFFFFFFFF) break;
	
        DWORD attrLen = *(DWORD*)&mft[attrOffset + 4];
        if (attrLen < 8) break;
			    
        if(attrType == 0x00000080){ // $DATA
            DAoffset = attrOffset;
            DASize = attrLen;  
            
            BYTE nonResident = mft[attrOffset + 8];
            if (!nonResident) break;
            
			WORD dataListStartpos = *(WORD*)&mft[DAoffset + 32] ;
			DWORD bytesToRead= DASize - dataListStartpos ;
			DWORD dataList_offset = DAoffset + dataListStartpos ; 
	
			ULONGLONG RealSize = *(ULONGLONG*)&mft[DAoffset + 48] ; 
			ULONGLONG BW = 0 ;		
				
			DWORD read = 0 ; 
			int64_t previous_cluster = 0 ;	
			int n = 0 ;
			int64_t entryno = -1; ; 
			
			// fbug << "MFT RunList Status : " << std :: endl ;
			while(read < bytesToRead){	
				BYTE FB = mft[dataList_offset + read] ; 	 
				read ++ ;
				
				if (FB == 0x00) break ;  
				
				BYTE ones_place = FB & 0x0F;  
				BYTE tens_place = (FB & 0xF0) >> 4; 
			
				if (read + ones_place + tens_place > bytesToRead) throw std::runtime_error("Runlist exceeds boundary!");
		
				uint64_t cluster_length = 0 ;
				int64_t signed_starting_cluster = 0 ;
				
	
				for(int i = 0; i<ones_place; ++i){
				 cluster_length |= ((uint64_t)mft[dataList_offset + read]) << (i * 8) ;
				 read ++ ;
				}
				
				for (int i = 0 ; i <tens_place; ++i) {
				    signed_starting_cluster |= ((uint64_t)mft[dataList_offset + read]) << (i * 8);
				    read ++ ;
				}
					
				if (tens_place > 0 && (mft[dataList_offset + read-1] & 0x80)) {
				    signed_starting_cluster |= (~0ULL) << (tens_place * 8);
				}
				
				uint64_t starting_cluster = signed_starting_cluster ;	
				starting_cluster += previous_cluster ;
				previous_cluster = starting_cluster ;
 
				
		   		if (cluster_length == 0 ) continue ;
		   		
		   		entryno += ((cluster_length) * (ClusterSize/MFT_ENTRY_SIZE)) ; 
		   		EntryRunNum.push_back((DWORD)entryno) ; 
		   		
				/*
				fbug << "MFT RunList Info" << std :: endl ;
				fbug << "Cluster " << n++ << " : " << std :: endl ;	
				fbug << "Cluster Length : "<< cluster_length<<"  -> Clusters Size : "<< cluster_length*ClusterSize <<" BYTES" <<std ::endl ; 
				fbug << "Starting Cluster : " << starting_cluster << std ::endl  ;
				*/

                if (cluster_length > 0) {
					MftRun mr ;
					mr.StartingLCN = starting_cluster ; 
					mr.ClusterCount = cluster_length ; 
					MftRunList.push_back(mr) ;
				}
			}            
        } 
        attrOffset += attrLen;
    }
    return ;
}

#endif

