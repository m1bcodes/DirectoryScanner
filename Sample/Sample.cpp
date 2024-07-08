// ConfigCrawler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <string>

#include "DirectoryScanner.h"
#include <boost/program_options.hpp>


class FileLister : public CDirectoryScanner
{
	virtual void process_file(const std::filesystem::path& p, const std::filesystem::path& logicalFilename, crc_t crc) override
	{
		std::cout << "\n";
		std::cout << p << "\n";
		std::cout << logicalFilename << "\n";
		std::cout << "\n";
	}

	//virtual std::ostream& logs(int indent = 0) override
	//{
	//	std::ostream cnull(0);
	//	return cnull;
	//}
};

FileLister cds;
std::vector<std::string> searchPaths;

void help(const boost::program_options::options_description& desc)
{
	std::cout << desc << "\n";
}

void parse_command_line(int argc, char* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("available options");
	desc.add_options()
		("help,h", "produce help message");
	desc.add_options()
		("search-path,p", po::value< std::vector<std::string> >(&searchPaths),
			"search path. Can be a single file, archive or directory");
	desc.add(*cds.createCommandLineOptions());

	po::positional_options_description p;
	p.add("search-path", -1);

	po::parsed_options parsed_options =
		po::command_line_parser(argc, argv).options(desc).positional(p).run();

	po::variables_map vm;
	po::store(parsed_options, vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		help(desc);
		exit(1);
	}
}

int main(int argc, char* argv[])
{
	try {
		parse_command_line(argc, argv);
		for (auto s : searchPaths)
		{
			cds.scanPath(s);
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}	
}

