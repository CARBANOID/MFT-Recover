# MFT-Recover
MFTrecover is a file recovery tool for the **Windows NTFS 3.1 file system**, used since Windows XP through the latest Windows 11.

The tool works by parsing **Master File Table (MFT) entries** and directly accessing the raw volume to retrieve the data of deleted files.

## Supported Storage Devices

* Hard Disk Drives (HDDs)
* Solid State Drives (SSDs) with **TRIM disabled**

## Incomplete Features

**Attribute List Handling**:
  Recovery support for Attribute List files is not fully implemented yet.
  I was unable to test with Large files containing an Attribute List on my system because they were not present.

**Current Issue**:
  The Attribute List logic currently uses recursive searching without a defined depth limit, which may cause crashes.

**Workaround**:
  The tool functions correctly without Attribute List support. For stability, the incomplete code has been commented out.

---

# How to Run

1. **Open Terminal (Command Prompt / PowerShell) as Administrator**

2. **Compile the program**

   ```bash
   g++ accessMFT.cpp -o MFTrecover.exe -g
   ```

3. **Run the program with the desired option**

### 1. Debug Mode

Checks which files are recoverable without actually restoring them.

```bash
MFTrecover.exe --debug <drive_letter>
```

Example : 

```bash 
MFTrecover.exe --debug D:
```

**debug<drive_letter>** (here debugD) name file will be created which will contain the list of recoverable files in that specific drive

### 2. Recover All Deleted Files

Recovers all deleted files from the drive into a folder named `Recovered`.

```bash
MFTrecover.exe --recoverall <drive_letter>
```

Example : 

```bash
MFTrecover.exe --recoverall D:
```

### 3. Recover Specific Files

Lets you enter the number of files and their names manually, then attempts to recover only those and stores them into a folder named `Recovered`.

```bash
MFTrecover.exe --recover <drive_letter>
```
Example :

```bash
MFTrecover.exe --recover D:
```

(After running this, the program will ask how many files you want to recover and their names.)


---
