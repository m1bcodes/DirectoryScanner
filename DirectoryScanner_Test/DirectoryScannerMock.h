//MIT License
//
//Copyright(c) 2022 m1bcodes
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this softwareand associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright noticeand this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#pragma once
#include "DirectoryScanner.h"

#include <regex>
#include <filesystem>

class CDirectoryScannerMock :
    public CDirectoryScanner
{
public:
    CDirectoryScannerMock() 
        : CDirectoryScanner()
    {}

    CDirectoryScannerMock(bool nozip, bool crcCheck, const std::vector<std::string>& filespecs, const std::vector<std::string>& excludeFilespecs)
        : CDirectoryScanner(nozip, crcCheck, filespecs, excludeFilespecs)
    {}

    virtual ~CDirectoryScannerMock() = default;

    virtual void process_file(const boost::filesystem::path& p, const boost::filesystem::path& logicalFilename, crc_t crc) override;

    void listFilesFound(std::filesystem::path startPath) const
    {
        for (const auto& fr : scannedFileInfo)
        {
            std::cout << std::filesystem::relative(fr.logicalFilename, startPath) << "\n";
        }
        std::cout << std::dec << scannedFileInfo.size() << " files found.\n";
    }

    struct FileRecord
    {
        std::string path;
        std::string logicalFilename;
        crc_t crc;
        int fileNo;
    };
    std::vector<FileRecord> scannedFileInfo;

    void testNoZip(bool nozip)
    {
        ASSERT_EQ(nozip, m_nozip);
    }

    void testCrcCheck(bool crcCheck)
    {
        ASSERT_EQ(crcCheck, m_crcCheck);
    }

    void testIncludeSpec(const std::vector<std::string>& filespecs)
    {
        ASSERT_EQ(filespecs, m_filespecs);
    }

    void testExcludeSpec(const std::vector<std::string>& excludeSpecs)
    {
        ASSERT_EQ(excludeSpecs, m_excludeFilespecs);
    }

};

