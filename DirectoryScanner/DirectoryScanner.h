//MIT License
//
//Copyright(c) 2017-2024 m1bcodes
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
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


#include <ostream>
#include <set>
#include <filesystem>

namespace SevenZip {
	class SevenZipLibrary;
}

namespace boost {
	namespace program_options {
		class options_description;
	}
}

class CDirectoryScanner
{
public:
	CDirectoryScanner();
	CDirectoryScanner(bool nozip, bool crcCheck, const std::vector<std::string>& filespecs, const std::vector<std::string>& excludeFilespecs);
	void initialize7zDllPath();
	~CDirectoryScanner();

	void set7zDllPath(const std::filesystem::path& path);

	//! parse command line arguments and return unmatched stuff
	std::vector<std::string> parseCommandLineArguments(const std::vector<std::string>& arguments);
	std::unique_ptr<boost::program_options::options_description> createCommandLineOptions();

	typedef unsigned int crc_t;

	virtual void scanPath(const std::filesystem::path& rootPath);
	virtual void process_file(const std::filesystem::path& p, const std::filesystem::path& logicalFilename, crc_t crc);
	virtual void process_7z(const std::filesystem::path& zipPath, const std::filesystem::path& logicalFilename, const std::string& fmtHint);

protected:
	void scanPathRec(const std::filesystem::path& rootPath, int indent);

	enum EEngine
	{
		engUnknown,
		engFile,
		eng7z
	};

	void dispatch_file(const std::filesystem::path& p, const std::filesystem::path& logicalFilename, crc_t crc);
	EEngine chooseEngine(const std::filesystem::path& p, std::string& fmtHint);
	bool fileHasNewCrcOrNotChecked(const std::filesystem::path& p, crc_t& knownCrc);
	crc_t calculate_crc32(std::string filename);
	std::filesystem::path generate_unique_path(const std::filesystem::path& base_dir = std::filesystem::temp_directory_path());
	virtual std::ostream& logs(int indent = 0);

	int logIndent;
	std::set<crc_t> crcSet;

	bool m_nozip;
	bool m_crcCheck;
	bool m_verbose;
	bool m_quiet;

	std::vector<std::string> m_filespecs;
	std::vector<std::string> m_excludeFilespecs;
	std::string m_7zDllPath;	//!< path to 7z.dll
	std::unique_ptr<SevenZip::SevenZipLibrary> m_7zlib;
};

