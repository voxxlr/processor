///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Geist Software Labs. All rights reserved.

///////////////////////////////////////////////////////////////////////////

#ifndef _Task
#define _Task

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/detail/thread_id.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/function.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/log/support/date_time.hpp>


namespace task
{
	BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);
	BOOST_LOG_ATTRIBUTE_KEYWORD(thread_name, "ThreadName", std::string)

	struct MemoryInfo
	{
		std::size_t totalRam{0};
		std::size_t freeRam{0};
		std::size_t availableRam{0};
		std::size_t totalSwap{0};
		std::size_t freeSwap{0};
	};

	#if defined(__LINUX__)
	unsigned long linuxGetAvailableRam()
	{
		std::string token;
		std::ifstream file("/proc/meminfo");
		while(file >> token) {
			if(token == "MemAvailable:") {
				unsigned long mem;
				if(file >> mem) {
					// read in kB and convert to bytes
					return mem * 1024;
				} else {
					return 0;
				}
			}
			// ignore rest of the line
			file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		return 0; // nothing found
	}
	#endif

	void initialize(std::string iName, char* iConfig, boost::function<bool(json_spirit::mObject&)> iMessageHandler)
	{

		MemoryInfo infos;

		#if defined(__WINDOWS__)
			MEMORYSTATUS memory;
			GlobalMemoryStatus(&memory);

			// memory.dwMemoryLoad;
			infos.totalRam = memory.dwTotalPhys;
			infos.availableRam = infos.freeRam = memory.dwAvailPhys;
			// memory.dwTotalPageFile;
			// memory.dwAvailPageFile;
			infos.totalSwap = memory.dwTotalVirtual;
			infos.freeSwap = memory.dwAvailVirtual;
		#elif defined(__LINUX__)
			struct sysinfo sys_info;
			sysinfo(&sys_info);

			infos.totalRam = sys_info.totalram * sys_info.mem_unit;
			infos.freeRam = sys_info.freeram * sys_info.mem_unit;

			infos.availableRam = linuxGetAvailableRam();
			if(infos.availableRam == 0)
				infos.availableRam = infos.freeRam;

			infos.totalSwap = sys_info.totalswap * sys_info.mem_unit;
			infos.freeSwap = sys_info.freeswap * sys_info.mem_unit;
		#endif

		boost::log::add_file_log
		(
			boost::log::keywords::file_name = iName+std::string(".log"),
			boost::log::keywords::auto_flush = true,
			boost::log::keywords::format =
			(
				boost::log::expressions::stream
					<< thread_name << " - "
					<< boost::log::expressions::format_date_time(timestamp, "%m-%d %H:%M:%S")
					<< ": <" << boost::log::trivial::severity
					<< "> " << boost::log::expressions::smessage
			)
		);

		boost::log::core::get()->set_filter
		(
			boost::log::trivial::severity >= boost::log::trivial::info
		);
		boost::log::add_common_attributes();

		//
		// read json parameter
		//
		std::cout << boost::filesystem::current_path();
		BOOST_LOG_TRIVIAL(info) << "P  " << boost::filesystem::current_path();
		BOOST_LOG_TRIVIAL(info) << "C  " << iConfig;
		json_spirit::mValue lValue;
		std::stringstream lStream(iConfig);
		json_spirit::read_stream(lStream, lValue);
		json_spirit::mObject& lObject = lValue.get_obj();
		
		iMessageHandler(lObject);
	}
};

#endif
