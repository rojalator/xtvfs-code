/* This file is part of XTVFS Reader.
 * Copyright (C) 2014 S. Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XTVFS Reader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XTVFS Reader.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "filesystem.h"

#include <algorithm>
#include <iostream>

using namespace std;
using namespace fs;

#include <iomanip>
static void dumpBlock(ostream &s, const Fat32::ByteArray &block, unsigned int wrap = 16, unsigned int pad = 4)
{
    string ascii;
    for (unsigned int i=0; i<block.size(); ++i)
    {
        if (i % wrap == 0)
        {
            s << " " << ascii << endl;
            ascii.clear();
        }
        else if (i % pad == 0)
        {
            s << " ";
        }

        s << hex << setw(2) << setfill('0') << (int)block[i] << ' ';
        if (isprint(block[i]))
            ascii += block[i];
        else
            ascii += '.';
    }
    s << endl;
}

#define Read8Bits(block, index) (block[index])
#define Read16Bits(block, index) (block[index+1]<<8 | block[index])
#define Read24Bits(block, index) (block[index+1]<<16 | block[index+1]<<8 | block[index])
//#define Read32Bits(block, index) (block[index+3]<<24 | block[index+2]<<16 | block[index+1]<<8 | block[index])
#define Read32Bits(block, index) ((unsigned long long)block[index+3]<<24 | block[index+2]<<16 | block[index+1]<<8 | block[index])

const size_t NoMoreClusters = 0x0FFFFFF8;
const size_t BadCluster = 0x0FFFFFF7;


string fs::to11CharFormat(const std::string &s)
{
    // Convert to 11-char format
    string eleven;
    size_t period = s.find_first_of(".");
    if (period != string::npos)
        eleven = s.substr(0, period);
    else
        eleven = s.substr(0, 8);
    while (eleven.size() != 8)
        eleven.push_back(' ');
    if (period != string::npos)
        eleven += s.substr(period+1);
    while (eleven.size() != 11)
        eleven.push_back(' ');
    std::transform(eleven.begin(), eleven.end(), eleven.begin(), ::toupper);

    return eleven;
}


string fs::from11CharFormat(const std::string &s)
{
    string eight(s.substr(0,8));
    eight.erase(eight.find_last_not_of(" ")+1);
    string three(s.substr(8,3));
    three.erase(three.find_last_not_of(" ")+1);

    string eightPointThree(eight);
    if (!three.empty())
        eightPointThree += "." + three;

    std::transform(eightPointThree.begin(), eightPointThree.end(), eightPointThree.begin(), ::tolower);

    return eightPointThree;
}


std::string DirEntry::toString() const
{
    return from11CharFormat(filename);
}


string DirEntry::attribToString(bool pad) const
{
    string s;
    const char padChar('.');

    if ((attrib & 0x0F) == 0x0F)
        s = "LFN";
    else
    {
        if (attrib & (1 << 0))
            s += "R";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 1))
            s += "R";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 2))
            s += "S";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 3))
            s += "V";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 4))
            s += "D";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 5))
            s += "A";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 6)) // ExPVR is calling this Device. Using X for for XTVFS VFAT
            s += "X";
        else if (pad)
            s += padChar;

        if (attrib & (1 << 7)) // ExPVR is calling this Normal
            s += "N";
        else if (pad)
            s += padChar;
    }

    return s;
}



// ===========================================================================
// ==                  F I L E S Y S T E M   C L A S S                      ==
// ===========================================================================

const size_t FileSystem::lbaBlockSize = 512;

bool FileSystem::open(const std::string &filepath)
{
    disk.open(filepath.c_str(), ios::in | ios::binary);

    if (!disk)
    {
        cerr << "Error opening " << filepath << endl;
        return false;
    }
    else
        return true;
}


FileSystem::~FileSystem()
{

}


bool FileSystem::readPartition(const ByteArray &mbr, unsigned int partition)
{
    if (partition > 3)
    {
        cerr << "Invalid partition number:" << partition << endl;
        return false;
    }

    const unsigned int offset = 446 + (partition * 16);

    unsigned char boot_flag = Read8Bits(mbr, offset + 0);
    unsigned int  CHS_begin = Read24Bits(mbr, offset + 1);
    unsigned char type_code = Read8Bits(mbr, offset + 4);
    unsigned int  CHS_end = Read24Bits(mbr, offset + 5);
    unsigned int LBA_begin = Read32Bits(mbr, offset + 8);
    unsigned int Number_of_sectors = Read32Bits(mbr, offset + 12);

    cout << "Partition " << (partition+1) << ": " << (int)boot_flag << ", "
         << CHS_begin << ", " << (int)type_code << ", " << CHS_end << ", "
         << LBA_begin << ", " << Number_of_sectors << endl;

    // Basic sanity check that the partition has been allocated some sectors
    return (Number_of_sectors > 0);
}


/// The first 446 bytes of the MBR are code that boots the computer. This
/// is followed by a 64 byte partition table, and the last two bytes are
/// always 0x55 and 0xAA. You should always check these last two bytes, as
/// a simple "sanity check" that the MBR is ok.
bool FileSystem::convertToMBR(const ByteArray &block)
{
    if ( (block.size() != 512) || (block[510] != 0x55) || (block[511]  != 0xAA) )
    {
        cerr << "Sanity check for MBR failed" << endl;
        return false;
    }

    bool okay = true;

    // Read partition 1 at block[446-461]
    okay = readPartition(block, 0);
    if (!okay)
    {
        cerr << "Failed to read first partition information!" << endl;
        return false;
    }

    // Read partition 2 at block[462-477]
    okay = okay && readPartition(block, 1);
    // Read partition 3 at block[478-493]
    okay = okay && readPartition(block, 2);
    // Read partition 4 at block[494-509]
    okay = okay && readPartition(block, 3);
    // Each partition is:
    // [0] = boot flag
    // [1-3] = CHS begin
    // [4] = Type code
    // [5-7] = CHS end
    // [8-11] = LBA begin
    // [12-15] = Number of sectors

    return okay;
}


FileSystem::ByteArray FileSystem::readLBA(size_t lba)
{
    static char buffer[lbaBlockSize];

    disk.seekg(lba * lbaBlockSize);
    disk.read(buffer, lbaBlockSize);

    return ByteArray(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]) );
}


FileSystem::ByteArray FileSystem::readLBA(size_t lba, size_t blocksToRead)
{
    ByteArray whole;
    for (unsigned int n=0; n<blocksToRead; n++)
    {
        const ByteArray part(readLBA(lba+n));
        whole.insert(whole.end(), part.begin(), part.end());
    }

    return whole;
}


FileSystem::ByteArray Fat32::readCluster(size_t clusterNumber)
{
    const size_t lbaAddr = m_clusterBeginLBA + (clusterNumber - 2) * m_sectorsPerCluster;
    return readLBA(lbaAddr, m_sectorsPerCluster);
}



// ===========================================================================
// ==                    F A T 3 2   C L A S S                              ==
// ===========================================================================

Fat32::Fat32()
{
}


bool Fat32::open(const std::string &filepath)
{
    if (!inherited::open(filepath))
        return false;

    bool okay;

    // Read the MBR?
    ByteArray block = readLBA(0);
#ifdef READ_MBR
    okay = convertToMBR(block);
    if (okay)
    {
        // We found an MBR, so read the first parition?
        // Need to get the data back from the call, but just guess for now
        block = read_lba(1);
    }
#endif
    // Try it as a FAT32 volume ID
    okay = convertToVolumeId(block);
    if (!okay)
        return okay;

    block = readLBA(1);
    okay = convertToFsInfo(block);
    if (!okay)
        return okay;

    return okay;
}


DirEntry Xtvfs::readDirectoryentry(const ByteArray &block, const int offset)
{
    DirEntry dirEntry = Fat32::readDirectoryentry(block, offset);
    dirEntry.filesize += (unsigned long long)Read8Bits(block, offset + 0x10) << 32;
    return dirEntry;
}


DirEntry Fat32::readDirectoryentry(const ByteArray &block, const int offset)
{
    DirEntry dirEntry;

    dirEntry.filename.resize(11);
    for (int i=0; i<11; i++)
        dirEntry.filename[i] = block[offset+i];
    dirEntry.attrib = Read8Bits(block, offset + 0x0B);
    const size_t firstClusterHigh = Read16Bits(block, offset + 0x14);
    const size_t firstClusterLow = Read16Bits(block, offset + 0x1A);
    dirEntry.firstCluster = (firstClusterHigh << 16) | firstClusterLow;
    dirEntry.filesize = Read32Bits(block, offset + 0x1C);

    return dirEntry;
}

#include <sstream>
#include <cmath>
string humanReadableByteCount(unsigned long long bytes)
{
    stringstream s;
    const int unit = 1024;
    if (bytes < unit)
    {
        s << bytes << " B";
    }
    else
    {
        const int exp = log(bytes) / log(unit);
        static const char pre[] = "KMGTPE";
        s << (bytes / pow(unit, exp)) << " " << pre[exp-1];
    }
    return s.str();
}


DirEntries Fat32::readDirectory(size_t startCluster)
{
    if (startCluster == (size_t)-1)
        startCluster = m_rootDirFirstCluster;

    DirEntries entries;

    ByteArray block = readCluster(startCluster);
//block.resize(50 * 32);
//dumpBlock(cout, block, 32);
    bool foundEndOfDirectoryMarker = false;
    while (!foundEndOfDirectoryMarker)
    {
        const int numBytesPerEntry = 32;
        const int numEntries = block.size() / numBytesPerEntry;
        for (int f=0; f<numEntries; ++f)
        {
            const int offset = f * numBytesPerEntry;

            DirEntry dirEntry = readDirectoryentry(block, offset);

            if (dirEntry.isDeleted())
                continue; // Deleted file found
            if (dirEntry.isEndOfList())
            {
                foundEndOfDirectoryMarker = true;
                break; // End of directory
            }
#ifdef DEBUG
cout << dirEntry.filename << " " << (int)Read8Bits(block, offset + 0x10) << " " << humanReadableByteCount(dirEntry.filesize) << endl;
#endif
            if (!dirEntry.isLFN())
                entries.push_back(dirEntry);

            if (dirEntry.isLFN())
                // http://wiki.osdev.org/FAT#Long_File_Names
                cout << "Skipping LFN entry." << endl;
#ifdef DEBUG
            else
                cout << "'" << dirEntry.filename << "' " << dirEntry.attribToString() << " " << dirEntry.filesize << " bytes [" << dirEntry.firstCluster << "]" << endl;
#endif
        }

        // Fetch the next directory block
        if (!foundEndOfDirectoryMarker)
        {
            startCluster = nextCluster(startCluster);
            if (startCluster >= 0x0FFFFFFF)
            {
                //cerr << "End of directory blocks without an end-of-dir marker" << endl;
                break;
            }
            block = readCluster(startCluster);
        }
    }

    return entries;
}


DirEntry Fat32::infoFor(const std::string &path)
{
    size_t cluster = m_rootDirFirstCluster;

    string s, p(path);
    size_t n = 0;
    while (!p.empty())
    {
        // Determine the next bit of path to process
        n = p.find_first_of("\\/", n);
        if (n != string::npos)
        {
            s = p.substr(0, n);
            p = p.substr(n+1);
        }
        else
        {
            s = p;
            p.clear();
        }

        // Convert to 11-char format, to make searching quicker
        const string eleven = to11CharFormat(s);

        const DirEntries entries = readDirectory(cluster);
        for (size_t i=0; i<entries.size(); ++i)
        {
            const DirEntry &d = entries[i];
            if (d.filename == eleven)
            {
                // Found it, but are we there yet?
                if (p.empty())
                    return d; // Found the leaf!
                else if (d.isDirectory())
                {
                    // Note the cluster number for the next time round the loop
                    cluster = d.firstCluster;
                    break;
                }
                else
                {
                    cout << "Unable to recurse but not found the leaf" << endl;
                    return DirEntry();
                }
            }
        }
    }

    return DirEntry();
}


#include <cmath>
/**
 * Read a volume ID block.
 * @param block The volume ID block
 * @return
 */
bool Fat32::convertToVolumeId(const ByteArray &block)
{
    if ( (block.size() != 512) || (block[510] != 0x55) || (block[511]  != 0xAA) )
    {
        cerr << "Sanity check for Volume ID failed" << endl;
        return false;
    }

    //dumpBlock(cerr, block);

    BPB_BytsPerSec = Read16Bits( block, 0x0B);  // Bytes per sector. Always 512
    BPB_SecPerClus = Read8Bits ( block, 0x0D);  // Sectors per cluster. 1,2,4,8,16,32,64,128
    BPB_RsvdSecCnt = Read16Bits( block, 0x0E);  // Number of reserved sectors. Usually 0x20
    BPB_NumFATs    = Read16Bits( block, 0x10);  // Number of FATs. Always 2
    BPB_TotSec32   = Read32Bits( block, 0x20);  // Total number of sectors
    BPB_FATSz32    = Read32Bits( block, 0x24);  // Sectors per FAT. Value depends on disk size
    BPB_RootClus   = Read32Bits( block, 0x2C);  // Root directory first cluster. Usually 0x00000002
    const int signature      = Read16Bits( block, 0x1FE); // Always 0xAA55

    cout << BPB_BytsPerSec << ", " << BPB_SecPerClus << ", "
         << BPB_RsvdSecCnt << ", " << BPB_NumFATs << ", "
         << BPB_FATSz32 << ", " << BPB_RootClus << ", "
         << signature << endl;
//    cout << "FFAT ends at 0x549800? == " << (BPB_RsvdSecCnt + (BPB_FATSz32 * BPB_NumFATs)) << endl;

    const unsigned long Partition_LBA_Begin = 0; ///< @todo get this passed in?
    m_fatBeginLBA = Partition_LBA_Begin + BPB_RsvdSecCnt;
    m_clusterBeginLBA = Partition_LBA_Begin + BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
    m_sectorsPerCluster = BPB_SecPerClus;
    m_rootDirFirstCluster = BPB_RootClus;
    cout << "FFAT begin LBA = 0x" << hex << m_fatBeginLBA << dec << endl;
    cout << "Cluster begin LBA = 0x" << hex << m_clusterBeginLBA << dec << endl;
    cout << "Sectors Per Cluster" << m_sectorsPerCluster << endl;

    return ((BPB_BytsPerSec==512) && (BPB_NumFATs==2) && (signature==0xAA55));
}


bool Fat32::convertToFsInfo(const ByteArray &block)
{
    const unsigned int sig1 = Read32Bits(block, 0x000); // FS information sector signature (0x52 0x52 0x61 0x41 = "RRaA")
    // 0x004 for 480 bytes reserved
    const unsigned int sig2 = Read32Bits(block, 0x1E4); // FS information sector signature (0x72 0x72 0x41 0x61 = "rrAa")
    const unsigned int freeClusters = Read32Bits(block, 0x1E8); // Last known number of free data clusters on the volume, or 0xFFFFFFFF if unknown
    const unsigned int lastAllocatedCluster = Read32Bits(block, 0x1EC); // Number of the most recently known to be allocated data cluster. Should be set to 0xFFFFFFFF during format.
    cout << "Free clusters=" << freeClusters << endl;
    cout << "Last allocated cluster=" << lastAllocatedCluster << endl;
    // 0x1F0 for 12 bytes reserved
    const unsigned int sig3 = Read32Bits(block, 0x1FC); // FS information sector signature (0x00 0x00 0x55 0xAA)

    return (sig1 == 0x41615252) && (sig2 == 0x61417272) && (sig3 == 0xaa550000);
}


size_t Fat32::nextCluster(size_t clusterNumber)
{
    // Determine the sector of FAT that the current cluster is in
    const size_t sectorOfFat = clusterNumber >> 7;
    const size_t offsetInFat = clusterNumber & 0x7F;

    ByteArray block = readLBA(m_fatBeginLBA + sectorOfFat);
    const size_t next = Read32Bits(block, offsetInFat * 4);

    return next;
}


bool Fat32::copyFile(std::ostream &s, size_t startCluster, unsigned long long bytesToCopy)
{
    // Some sanity checks
    if (startCluster == 0 && bytesToCopy == 0)
        return true; // This is an empty file, just say everything is fine
    else if (startCluster == 0 && bytesToCopy > 0)
        return false; // No sensible start cluster but had a file size?

    ByteArray block;
    size_t currentCluster = startCluster;
    while (bytesToCopy > 0 && currentCluster < 0x0FFFFFFF) // Apparently FAT32 would be 0xFFFFFFF8 or greater. XTV is different?
    {
        block = readCluster(currentCluster);
        const size_t bytesThisBlock =  (bytesToCopy > block.size()) ? block.size() : bytesToCopy;
        s.write((const char*)&block[0], bytesThisBlock);

        bytesToCopy -= bytesThisBlock;
        currentCluster = nextCluster(currentCluster);
    }

    // Final sanity check
    return (bytesToCopy == 0 && currentCluster >= 0x0FFFFFFF);
}


bool Fat32::copyFile(std::ostream &s, const std::string &path)
{
    const DirEntry fileInfo = infoFor(path);

    if (fileInfo.filename.empty() || fileInfo.isDirectory() || fileInfo.firstCluster == 0)
        return false;
    else
    {
#ifdef DEBUG
        cout << "About to copy " << path << " of " << fileInfo.filesize << " bytes from cluster " << fileInfo.firstCluster << endl;
#endif
        return copyFile(s, fileInfo.firstCluster, fileInfo.filesize);
    }
}


bool Fat32::copyFile(const std::string &srcPath, const std::string &destPath)
{
    std::ofstream f(destPath.c_str(), std::ios::out | std::ios::binary);
    if (!f)
        return false;

    const bool okay = copyFile(f, srcPath);
    f.close();

    return okay;
}



// ===========================================================================
// ==                    X T V F S   C L A S S                              ==
// ===========================================================================

bool Xtvfs::open(const std::string &filepath)
{
    if (!inherited::open(filepath))
        return false;

    bool okay = true;
    ByteArray block = readLBA(2);
    const unsigned int xfs = Read32Bits(block, 0x00); // XFS marker: 58 46 53 30 = "XFS0"
    //const unsigned int num = Read32Bits(block, 0x64); // 0x0000034c == 844, or 76 and 3
    if (xfs != 0x30534658)
        return false;

    cout << "XFS marker found" << endl;

    return okay;
}


bool Xtvfs::convertToVolumeId(const ByteArray &block)
{
    bool okay = inherited::convertToVolumeId(block);

    m_vfatBeginLBA = m_fatBeginLBA + (BPB_FATSz32*BPB_NumFATs);
    cout << "FFAT begin LBA = 0x" << hex << m_fatBeginLBA << dec << endl;
    cout << "Cluster begin LBA = 0x" << hex << m_clusterBeginLBA << dec << endl;
    cout << "Sectors Per Cluster" << m_sectorsPerCluster << endl;
    cout << "VFAT begin LBA = 0x" << hex << m_vfatBeginLBA << dec << endl;

    // First get a rough estimate (2% of the partition's total number of sectors)
    const double lbaVidEstimate = BPB_TotSec32 * 0.02;

    // Get the file cluster where the video area starts
    // Note that this is why it was an estimate before,
    // we need to round up to the next cluster
    m_vdataBeginLBA = ( ceil((lbaVidEstimate - m_clusterBeginLBA)/  BPB_SecPerClus)
                        * BPB_SecPerClus) + m_clusterBeginLBA;
    cout << "VDATA begin LBA = 0x" << hex << m_vdataBeginLBA << dec << endl;

    return okay;
}

FileSystem::ByteArray Xtvfs::readVideoCluster(size_t clusterNumber)
{
    const size_t vsectorsPerCluster = 3008;
    const size_t lbaAddr = m_vdataBeginLBA + (clusterNumber - 2) * vsectorsPerCluster;
    return readLBA(lbaAddr, vsectorsPerCluster);
}


size_t Xtvfs::nextVideoCluster(size_t clusterNumber)
{
    // Determine the sector of FAT that the current cluster is in
    const size_t sectorOfFat = clusterNumber >> 7;
    const size_t offsetInFat = clusterNumber & 0x7F;

    ByteArray block = readLBA(m_vfatBeginLBA + sectorOfFat);
    const size_t next = Read32Bits(block, offsetInFat * 4);

    return next;
}


const size_t lastClusterMarker = 0xfffffff;

/// The cluster size for this FAT is 3008 sectors (47 * the file cluster size of 64). This was chosen as it is a multiple of 188 (the size of a transport stream packet) allowing exactly 8192 packets to fit in each cluster without any crossover.
const size_t vfatClusterSize = 0x178000;

/// Follow the video fat chain and check it makes sense
bool Xtvfs::verifyVideoChain(size_t clusterNumber, size_t filesize)
{
    const size_t expectedChainLength = (filesize / vfatClusterSize) + 1;

    vector<size_t> chain;
    size_t currentCluster = clusterNumber;
    while (currentCluster != lastClusterMarker &&
           chain.size() <= expectedChainLength)
    {
        chain.push_back(currentCluster);
        currentCluster = nextVideoCluster(currentCluster);
        // Check for loops:
        if (find(chain.begin(), chain.end(), currentCluster) != chain.end())
        {
            cerr << "Found a loop!" << endl;
            break;
        }
    }

    return (currentCluster == lastClusterMarker) &&
           (chain.size() == expectedChainLength);
}


bool Xtvfs::copyFile(std::ostream &s, const std::string &path)
{
    const DirEntry fileInfo = infoFor(path);

    if (fileInfo.filename.empty() || fileInfo.isDirectory() || fileInfo.firstCluster == 0)
        return false;
    else if (fileInfo.isDevice())
    {
#ifdef DEBUG
        cout << "About to copy video " << path << " of " << fileInfo.filesize << " bytes from cluster " << fileInfo.firstCluster << endl;
#endif
        return copyVideoFile(s, fileInfo.firstCluster, fileInfo.filesize);
    }
    else
    {
#ifdef DEBUG
        cout << "About to copy " << path << " of " << fileInfo.filesize << " bytes from cluster " << fileInfo.firstCluster << endl;
#endif
        return inherited::copyFile(s, fileInfo.firstCluster, fileInfo.filesize);
    }
}


bool Xtvfs::copyFile(const std::string &srcPath, const std::string &destPath)
{
    const DirEntry fileInfo = infoFor(srcPath);

    if (fileInfo.filename.empty() || fileInfo.isDirectory() || fileInfo.firstCluster == 0)
        return false;
    else if (fileInfo.isDevice())
    {
#ifdef DEBUG
        cout << "About to copy video " << srcPath << " of " << fileInfo.filesize << " bytes from cluster " << fileInfo.firstCluster << endl;
#endif
//        return copyVideoFile(s, fileInfo.firstCluster, fileInfo.filesize);
        return copyVideoFile(destPath, fileInfo.firstCluster, fileInfo.filesize);
    }
    else
    {
        return inherited::copyFile(srcPath, destPath);
    }
}


bool Xtvfs::copyVideoFile(std::ostream &s, size_t startCluster, unsigned long long bytesToCopy)
{
    // Some sanity checks
    if (startCluster == 0 && bytesToCopy == 0)
        return true; // This is an empty file, just say everything is fine
    else if (startCluster == 0 && bytesToCopy > 0)
        return false; // No sensible start cluster but had a file size?

    ByteArray block;
    size_t currentCluster = startCluster;
    while (bytesToCopy > 0 && currentCluster < 0x0FFFFFFF) // Apparently FAT32 would be 0xFFFFFFF8 or greater. XTV is different?
    {
        block = readVideoCluster(currentCluster);
        const size_t bytesThisBlock =  (bytesToCopy > block.size()) ? block.size() : bytesToCopy;
        s.write((const char*)&block[0], bytesThisBlock);

        bytesToCopy -= bytesThisBlock;
        currentCluster = nextVideoCluster(currentCluster);
    }

    // Final sanity check
    return (bytesToCopy == 0 && currentCluster >= 0x0FFFFFFF);
}


bool Xtvfs::copyVideoFile(const std::string &dest, size_t startCluster, unsigned long long bytesToCopy)
{
    cout << "copyVideoFile(" << dest << "," << startCluster << "," << humanReadableByteCount(bytesToCopy) << ")" << endl;
    cout << "using fopen to save the file" << endl;
    FILE *s = fopen(dest.c_str(), "wb");
    if (s == 0)
        return false;

    // Some sanity checks
    if (startCluster == 0 && bytesToCopy == 0)
        return true; // This is an empty file, just say everything is fine
    else if (startCluster == 0 && bytesToCopy > 0)
        return false; // No sensible start cluster but had a file size?
 unsigned long long bytesCopied = 0;
    ByteArray block;
    size_t currentCluster = startCluster;
//    while (bytesToCopy > 0 && currentCluster < 0x0FFFFFFF) // Apparently FAT32 would be 0xFFFFFFF8 or greater. XTV is different?
    while (currentCluster < 0x0FFFFFFF) // Apparently FAT32 would be 0xFFFFFFF8 or greater. XTV is different?
    {
        block = readVideoCluster(currentCluster);
        const size_t bytesThisBlock =  (bytesToCopy > block.size()) ? block.size() : bytesToCopy;
        const size_t ret = fwrite((const char*)&block[0], 1, bytesThisBlock, s);
        if (ret != bytesThisBlock)
            cout << "Writing error: " << ret << "  " << errno << endl;
        else
            bytesCopied += ret;

//        bytesToCopy -= bytesThisBlock;
        currentCluster = nextVideoCluster(currentCluster);
    }

    fclose(s);
cout << "Copied " << humanReadableByteCount(bytesCopied) << " bytes" << endl;
    // Final sanity check
    return (bytesToCopy == 0 && currentCluster >= 0x0FFFFFFF);
}


std::list<size_t> Xtvfs::getAllocationChain(const std::string &srcPath)
{
    std::list<size_t> result;

    const DirEntry fileInfo = infoFor(srcPath);

    if (fileInfo.filename.empty() || fileInfo.isDirectory() || fileInfo.firstCluster == 0)
        return result;
    else if (fileInfo.isDevice())
    {
        cout << "Following video cluster chain for " << srcPath << endl;
          size_t currentCluster = fileInfo.firstCluster;
          while (currentCluster < 0x0FFFFFFF) // Apparently FAT32 would be 0xFFFFFFF8 or greater. XTV is different?
          {
              result.push_back(currentCluster);
              currentCluster = nextVideoCluster(currentCluster);
          }

cout << "Returning list of " << result.size() << " video clusters: would be " << humanReadableByteCount((unsigned long long)result.size() * 3008 * 512) << endl;
          return result;
    }
    else
    {
//        return inherited::copyFile(srcPath, destPath);
        return result;
    }
}
