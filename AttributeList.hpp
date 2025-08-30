#include <windows.h>
#include <winioctl.h>
#include "MFTRuns.hpp"
#include "GetAttributes.hpp"
#include "Resources.hpp"
using namespace std;

extern bool accessMFT(BYTE* mftEntry,bool isAttributeList); 

ULONGLONG SearchEntryOffset(ULONGLONG &EntryNo){  // in the Volume
	if(EntryNo <= EntryRunNum[0])  return  MftRunList[0].StartingLCN *ClusterSize + EntryNo  * MFT_ENTRY_SIZE  ; 
	int low = 1 ;
	int high = MftRunList.size()-1 ;
		
	while(low <= high){
		WORD mid = (low) + (high - low)/2 ; 
		DWORD prevEntryNo =  EntryRunNum[mid - 1] ;
		DWORD thisEntryNo = EntryRunNum[mid] ;
        
		if(EntryNo <= thisEntryNo && EntryNo > prevEntryNo) {
		    return MftRunList[mid].StartingLCN * ClusterSize + (EntryNo - prevEntryNo - 1) * MFT_ENTRY_SIZE;
		}
		
		if (thisEntryNo < EntryNo) low = mid + 1;
		else high = mid - 1;
	}
	return (ULONGLONG)-1;
}

bool GotoMftEntry(ULONGLONG &FileReference, BYTE* newMftEntry) {
	ULONGLONG mftEntryoffset = SearchEntryOffset(FileReference) ;
	LARGE_INTEGER mftPtr ; mftPtr.QuadPart = mftEntryoffset ;
	DWORD bytesRead = 0 ;

	if (!ReadFile(hVolume, newMftEntry, MFT_ENTRY_SIZE, &bytesRead, NULL)) {
		std :: cerr << "Failed to read MFT entries. Error: " << GetLastError() << std :: endl;
		return false;
	}
	if(bytesRead != MFT_ENTRY_SIZE) return false ;
	return accessMFT(newMftEntry,true) ; 
}


bool getAttrListData(DWORD AttrListoffset,DWORD AttrListSize,BYTE *mftEntry,WORD &MFT_ENTRY_SIZE){
	BYTE non_resident = mftEntry[AttrListoffset + 8] ; 
	BYTE name_length =  mftEntry[AttrListoffset +9] ; 
	WORD name_offset = *(WORD*)&mftEntry[AttrListoffset + 10] ;
	
	if(name_length){
		string fname ; 
		wchar_t* name = (wchar_t*)&mftEntry[AttrListoffset + name_offset];
		for (int j = 0; j < name_length; j++) fname.push_back(name[j]);
		fbug << "Data Attribute -> File Name " << fname << endl ;
		return 1 ;
 	}	 
 	
 	if(non_resident){
		WORD dataListStartpos = *(WORD*)&mftEntry[AttrListoffset + 32] ;
		DWORD bytesToRead= AttrListSize - dataListStartpos ;
		DWORD dataList_offset = AttrListoffset + dataListStartpos ; 

		ULONGLONG RealSize = *(ULONGLONG*)&mftEntry[AttrListoffset + 48] ; 
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
			fbug << "Attribute List Cluster " << std :: endl ;
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
			    	std :: cout << "Problem Cluster : " << cn << "  " ;
			    	SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
					return false ; 
				}	
			
			    if (!ReadFile(hVolume, cluster, ClusterSize, &bytesRead, NULL)) {
			        std :: cerr << "Failed to read cluster. Error: " << GetLastError() << std :: endl;
			       	SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
			        return false;
			    }
			
				/* Work Here */

				RealSize -= bytesRead ; 
			    if (RealSize == 0) break;
			    cn++;
			}
			SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);
		}
 		return true ;
 	}else{		
		DWORD data_length = *(DWORD*)&mftEntry[AttrListoffset +16] ;
		if (data_length == 0) {
		    cerr << "No data in resident attribute. Skipping.\n";
		    return false;
		}
		
		WORD AttrEntryOffset = *(WORD*)&mftEntry[AttrListoffset +20] ;
		if (AttrEntryOffset + data_length > MFT_ENTRY_SIZE) {
		    cerr << "Resident data exceeds MFT entry bounds. Skipping.\n";
		    return false;
		}

		BYTE * AttrEntry = &mftEntry[AttrListoffset + AttrEntryOffset] ;
		DWORD attrType , DAoffset , attrOffset = 0; 
		WORD  DAsize , Sequence ; 
		ULONGLONG VCN , FileReference , fileRefAndSeq ;
	   	LARGE_INTEGER savePos;  
	    SetFilePointerEx(hVolume, {0}, &savePos, FILE_CURRENT);
		
		while (data_length != 0) {
		    DWORD attrType = *(DWORD*)&AttrEntry[attrOffset];    
		    if (attrType == 0xFFFFFFFF) break;
	        WORD attrLen = *(WORD*)&AttrEntry[attrOffset + 4]; 
	        if (attrLen == 0) break;
			if (attrOffset + attrLen > MFT_ENTRY_SIZE) {
			    cout << "  [!] Bad attribute length " << attrLen << " at offset " << attrOffset << endl ;
			    break;
			}	
	        VCN = *(ULONGLONG*)&AttrEntry[attrOffset + 8]; 
	        fileRefAndSeq =  *(ULONGLONG*)&AttrEntry[attrOffset + 16]; 
			FileReference = fileRefAndSeq & 0x0000FFFFFFFFFFFF ;  //  Make last 2 bytes Zero 
	        Sequence = (ULONGLONG)fileRefAndSeq >> 48 ; // Right shift first 6 bytes 
			BYTE newMftEntry[MFT_ENTRY_SIZE] ;
				    // $ATTRIBUTE LIST				 $DATA
	        if(attrType == 0x00000020 || attrType == 0x00000080) {  	
			   if(!GotoMftEntry(FileReference, newMftEntry)) return false ;
	        } 
	        attrOffset += attrLen ; 
	        data_length -= attrLen;	 
		}
		SetFilePointerEx(hVolume, savePos, NULL, FILE_BEGIN);   
	}
	return true ;
}