/**
 * @file aialert.h
 * @brief Declaration of AIArgs and AIAlert classes.
 *
 * Copyright (c) 2013, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   02/11/2013
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   05/11/2013
 *   Moved everything in namespace AIAlert, except AIArgs.
 */
#ifndef AI_ALERT
#define AI_ALERT
#include "llpreprocessor.h"
#include "llstring.h"
#include <deque>
#include <exception>
#define   THROW_ALERT_CLASS(Alert, ...) throw Alert(AIAlert::Prefix(),                                                     AIAlert::not_modal, __VA_ARGS__)
#define  THROW_MALERT_CLASS(Alert, ...) throw Alert(AIAlert::Prefix(),                                                         AIAlert::modal, __VA_ARGS__)
#ifdef __GNUC__
#define  THROW_FALERT_CLASS(Alert, ...) throw Alert(AIAlert::Prefix(__PRETTY_FUNCTION__, AIAlert::pretty_function_prefix), AIAlert::not_modal, __VA_ARGS__)
#define THROW_FMALERT_CLASS(Alert, ...) throw Alert(AIAlert::Prefix(__PRETTY_FUNCTION__, AIAlert::pretty_function_prefix),     AIAlert::modal, __VA_ARGS__)
#else
#define  THROW_FALERT_CLASS(Alert, ...) throw Alert(AIAlert::Prefix(__FUNCTION__, AIAlert::pretty_function_prefix), AIAlert::not_modal, __VA_ARGS__)
#define THROW_FMALERT_CLASS(Alert, ...) throw Alert(AIAlert::Prefix(__FUNCTION__, AIAlert::pretty_function_prefix),     AIAlert::modal, __VA_ARGS__)
#endif
#define   THROW_ALERT(...)   THROW_ALERT_CLASS(AIAlert::Error, __VA_ARGS__)
#define  THROW_MALERT(...)  THROW_MALERT_CLASS(AIAlert::Error, __VA_ARGS__)
#define  THROW_FALERT(...)  THROW_FALERT_CLASS(AIAlert::Error, __VA_ARGS__)
#define THROW_FMALERT(...) THROW_FMALERT_CLASS(AIAlert::Error, __VA_ARGS__)
#define   THROW_ALERTC(...)   THROW_ALERT_CLASS(AIAlert::ErrorCode, __VA_ARGS__)
#define  THROW_MALERTC(...)  THROW_MALERT_CLASS(AIAlert::ErrorCode, __VA_ARGS__)
#define  THROW_FALERTC(...)  THROW_FALERT_CLASS(AIAlert::ErrorCode, __VA_ARGS__)
#define THROW_FMALERTC(...) THROW_FMALERT_CLASS(AIAlert::ErrorCode, __VA_ARGS__)
#define   THROW_ALERTE(...) do { int errn = errno;   THROW_ALERT_CLASS(AIAlert::ErrorCode, errn, __VA_ARGS__); } while(0)
#define  THROW_MALERTE(...) do { int errn = errno;  THROW_MALERT_CLASS(AIAlert::ErrorCode, errn, __VA_ARGS__); } while(0)
#define  THROW_FALERTE(...) do { int errn = errno;  THROW_FALERT_CLASS(AIAlert::ErrorCode, errn, __VA_ARGS__); } while(0)
#define THROW_FMALERTE(...) do { int errn = errno; THROW_FMALERT_CLASS(AIAlert::ErrorCode, errn, __VA_ARGS__); } while(0)
#ifdef EXAMPLE_CODE
	catch (AIAlert::Error const& error)
    {
	   AIAlert::add(error);
	}
	catch (AIAlert::ErrorCode const& error)
    {
	   if (error.getCode() != EEXIST)
	   {
		 AIAlert::add(alert, AIAlert::pretty_function_prefix);
	   }
	}
	THROW_ALERT("ExampleKey");
	THROW_ALERT("ExampleKey", AIArgs("[FIRST]", first)("[SECOND]", second)(...etc...));
	THROW_ALERT("ExampleKey", error);
	THROW_ALERT(error, "ExampleKey");
	THROW_ALERT("ExampleKey", AIArgs("[FIRST]", first)("[SECOND]", second), error);
	THROW_ALERT(error, "ExampleKey", AIArgs("[FIRST]", first)("[SECOND]", second));
	THROW_MFALERT("ExampleKey", AIArgs("[FIRST]", first));
	THROW_FALERTE("ExampleKey", AIArgs("[FIRST]", first));
#endif
class LL_COMMON_API AIArgs
{
  private:
	LLStringUtil::format_map_t mArgs;
  public:
	AIArgs(void) { }
	AIArgs(char const* key, std::string const& replacement) { mArgs[key] = replacement; }
	AIArgs& operator()(char const* key, std::string const& replacement) { mArgs[key] = replacement; return *this; }
	~AIArgs() noexcept { }
	LLStringUtil::format_map_t const& operator*() const { return mArgs; }
};
namespace AIAlert
{
enum modal_nt
{
  not_modal,
  modal
};
enum alert_line_type_nt
{
  normal = 0,
  empty_prefix = 1,
  pretty_function_prefix = 2
};
class LL_COMMON_API Prefix
{
  public:
	Prefix(void) : mType(empty_prefix) { }
	Prefix(char const* str, alert_line_type_nt type) : mStr(str), mType(type) { }
	operator bool(void) const { return mType != empty_prefix; }
	alert_line_type_nt type(void) const { return mType; }
	std::string const& str(void) const { return mStr; }
  private:
	std::string mStr;
	alert_line_type_nt mType;
};
class LL_COMMON_API Line
{
  private:
	bool mNewline;
	std::string mXmlDesc;
	AIArgs mArgs;
	alert_line_type_nt mType;
  public:
	Line(std::string const& xml_desc, bool newline = false) : mNewline(newline), mXmlDesc(xml_desc), mType(normal) { }
	Line(std::string const& xml_desc, AIArgs const& args, bool newline = false) : mNewline(newline), mXmlDesc(xml_desc), mArgs(args), mType(normal) { }
	Line(Prefix const& prefix, bool newline = false) : mNewline(newline), mXmlDesc("AIPrefix"), mArgs("[PREFIX]", prefix.str()), mType(prefix.type()) { }
	~Line() noexcept { }
	void set_newline(void) { mNewline = true; }
	std::string getXmlDesc(void) const { return mXmlDesc; }
	LLStringUtil::format_map_t const& args(void) const { return *mArgs; }
	bool prepend_newline(void) const { return mNewline; }
	bool suppressed(unsigned int suppress_mask) const { return (suppress_mask & mType) != 0; }
	bool is_prefix(void) const { return mType != normal; }
};
class LL_COMMON_API Error : public std::exception
{
  public:
	typedef std::deque<Line> lines_type;
	~Error() noexcept { }
	lines_type const& lines(void) const { return mLines; }
	bool is_modal(void) const { return mModal == modal; }
	Error(Prefix const& prefix, modal_nt type, Error const& alert);
	Error(Prefix const& prefix, modal_nt type,
		  std::string const& xml_desc, AIArgs const& args = AIArgs());
	Error(Prefix const& prefix, modal_nt type,
		  Error const& alert,
		  std::string const& xml_desc, AIArgs const& args = AIArgs());
	Error(Prefix const& prefix, modal_nt type,
		  std::string const& xml_desc,
		  Error const& alert);
	Error(Prefix const& prefix, modal_nt type,
		  std::string const& xml_desc, AIArgs const& args,
		  Error const& alert);
  private:
	lines_type mLines;
	modal_nt mModal;
};
class LL_COMMON_API ErrorCode : public Error
{
  private:
	int mCode;
  public:
	~ErrorCode() noexcept { }
	int getCode(void) const { return mCode; }
	ErrorCode(Prefix const& prefix, modal_nt type, int code,
	          Error const& alert) :
	  Error(prefix, type, alert), mCode(code) { }
	ErrorCode(Prefix const& prefix, modal_nt type, int code,
		      std::string const& xml_desc, AIArgs const& args = AIArgs()) :
	  Error(prefix, type, xml_desc, args), mCode(code) { }
	ErrorCode(Prefix const& prefix, modal_nt type, int code,
		      Error const& alert,
		      std::string const& xml_desc, AIArgs const& args = AIArgs()) :
	  Error(prefix, type, alert, xml_desc, args), mCode(code) { }
	ErrorCode(Prefix const& prefix, modal_nt type, int code,
		      std::string const& xml_desc,
		      Error const& alert) :
	  Error(prefix, type, xml_desc, alert), mCode(code) { }
	ErrorCode(Prefix const& prefix, modal_nt type, int code,
		      std::string const& xml_desc, AIArgs const& args,
		      Error const& alert) :
	  Error(prefix, type, xml_desc, args, alert), mCode(code) { }
};
}
#endif
