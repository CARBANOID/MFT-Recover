NTFS 
----

-> Everything is a file -> all system data is stored as files in NTFS
-> In FAT file system some system metadata is stored in flat data table in reseverd system space

-> NTFS file system files are stored in the root directory as hidden files , 
   usually designated as $[MetaData_FileName] -> syntax of metafile
   
$MFT
----

-> The primary NTFS MetaData file is the $MFT 
-> $MFT tracks every file and directory on a NTFS volume , including itself 
-> $MFT is like a master index , index of the entire volume in one location and make refrences to
	other index files such as $I30 file  // (each directory has it's own I30 index file)

	$I30 -> keeps the listing/indexes of all the files present inside the directory
	
	NTFS Volume X
		|->[root] 
	          |-> $MFT 
			  |	  |-> $I30 dir1
			  |	  |-> $I30 dir2
			  |	  |-> ...
			  |	  |-> ...
		   	  |	  |-> $I30 dirN
			  |	
	          |-> $I30 [root] // $I30 index file of [root] directory
	          
	          
	N -> Total Number of directories inside NFT Volume X 
	

$MFT Entries 
------------
-> Each entry/file record in MFT is 1024 bytes, or two sectors 

-> if there is too much metadata for a file to fit within 1024 btyes then MFT will create second entry for it and 
reference that second entry 
-> typically most of the times one entry is created for one file 

	Entry (file record) header
	--------------------------
	-> Each entry/file record has a header that contains metadata about the entry .
	-> offset 20 of the entry tells us about the size of the header 
	-> The header is usually the first 56 bytes of entry
	
	Entry signature -> entries start with  46 49 4C 45 / x46x49x4Cx45 or FILE  (first 4 bytes)
	(at the starting fo each entry)
	

 -> First entry of MFT contains metadata about MFT 
 
 	Fix Up Array 
 	-------------
 	-> It is a way for MFT do error checking on itself
 	
 	-> offet 04 and offset 05 = number of btyes in the Fix Up array (tells where the fix up array starts from )
  	-> offet 06 and offset 07 = number of two-btyes entries in the Fix Up array (how many byte pairs are in the fix up array)
 
 		at offset 04 we have hex value 30
		 -> it tells us that fix up array starts after 48 bytes(16^1 * 3 + 16^0 * 0) from the start of entry
		 
		at offset 05 we have hex value 03 which tells that it's' going to occupy 3-2 bytes string

		 how MFT check 
		  if first 2 bytes of the fix up array and last 2 bytes of each sectors are same then no problem
		  else ther is a problem		
 	
	 	----FILE---  ---Fix Up Array---     
	 	46 49 4C 45   30           00    03 00-5B 1C 1F 9D 02 00 00 00
offset	00 01 02 03   04           05    06 07 08                   15


	offset 22-23 :
		Flags
	    0x00 not in use (deleted)
	    0x01 in use
	    0x02 directory
	    0x03 directory in use
	Deleting a file changes the flag to 0x00, but does nothing to clear out the data, thus many deleted file's metadata is still recoverable as long as the record hasn't been recycled.

24-27 Used size of MFT entry
	$LogFile Transaction Entry 
	--------------------------
		-> $LogFile -> Keeps the log of the all the changes which has been done in the whole volume/system 
					  such as whether a file has changed or updated .
					-> each entry of LogFile takes 4096 btyes / 8 Sectors 
					-> 52 43 52 44/x52x43x52x44 or RCRD is the Entry Signature of each entry in the logFile 
					  
		-> offset 08 to 15 = $LogFile Transaction Entry 
		
	 	<---FILE-->  <-Fix Up Array->          <-LogFile Transaction Entry ->
	 	46 49 4C 45   30           00    03 00-5B 1C 1F 9D 02 00 00 00
offset	00 01 02 03   04           05    06 07 08                   15 
		

			 
	$MFTMirr              	
	--------
	 -> Contains first 4 entries of MFT 
	 	-> 1st entry -> $MFT
	 	-> 2nd entry -> $MFTMirr
	 	-> 3rd entry -> $LogFile
	 	-> 4th entry -> $Volume
	


$MFT Attributes
***************
	Most Important Attributes (most forensics exam)
	-------------------------
		-> Standard Information Attribute -> Signature/header -> (x10x00x00x00) 
		-> File Name Attribute -> Signature/header -> (x30x00x00x00)
		-> Data Attribute -> Signature/header -> (x80x00x00x00)  
	
	
	Resident Attribute Structure  (Standard Information ,File Name Attribute,
	----------------------------  Data Attribute(can be since data can stored with it or in clusters))

  Offset |Size| Field
	0x00 | 4  | Attribute Type (0x30)
	0x04 | 4  | Length of Attribute
	0x08 | 1  | Non-resident flag (0)
	0x09 | 1  | Name length (optional)
	0x0A | 2  | Name offset
	0x0C | 2  | Flags
	0x0E | 2  | Attribute ID
	0x10 | 4  | Content size (resident)
	0x14 | 2  | Content offset	
	
		
	First Attribute offset 
	----------------------
	-> offset 20 to 21 = number of bytes to the first attribute (usually the FileName attribute)
						-> tells after how many bytes from start of entry , first attribute is present
	
	 Standard Information Attribute -> Starting with/Signature/header -> (x10x00x00x00) 
	--------------------------------------------------------------------------------
	
	-> first byte after the Standard Information Attribute header  tells about the size of the
	   Standard Information Attribute and also the starting of next attribute which 
	   is after the Standard Information Attribute

	-> If a file is moved from system A to System B then time when the file is moved in System B ,that time will be it's
		creation time in System B .
	-> other time stamps may remain same as it was in System A .
	
		FILE TIME (local -> according to my system bois time format) 
		FILE TIME(UTC -> universal time code)

Time Stamps 
-----------
1) CT(Creation date and time) Contains a FILETIME

2) MT(Last modification date and time)
(Also referred to as last written date and time)
Contains a FILETIME

3) (EFT) entry last modification date and time
Contains a FILETIME

4) LAT(Last access date and time)Contains a FILETIME		
		

1st sector of 1st entry of $MFT 
-------------------------------

46 49 4C 45 30 00 03 00 5B 1C 1F 9D 02 00 00 00


01 00 01 00 38 00 01 00 B8 01 00 00 00 04 00 00
00 00 00 00 00 00 00 00 07 00 00 00 00 00 00 00	
						 
						|SI header| |--SIsize--|
3A 00 00 00 00 00 00 00 10 00 00 00 60  00 00 00
						|StartSI
						--------------------  
											| 
      offset 20 tells after how many bytes from SI starting,first metadata entry for that file starts i.e x18 -> 24
	                   |ContentSize| |contentoffset|
00 00 18 00 00 00 00 00 48 00 00 00   18 00           00 00
-------
|------------------Time Stamp-----------------|-----
|--Creation Date Time-|  |Last Modify DateTime|			 |--> Time Stamp is the date and time information of a file, 				   	
6C D2 C2 46 DB 65 DA 01  6C D2 C2 46 DB 65 DA 01   		 |there are 4 time stamps each of 8 bytes 
|------------------Time Stamp-----------------|          |CT,MT,EMT,LAT -> will be same for MFT entry
|Entry Modify DateTime|  |Last Accessed Date Time|					   	
6C D2 C2 46 DB 65 DA 01  6C D2 C2 46 DB 65 DA 01
06 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00
					     |FN Header| FNAsize
00 00 00 00 00 00 00 00  30 00 00 00  68     00 00 00
		 			 ^	 ^
					 |	 |
				endSI	startFN

						|--Creation Date Time-|  
00 00 18 00 00 00 03 00 4A 00 00 00 18 00 01 00
						  							-> Standard information time stamps gets updated regulary 
												      where as file name time stamps doesn't
|Last Modify DateTime|	|Entry Modify DateTime|			
6C D2 C2 46 DB 65 DA 01 6C D2 C2 46 DB 65 DA 01

|Last Accessed DateTime|	
6C D2 C2 46 DB 65 DA 01 00 40 00 00 00 00 00 00
00 40 00 00 00 00 00 00 06 00 00 00 00 00 00 00
04 03 24 00 4D 00 46 00 54 00 00 00 00 00 00 00
											 ^
											 |
											 endFN
									 											 
-DHeader-   Dsize		     
80 00 00 00  48     00 00 00 01 00 40 00 00 00 06 00
^							  ^
|							  |---------------------------Flag -> resident file(x00) or non-resident file(x01)
startD																	
 								resident file =all file data is present is this MFT entry (rare case)
								cause the MFT entry size is only 1KB so the file size has to be atmost 600 or 700 btyes

00 00 00 00 00 00 00 00 7F A8 00 00 00 00 00 00

 after 4 rows from DHeader , the first run-list starts i.e file data starting (where on disk data starts)
 |																	|			
40 00 00 00 00 00 00 00 00 00 88 0A 00 00 00 00						|	
00 00 88 0A 00 00 00 00 00 00 88 0A 00 00 00 00						|
																	|
---------------------------------------------------------------------
|-first run list- 6bytes
|				|
33 80 A8 00 00 00 0C 00 B0 00 00 00 68 00 00 00
01 00 40 00 00 00 05 00 00 00 00 00 00 00 00 00
06 00 00 00 00 00 00 00 40 00 00 00 00 00 00 00
00 70 00 00 00 00 00 00 08 60 00 00 00 00 00 00
08 60 00 00 00 00 00 00 31 01 FF FF 0B 31 01 26
00 F4 41 01 85 69 C9 02 41 01 66 F1 FA FE 31 01
4E 01 0C 41 01 E7 26 F9 00 41 01 B0 9F 41 FD 00
FF FF FF FF 00 00 00 00 01 00 40 00 00 00 05 00
00 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00
40 00 00 00 00 00 00 00 00 20 00 00 00 00 00 00
08 10 00 00 00 00 00 00 08 10 00 00 00 00 00 00
31 01 FF FF 0B 31 01 26 00 F4 00 00 00 00 3A 00


Run List/Fragment
-----------------
-> MFT will tell us about each Run List ,how many cluster it is total and where it starts 
	
	Runlist signature 
	xB1xB2xB3xB4xB5xB6xB7xB8....
	
FB -> first btye -> Flag -> (tells total number of bytes present in the runlist excluding itself) = B1one's'+B1ten's'

	FB = TO
***one's' position(O) of FB -> It tells that the next O bytes (B1,B2,...,BO) tells the size of the fragment/RunList
							/length(Number) of the clusters assigned to fragment/RunList .
							
***Ten's' position(T) of FB -> It tells after O bytes next T Bytes  (B_1,B_2,...,B_T) tells current fragment/RunList cluster
							  starting location  in disk from previous fragment starting cluster
							 / current fragment starting cluster is how much clusters after the 
							 	previous fragment starting cluster 
	example - 
		RunList = xB1xB2xB3xB4xB5xB6xB7xB8
		B1 = 43 -> 4+3 = 7 bytes (after B1) are in the run list i.e total bytes in runlist = (4+3)+1		
			
	***	one's' position of B1 -> It tells that first 3 bytes (B2,B3,B4) is the size of the fragment/RunList
	***	Ten's' position of B1 -> next 4 bytes (B5,B6,B7,B8) is it's' starting location in clusters in disk
			

Slack Data in the Clusters 
-------------------------
-> Any bytes between the file’s logical end (EOF) and the end of the final allocated cluster are called file slack.
so , the slack data is only present in the last cluster of last datalist


How NTFS Writes Data:
---------------------	
-> Files are stored in clusters, and NTFS allocates full clusters to files, 
-> even if the file size isn’t a perfect multiple of the cluster size.
-> The last data run (the last sequence of contiguous clusters in the $DATA attribute) ends with a partial cluster
 if the file size isn’t a multiple of the cluster size.

-> The file’s Real Size in DATA ATTRIBUTE tells where the meaningful data ends.

If the cluster size is 4096 bytes and the file ends at byte 1000 inside the last cluster, then 3096 bytes are slack.

Important Notes:
----------------
-> Only the last cluster can contain file slack, because the rest of the clusters are filled fully with actual file data.
-> If the file’s real size is an exact multiple of the cluster size, there is no slack.
-> Slack bytes may contain:
	-> Zeroes (if OS zeroed them during allocation)
	-> Leftover data from previously deleted files (if not zeroed)

	Example:
	--------
	File size (RealSize): 6600 bytes
	Cluster size: 4096 bytes
	Required clusters: 2
	First cluster (4096 bytes): fully used
	Second cluster: only 2504 bytes used → 1592 bytes of slack

Where slack lives:
****************************************************
Slack data = ClusterSize - (RealSize % ClusterSize)*
But only in the last cluster of the last data run. *
****************************************************

FILE SLACK
----------
File slack is the unused space between the end of file and the end of cluster. 
File slack appears because cluster is the smallest unit of disk space allocation in NTFS 
and whole cluster is used even the file does not fill the whole cluster (Mallery, 2001). 
This empty space can be used to hide data (Chuvakin, 2002)

There are 2 types of file slack, which are RAM slack and drive slack. 
RAM slack spans from end of a file to the end of sector while drive slack spans from the start of next sector to
the end of cluster (NTI, 2004). For example, a 600 bytes file is stored in a NTFS with 
2048-bytes cluster and 512-bytes sector as shown in figure 8. 
RAM slack is from the end of file to the end of sector 2 and drive slack is composed of sector 3 and 4.

TRIM in SSD
-----------

By Defualt Trim Operation is Set(0) in windows , as soon as the file is deleted SSD calls trim operation which find the free
clusters aviable in the device through $BITMAP and trims those clusters i.e rewrites the clusters with zero , 
so to recover in SSD we have to turn off Trim Operation i.e Set(1) .

Open Command Prompt in Admin Mode and Type
> fsutil behavior query DisableDeleteNotify

1) DisableDeleteNotify = 0 → TRIM is enabled
2) DisableDeleteNotify = 1 → TRIM is disabled

To Disable TRIM 
---------------
> fsutil behavior set DisableDeleteNotify 1

To Enable Trim 
--------------
fsutil behavior set DisableDeleteNotify 0



