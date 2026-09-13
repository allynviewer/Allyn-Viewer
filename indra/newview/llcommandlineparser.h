/** 
 * @file llcommandlineparser.h
 * @brief LLCommandLineParser class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_LLCOMMANDLINEPARSER_H
#define LL_LLCOMMANDLINEPARSER_H
#include <boost/function/function1.hpp>
namespace boost
{
	namespace program_options
	{
		template <class charT> class basic_command_line_parser;
		typedef basic_command_line_parser<char> command_line_parser;
	}
}
class LLCommandLineParser
{
public:
	typedef std::vector< std::string > token_vector_t;
	void addOptionDesc(
					   const std::string& option_name,
					   boost::function1<void, const token_vector_t&> notify_callback = 0,
					   unsigned int num_tokens = 0,
					   const std::string& description = LLStringUtil::null,
					   const std::string& short_name = LLStringUtil::null,
					   bool composing = false,
					   bool positional = false,
					   bool last_option = false);
	bool parseCommandLine(int argc, char **argv);
	bool parseCommandLineString(const std::string& str);
	bool parseCommandLineFile(const std::basic_istream< char >& file);
	void notify();
	std::ostream& printOptionsDesc(std::ostream& os) const;
	bool hasOption(const std::string& name) const;
	const token_vector_t& getOption(const std::string& name) const;
	void printOptions() const;
	const std::string& getErrorMessage() const { return mErrorMsg; }
	typedef boost::function1<std::pair<std::string, std::string>, const std::string&> parser_func;
	void setCustomParser(parser_func f) { mExtraParser = f; }
private:
	bool parseAndStoreResults(boost::program_options::command_line_parser& clp);
	std::string mErrorMsg;
	parser_func mExtraParser;
};
inline std::ostream& operator<<(std::ostream& out, const LLCommandLineParser& clp)
{
    return clp.printOptionsDesc(out);
}
class LLControlGroup;
class LLControlGroupCLP : public LLCommandLineParser
{
public:
	void configure(const std::string& config_filename,
				   LLControlGroup* controlGroup);
};
#endif
