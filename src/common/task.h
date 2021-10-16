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
#include <boost/log/sources/record_ostream.hpp>

#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/detail/thread_id.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <boost/function.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/log/support/date_time.hpp>

#if defined(WIN32)

#include <windows.h>

unsigned long long availableMemory()
{
	MEMORYSTATUSEX memory = { sizeof memory };
	GlobalMemoryStatusEx(&memory);
	return memory.ullAvailPhys;
}


#elif defined (LINUX)

#include <sys/sysinfo.h>

unsigned long long availableMemory()
{
	struct sysinfo sys_info;
	sysinfo(&sys_info);
	return sys_info.freeram * sys_info.mem_unit;
}

#endif

#include <thread>


namespace task
{
	BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);
	BOOST_LOG_ATTRIBUTE_KEYWORD(thread_name, "ThreadName", std::string)

	void initialize(std::string iName, char* iConfig, boost::function<bool(json_spirit::mObject&)> iMessageHandler)
	{
		#ifdef WIN32
		boost::log::add_console_log(std::cout, boost::log::keywords::format = ">> %Message%");
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
