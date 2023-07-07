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
#ifndef XTVFS_FILESYSTEM_H
#define XTVFS_FILESYSTEM_H

#include <fstream>
#include <list>
#include <string>
#include <vector>

/// FileSystem access classes and functions
namespace fs
{


class DirEntry
{
  public:
    std::string filename;
    unsigned char attrib;
    size_t firstCluster;
    unsigned long long filesize;  // Something > 4 bytes

    bool isLFN() const { return ((attrib & 0x0F) == 0x0F); }
    bool isReadOnly() const { return attrib & (1 << 0); }
    bool isHidden() const { return attrib & (1 << 1); }
    bool isSystem() const { return attrib & (1 << 2); }
    bool isVolumeId() const { return attrib & (1 << 3); }
    bool isDirectory() const { return attrib & (1 << 4); }
    bool isArchive() const { return attrib & (1 << 5); }

    /// Peculiar to XTVFS - means clusters must be read from VFAT / Video data
    bool isDevice() const { return attrib & (1 << 6); }

    /// Peculiar to XTVFS
    bool isNormal() const { return attrib & (1 << 7); }

    /// Files are marked as deleted by replacing the first character of the name with 0xE5
    bool isDeleted() const { return (unsigned char)filename[0] == 0xE5; }

    /// The last entry in the directory will be zero.
    bool isEndOfList() const { return filename[0] == 0x00; }

    /// Mark an entry as invalid, eg. the last entry in the directory.
    /// This is not part of any standard and specific to this library.
    void invalidate() { filesize = 0xDEADBEEF; firstCluster = 0xDEADBEEF; }

    /// Determine if a directory entry has been marked as invalid
    /// This is not part of any standard and specific to this library.
    bool isValid() const { return filesize != 0xDEADBEEF && firstCluster != 0xDEADBEEF; }

    std::string attribToString(bool pad = true) const;
    std::string toString() const;
};

typedef std::vector<DirEntry> DirEntries;

/// Convert a filename in a human-readable format to the 11-char format, e.g. "main.cpp" to "MAIN    CPP"
std::string to11CharFormat(const std::string &s);

/// Convert a filename in the 11-char format, e.g. "MAIN    CPP" to "main.cpp"
std::string from11CharFormat(const std::string &s);


class FileSystem
{
public:
    typedef std::vector<unsigned char> ByteArray;

    virtual ~FileSystem();

    /// Open an image or disk
    virtual bool open(const std::string &filepath);

    /// Read directory entries from the specified cluster
    virtual DirEntries readDirectory(size_t startCluster = (size_t)-1) = 0;

    /// Retrieve info for a specified path, recursing as required.
    virtual DirEntry infoFor(const std::string &path) = 0;

    /// Copy a file given a path
    virtual bool copyFile(std::ostream &s, const std::string &path) = 0;

    /// Copy a file to a file
    virtual bool copyFile(const std::string &srcPath, const std::string &destPath) = 0;

protected:

    /// Define how many bytes are in a LBA block
    static const size_t lbaBlockSize;

    std::ifstream disk;


    /// Read a logical block
    ByteArray readLBA(size_t lba);

    /// Read a number of logical blocks
    ByteArray readLBA(size_t lba, size_t blocksToRead);

    /// Convert a block into an Master Boot Record (MBR)
    bool convertToMBR(const ByteArray &block);

    /**
     * Read partition data from an MBR block
     * @param mbr The full MBR block to read partition data from
     * @param partition The partition to read, between 0 and 3
     * @return True if things look okay
     */
    bool readPartition(const ByteArray &mbr, unsigned int partition);
};


/// Hints and tips from http://www.pjrc.com/tech/8051/ide/fat32.html.
/// Spec at http://staff.washington.edu/dittrich/misc/fatgen103.pdf
///
class Fat32 : public FileSystem
{
public:

    Fat32();

    /// Open an image or disk
    virtual bool open(const std::string &filepath);

    /// Read directory entries from the specified cluster
    virtual DirEntries readDirectory(size_t startCluster = (size_t)-1);

    /// Retrieve info for a specified path, recursing as required.
    virtual DirEntry infoFor(const std::string &path);

    /// Copy a file to a stream
    virtual bool copyFile(std::ostream &s, const std::string &path);

    /// Copy a file to a file
    virtual bool copyFile(const std::string &srcPath, const std::string &destPath);

protected:
    /// Convert a block into a FAT32 volume ID
    virtual bool convertToVolumeId(const ByteArray &block);

    /// Read a block as FileSystem Info
    bool convertToFsInfo(const ByteArray &block);

    /**
     * Read a directory entry from the block at the offset specified.
     * @param block
     * @param offset
     * @return A directory entry
     */
    virtual DirEntry readDirectoryentry(const ByteArray &block, const int offset);

    /// Read a cluster.
    /// The clusters begin their numbering at 2, so there is no cluster #0 or cluster #1.
    ByteArray readCluster(size_t clusterNumber);

    /// Get the next cluster, given the current cluster
    size_t nextCluster(size_t clusterNumber);

    /// Lower-level access function to copy a chain of blocks
    bool copyFile(std::ostream &s, size_t startCluster, unsigned long long bytesToCopy);


    // The bios parameter block info
    int BPB_BytsPerSec;  ///< Bytes per sector. Always 512
    int BPB_SecPerClus;  ///< Sectors per cluster. 1,2,4,8,16,32,64,128
    int BPB_RsvdSecCnt;  ///< Number of reserved sectors. Usually 0x20
    int BPB_NumFATs;     ///< Number of FATs. Always 2
    int BPB_TotSec32;    ///< Total number of sectors
    int BPB_FATSz32;     ///< Sectors per FAT. Value depends on disk size
    int BPB_RootClus;    ///< Root directory first cluster. Usually 0x00000002

    // Data derived from the BIOS Parameter Block
    unsigned long m_fatBeginLBA;
    unsigned long m_clusterBeginLBA;
    unsigned char m_sectorsPerCluster;
    unsigned long m_rootDirFirstCluster;

private:
    typedef FileSystem inherited;

};


/**
 * The Xtvfs class reads an XTV file system image.
 * This file system is used on Sky+ boxes.
 *
 * From http://forums.digitalspy.co.uk/showthread.php?t=1210077 :
 * The Sky file system is a custom extension of FAT32 called XTVFS.
 * Data is stored in 32K (32,768 byte) clusters for system files and 1.5MB (1,540,096 byte) clusters for video.
 * Additionally the video data is stored outside the standard FAT32 format.
 *
 * XTVFS does have some things in common with FAT32 (enough to allow most files
 * to be read), but the video files (the ones with the .STR extension) are
 * stored differently. One of the differences is the way in which the file size
 * is stored for these files. Windows will only read the first 'half' of the
 * file size number stored on the disk, so what it displays will be the
 * remainder of dividing it by 4GB. For example, a 7GB XTVFS file will show up
 * in windows as 3GB. ExPVR will show the true file size of the recordings.
 *
 * clawson on http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=732407 seems to have xtvfs code
 *
 * http://wiki.ph-mb.com/wiki/XTVFS http://wiki.ph-mb.com/wiki/Video_FAT
 */
class Xtvfs : public Fat32
{
public:

    /// Open an image or disk
    bool open(const std::string &filepath);

    /// Copy a file to a stream
    virtual bool copyFile(std::ostream &s, const std::string &path);

    /// Copy a file to a file
    virtual bool copyFile(const std::string &srcPath, const std::string &destPath);

    /// Utility function to fetch the sectors of the specified path.
    /// This only expected to be used for low-level inspection tools.
    std::list<size_t> getAllocationChain(const std::string &srcPath);

protected:

    /**
     * Read a directory entry from the block at the offset specified.
     * @param block
     * @param offset
     * @return A directory entry
     */
    virtual DirEntry readDirectoryentry(const ByteArray &block, const int offset);


private:
    typedef Fat32 inherited;

    /// Convert a block into a FAT32 volume ID
    virtual bool convertToVolumeId(const ByteArray &block);

    /// Read a video cluster.
    /// The clusters begin their numbering at 2, so there is no cluster #0 or cluster #1.
    ByteArray readVideoCluster(size_t clusterNumber);

    /// Follow the video fat chain and check it makes sense
    bool verifyVideoChain(size_t clusterNumber, size_t filesize);

    /// Get the next video cluster, given the current cluster
    size_t nextVideoCluster(size_t clusterNumber);

    bool copyVideoFile(std::ostream &s, size_t startCluster, unsigned long long bytesToCopy);
    bool copyVideoFile(const std::string &dest, size_t startCluster, unsigned long long bytesToCopy);

    unsigned long m_vfatBeginLBA;
    unsigned long m_vdataBeginLBA;
};

} // end of namespace fs

#endif // XTVFS_FILESYSTEM_H
