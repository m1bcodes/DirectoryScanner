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

const char* testDir = R"(..\Test)";

TEST(DirectoryScanner, ctorNoArguments)
{
	CDirectoryScannerMock cds;
	cds.testCrcCheck(false);
	cds.testNoZip(false);
	cds.testIncludeSpec({ ".*" });
	cds.testExcludeSpec({ "" });
}


TEST(DirectoryScanner, SevenZipDllPath)
{
	CDirectoryScannerMock cds;
	const char* testDllPath = R"(c:\7zip\7z.dll)";
	auto rest = cds.parseCommandLineArguments({ "-7", testDllPath });
	cds.test7zPath(testDllPath);
	ASSERT_TRUE(rest.empty());
}

TEST(DirectoryScanner, commandLineParser)
{
	CDirectoryScannerMock cds;
	auto rest = cds.parseCommandLineArguments({ "--nozip", "--checkcrc", "-f", "A", "B", "C", "-e", "\"D E\""});
	cds.testCrcCheck(true);
	cds.testNoZip(true);
	cds.testIncludeSpec({ "A","B","C"});
	cds.testExcludeSpec({ "\"D E\"" });
	ASSERT_EQ(rest.size(), 0);
}

TEST(DirectoryScanner, commandLineParserWithForeignParameters)
{
	CDirectoryScannerMock cds;
	auto rest = cds.parseCommandLineArguments({ "source1", "source2", "--nozip", "--checkcrc", "-f", "A", "B", "C", "-e", "\"D E\"", "--newopt"});
	cds.testCrcCheck(true);
	cds.testNoZip(true);
	cds.testIncludeSpec({ "A","B","C" });
	cds.testExcludeSpec({ "\"D E\"" });
	ASSERT_EQ(rest.size(), 3);
}

TEST(DirectoryScanner, ctorArguments)
{
	{
		CDirectoryScannerMock cds(false, true, { "A" }, { "B" });
		cds.testCrcCheck(true);
		cds.testNoZip(false);
		cds.testIncludeSpec({ "A" });
		cds.testExcludeSpec({ "B" });
	}
	{
		CDirectoryScannerMock cds(true, false, { "A" }, { "B" });
		cds.testCrcCheck(false);
		cds.testNoZip(true);
		cds.testIncludeSpec({ "A" });
		cds.testExcludeSpec({ "B" });
	}
}

TEST(DirectoryScanner, With_Archives_Without_CRC_check)
{
  CDirectoryScannerMock cds(false, false, { ".*" }, { "" });
  std::filesystem::path startPath = testDir;
  cds.scanPath(startPath.string());

#ifdef _DEBUG
  cds.listFilesFound(startPath);
#endif

  ASSERT_EQ(cds.scannedFileInfo.size(), 44);
}

TEST(DirectoryScanner, Without_Archives_Without_CRC_check)
{
	CDirectoryScannerMock cds(true, false, { ".*" }, { "" });
	std::filesystem::path startPath = testDir;
	cds.scanPath(startPath.string());

#ifdef _DEBUG
	cds.listFilesFound(startPath);
#endif

	ASSERT_EQ(cds.scannedFileInfo.size(), 7);
}

TEST(DirectoryScanner, With_Archives_With_CRC_check)
{
	CDirectoryScannerMock cds(false, true, { ".*" }, { "" });
	std::filesystem::path startPath = testDir;
	cds.scanPath(startPath.string());

#ifdef _DEBUG
	cds.listFilesFound(startPath);
#endif

	std::set<int> fnoSet;
	for (const auto& fr : cds.scannedFileInfo)
	{
		ASSERT_EQ(fnoSet.find(fr.fileNo), fnoSet.end());
		fnoSet.insert(fr.fileNo);
		ASSERT_NE(fr.crc, 0);
	}

	ASSERT_EQ(cds.scannedFileInfo.size(), 16);
}

TEST(DirectoryScanner, IncludePattern)
{
	CDirectoryScannerMock cds(false, true, { "file_[0246]\\..*" }, { "" });
	std::filesystem::path startPath = testDir;
	cds.scanPath(startPath.string());

#ifdef _DEBUG
	cds.listFilesFound(startPath);
#endif

	ASSERT_EQ(cds.scannedFileInfo.size(), 4);
}

TEST(DirectoryScanner, ExcludePattern)
{
	CDirectoryScannerMock cds(false, true, { "file_\\d\\..*" }, { "file_[0246]\\..*" });
	std::filesystem::path startPath = testDir;
	cds.scanPath(startPath.string());

#ifdef _DEBUG
	cds.listFilesFound(startPath);
#endif

	ASSERT_EQ(cds.scannedFileInfo.size(), 4);
}

