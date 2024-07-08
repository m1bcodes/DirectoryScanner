//MIT License
//
//Copyright(c) 2022-2024 m1bcodes
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


#include "pch.h"
#include "DirectoryScannerMock.h"
#include <fstream>

void CDirectoryScannerMock::process_file(const std::filesystem::path& p, const std::filesystem::path& logicalFilename, crc_t crc)
{
    std::cout << "processFile: path:" << p << ", filename: " << logicalFilename << ", crc: " << crc << "\n";
    FileRecord fr;
    fr.path = p.string();
    fr.logicalFilename = logicalFilename.string();
    fr.crc = crc;

    std::ifstream ifs(p.string());
    ASSERT_TRUE(ifs);
    if (ifs) {
        std::string line;
        std::getline(ifs, line);
        if (line.empty())
        {
            ASSERT_TRUE(false);
        }
        std::regex re("\\d+");
        std::smatch matchContent;
        if (!std::regex_search(line, matchContent, re))
        {
            ASSERT_TRUE(false) << "File content is '" << line << "'";
        }
        ASSERT_EQ(matchContent.size(), 1);

        std::smatch matchName;
        std::string filename = logicalFilename.filename().string();
        ASSERT_TRUE(std::regex_search(filename, matchName, re));
        ASSERT_EQ(matchName.size(), 1);

        if (matchContent[0] != matchName[0])
        {
            ASSERT_TRUE(false);
        }
        EXPECT_EQ(matchContent[0], matchName[0]);

        fr.fileNo = std::atoi(matchName[0].str().c_str());
    }

    scannedFileInfo.push_back(fr);
}
