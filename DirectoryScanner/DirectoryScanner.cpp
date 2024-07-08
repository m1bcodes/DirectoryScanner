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

#include "pch.h"
#include "DirectoryScanner.h"

#include <7zpp/7zpp.h>

#include <exception>
#include <iostream>
#include <fstream>
#include <random>

#include <boost/crc.hpp>
#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

CDirectoryScanner::CDirectoryScanner()
	: logIndent(0)
	, m_nozip(false)
	, m_crcCheck(false)
	, m_filespecs({ ".*" })
	, m_excludeFilespecs({""})
{
	initialize7zDllPath();
}

CDirectoryScanner::CDirectoryScanner(bool nozip, bool crcCheck, const std::vector<std::string>& filespecs, const std::vector<std::string>& excludeFilespecs)
	: logIndent(0)
	, m_nozip(nozip)
	, m_crcCheck(crcCheck)
	, m_filespecs(filespecs)
	, m_excludeFilespecs(excludeFilespecs)
{
	initialize7zDllPath();
}

void CDirectoryScanner::initialize7zDllPath()
{
	auto programPath = boost::dll::program_location();
	auto dllPath = programPath.parent_path() / "7z.dll";
	m_7zDllPath = dllPath.string();
}

CDirectoryScanner::~CDirectoryScanner()
{
}

std::vector<std::string> CDirectoryScanner::parseCommandLineArguments(const std::vector<std::string>& arguments)
{
	namespace po = boost::program_options;
	std::unique_ptr<po::options_description> desc = createCommandLineOptions();

	po::parsed_options parsed_options =
		po::command_line_parser(arguments).options(*desc).allow_unregistered().run();

	//// from here:
	//// http://stackoverflow.com/questions/35133523/c-boost-program-options-multiple-lists-of-arguments

	po::variables_map vm;
	po::store(parsed_options, vm);
	po::notify(vm);
	
	std::vector<std::string> to_pass_further = po::collect_unrecognized(parsed_options.options, po::include_positional);
	return to_pass_further;
}

std::unique_ptr<boost::program_options::options_description> CDirectoryScanner::createCommandLineOptions()
{
	namespace po = boost::program_options;
	std::unique_ptr<po::options_description> desc = std::make_unique<po::options_description>("Directory scanner options");
	desc->add_options()
		("filespec,f", po::value< std::vector<std::string> >(&m_filespecs)->multitoken(),
			"file specification (regexp). "
			"Process only files matching any of these file specifications.")
		("exclude,e", po::value< std::vector<std::string> >(&m_excludeFilespecs)->multitoken(),
			"file specification (regexp). "
			"Do not process files matching any of these file specifications.")
		("nozip,n", po::value<bool>(&m_nozip)->zero_tokens(),
			"do not recurse into archives files")
		("checkcrc,c", po::value<bool>(&m_crcCheck)->zero_tokens(),
			"Do not calculate crc of files prior to scanning and do not skip"
			"scanning if file has already been scanned.")
		("7zdll,7", po::value<std::string>(&m_7zDllPath), 
			"path to 7z.dll. If omitted 7z.dll is searched in the folder, where the executable is stored.");
	return desc;
}

void CDirectoryScanner::scanPathRec(const std::filesystem::path& rootPath, int indent)
{
	logs(indent) << "Searching directory " << rootPath << "\n";

	std::filesystem::directory_iterator rdi(rootPath);
	std::filesystem::directory_iterator endIter = std::filesystem::directory_iterator();

	while (rdi != endIter) // 2.
	{
		try {
			// logs << rdi.level() << ": " << rdi->path() << "\n";
			if (std::filesystem::is_regular_file(rdi->status())) {
				dispatch_file(*rdi, rdi->path(), 0);
			}
			if (std::filesystem::is_directory(*rdi)) {
				if (!std::filesystem::is_symlink(*rdi))
				{
					scanPathRec(rdi->path(), indent + 1);
				}
			}
		}
		catch (std::exception& ex)
		{
			logs(0) << "\nError in directory " << rdi->path() << " -- skipped: \n";
			logs(0) << ex.what() << "\n\n";
		}
		rdi++;
	}
}


void CDirectoryScanner::scanPath(const std::filesystem::path& rootPath)
{
	if (!m_nozip) {
		m_7zlib = std::make_unique<SevenZip::SevenZipLibrary>();
		if (!m_7zlib->Load(m_7zDllPath))
			throw std::runtime_error("Error loading 7z.dll from " + m_7zDllPath);
	}

	if (std::filesystem::is_regular_file(rootPath))
	{
		dispatch_file(rootPath, rootPath, 0);
	}
	else
	{
		scanPathRec(rootPath, 0);
	}
}

void CDirectoryScanner::process_file(const std::filesystem::path& p, const std::filesystem::path& logicalFilename, crc_t crc)
{
}

class ListCallBackOutput : public SevenZip::ListCallback
{
public:
	ListCallBackOutput(std::vector<SevenZip::intl::FileInfo>& fileInfos)
		: m_fileInfos(fileInfos)
	{}

	virtual void OnFileFound(const SevenZip::intl::FileInfo& fileInfo) override
	{
		m_fileInfos.push_back(fileInfo);
	}

	std::vector<SevenZip::intl::FileInfo>& m_fileInfos;
};

void CDirectoryScanner::process_7z(const std::filesystem::path& zipPath, const std::filesystem::path& logicalFilename, const std::string& fmtHint)
{
	logs(logIndent) << "searching archive " << zipPath.filename() << std::endl;
	try {
		SevenZip::SevenZipLister lister(*m_7zlib, zipPath.string());

		SevenZip::CompressionFormat::_Enum compressionFormat;
		// Try to detect compression type: for some reason this doesnt work.
		//if (!lister.DetectCompressionFormat()) throw std::exception("Can't detect compression format");
		if (fmtHint == "zip") 
			compressionFormat = SevenZip::CompressionFormat::Zip;
		else if (fmtHint == "7z") 
			compressionFormat = SevenZip::CompressionFormat::SevenZip;
		else if (fmtHint == "tar")
			compressionFormat = SevenZip::CompressionFormat::Tar;
		else if (fmtHint == "gz")
			compressionFormat = SevenZip::CompressionFormat::GZip;
		else if (fmtHint == "xz")
			compressionFormat = SevenZip::CompressionFormat::XZ;
		else if (fmtHint == "bz2")
			compressionFormat = SevenZip::CompressionFormat::BZip2;
		else if (fmtHint == "cab")
			compressionFormat = SevenZip::CompressionFormat::Cab;
		else
			throw std::logic_error("Unknown format: " + fmtHint);

		lister.SetCompressionFormat(compressionFormat);

		std::vector<SevenZip::intl::FileInfo> fileInfos;
		ListCallBackOutput myListCallBack(fileInfos);
		lister.ListArchive("", (SevenZip::ListCallback*) & myListCallBack);
		
		for (size_t i = 0; i < fileInfos.size(); i++)
		{
			SevenZip::FileInfo& fi = fileInfos[i];
			logs() << "Entry " << i << ": " << fi.FileName << " " << (fi.IsDirectory ? "<DIR>" : "") << " " << std::hex << fi.crc << "\n";
			if (fi.IsDirectory) continue;

			std::filesystem::path pathInZip = fi.FileName;
			std::filesystem::path fileInZip = pathInZip.filename();
			size_t fileSize = fi.Size;
			std::string fmtHint;
			EEngine eng = chooseEngine(fileInZip, fmtHint);
			if (eng != engUnknown) {
				if (!m_crcCheck || fi.crc == 0 || crcSet.find(fi.crc) == crcSet.end())
				{
					std::filesystem::path tempPath = std::filesystem::temp_directory_path() / generate_unique_path();
					std::filesystem::create_directories(tempPath);

					logs(logIndent) << "extracting file " << fileInZip << "\n";

					SevenZip::SevenZipExtractor extractor(*m_7zlib, zipPath.string());
					extractor.SetCompressionFormat(compressionFormat);
					
					unsigned int index = static_cast<unsigned int>(i);
					extractor.ExtractFilesFromArchive(&index, 1, tempPath.string());

					try {
						std::filesystem::path tempFilePath = tempPath / fi.FileName;
						dispatch_file(tempFilePath, logicalFilename / pathInZip.string(), fi.crc);
						crcSet.insert(fi.crc);
					}
					catch (const std::exception & ex)
					{
						std::cerr << "Error processing file from archive: " << ex.what() << "\n";
					}
					std::filesystem::remove_all(tempPath);
				}
				else
				{
					// file was already scanned
					logs(logIndent) << "already processed: " << fileInZip << "\n";
				}

			}
		}
	}
	catch (const std::exception & ex)
	{
		std::cerr << "Error processing archive: " << ex.what() << "\n";
	}
}

inline void CDirectoryScanner::dispatch_file(const std::filesystem::path& p, const std::filesystem::path& logicalFilename, crc_t crc)
{
	std::string fmtHint;
	EEngine engine = chooseEngine(logicalFilename, fmtHint);
	logIndent++;
	switch (engine) {
	case engFile:
		if (fileHasNewCrcOrNotChecked(p, crc)) {
			process_file(p, logicalFilename, crc);
		}
		break;
		break;
	case eng7z:
		process_7z(p, logicalFilename, fmtHint);
		break;
	}
	logIndent--;
}

CDirectoryScanner::EEngine CDirectoryScanner::chooseEngine(const std::filesystem::path& p, std::string& fmtHint)
{

	boost::smatch what;

	class CFileFmtInfo
	{
	public:
		CFileFmtInfo(const std::string& pRegex, const std::string& pHint, EEngine pEngine)
			: regex(pRegex)
			, hint(pHint)
			, engine(pEngine)
		{}

		std::string regex;
		std::string hint;
		EEngine engine;
	};

	std::list<CFileFmtInfo> fileFmtInfos;
	fileFmtInfos.push_back(CFileFmtInfo(".*\\.zip", "zip", eng7z));
	fileFmtInfos.push_back(CFileFmtInfo(".*\\.7z", "7z", eng7z));
	fileFmtInfos.push_back(CFileFmtInfo(".*\\.tgz", "gz", eng7z));
	fileFmtInfos.push_back(CFileFmtInfo(".*\\.tar", "tar", eng7z));
	fileFmtInfos.push_back(CFileFmtInfo(".*\\.gz", "gz", eng7z));
	fileFmtInfos.push_back(CFileFmtInfo(".*\\.cab", "cab", eng7z));
	// not working:
	// fileFmtInfos.push_back(CFileFmtInfo(".*\\.bz2", "bz2", eng7z));
	// fileFmtInfos.push_back(CFileFmtInfo(".*\\.xz", "xz", eng7z));

	for (CFileFmtInfo& fmti : fileFmtInfos) {
		const boost::regex filter(fmti.regex, boost::regex_constants::icase);
		if (boost::regex_match(p.filename().string(), what, filter)) {
			if (m_nozip) {
				// we dont process archives, even if nozip is specified.
				fmtHint = "";
				return engUnknown;
			}
			else {
				fmtHint = fmti.hint;
				return fmti.engine;
			}
		}
	}

	bool included = false;
	for (size_t i = 0; i < m_filespecs.size(); i++)
	{
		const boost::regex file_filter(m_filespecs[i], boost::regex_constants::icase);
		if (boost::regex_match(p.filename().string(), what, file_filter)) {
			included = true;
			break;
		}
	}
	if (included) {
		for (size_t i = 0; i < m_excludeFilespecs.size(); i++)
		{
			const boost::regex file_filter(m_excludeFilespecs[i], boost::regex_constants::icase);
			if (boost::regex_match(p.filename().string(), what, file_filter)) {
				included = false;
				break;
			}
		}
	}
	if (included)
		return engFile;
	else 
		return engUnknown;
}

bool CDirectoryScanner::fileHasNewCrcOrNotChecked(const std::filesystem::path& p, crc_t& crc)
{
	if (m_crcCheck) {
		if (crc == 0) {
			// crc is not known: calculate it
			crc = calculate_crc32(p.string());
		}
		else {
			// crc is known (from zip file information):  do nothing
		}
		if (crcSet.find(crc) == crcSet.end()) {
			// crc is new: insert end return true
			crcSet.insert(crc);
			return true;
		}
		else {
			// crc is known
			return false;
		}

	}
	else {
		// dont need to check, treat as new
		return true;	
	}
}

CDirectoryScanner::crc_t CDirectoryScanner::calculate_crc32(std::string filename)
{
	std::ifstream ifs(filename, std::ios::binary);
	if (!ifs) throw std::runtime_error("Can't calculate crc checksum. File not found: " + filename);

	logs(logIndent) << "determining crc...";

	boost::crc_32_type crc;
	const int bufSize = 0xffff;
	char* buf = new char[bufSize];
	while (ifs) {
		ifs.read(buf, bufSize);
		std::streamsize nread = ifs.gcount();
		crc.process_bytes(buf, nread);
	}
	delete[] buf;
	logs() << std::hex << crc.checksum() << std::dec << "\n";
	return crc.checksum();
}

inline std::filesystem::path CDirectoryScanner::generate_unique_path(const std::filesystem::path& base_dir) 
{
	// Use the current time as a seed for random number generation
	auto now = std::chrono::system_clock::now();
	auto seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

	std::mt19937 generator(static_cast<unsigned int>(seed));
	std::uniform_int_distribution<int> distribution(0, 15);
	auto random_char = []() -> char {
		static const char characters[] = "0123456789abcdef";
		static std::mt19937 generator(std::random_device{}());
		static std::uniform_int_distribution<int> distribution(0, 15);
		return characters[distribution(generator)];
		};

	// Generate a unique filename by checking if the file already exists
	std::filesystem::path unique_path;
	do {
		std::string random_filename(16, '\0');
		for (char& c : random_filename) {
			c = random_char();
		}
		unique_path = base_dir / random_filename;
	} while (std::filesystem::exists(unique_path));

	return unique_path;
}

std::ostream& CDirectoryScanner::logs(int indent)
{
	return std::cout;
}

