/**
 * @file aixml.h
 * @brief XML serialization support.
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
 *   30/07/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIXML_H
#define AIXML_H
#include "llxmltree.h"
#include "llxmlnode.h"
#include "llfile.h"
#include <sstream>
#include "aialert.h"
extern char const* const DEFAULT_LLUUID_NAME;
extern char const* const DEFAULT_MD5STR_NAME;
extern char const* const DEFAULT_LLDATE_NAME;
class LLUUID;
class LLMD5;
class LLDate;
bool md5strFromXML(LLXmlTreeNode* node, std::string& md5str_out);
bool md5strFromXML(LLXmlTreeNode* node, std::string& md5str_out, std::string const& attribute_name);
bool UUIDFromXML(LLXmlTreeNode* node, LLUUID& uuid_out);
bool UUIDFromXML(LLXmlTreeNode* node, LLUUID& uuid_out, std::string const& attribute_name);
bool dateFromXML(LLXmlTreeNode* node, LLDate& date_out);
bool dateFromXML(LLXmlTreeNode* node, LLDate& date_out, std::string const& attribute_name);
bool versionFromXML(LLXmlTreeNode* node, U32& major_out, U32& minor_out);
bool versionFromXML(LLXmlTreeNode* node, U32& major_out, U32& minor_out, std::string const& attribute_name);
class AIXMLElement
{
  private:
	std::ostream& mOs;
	std::string mName;
	int mIndentation;
	bool mHasChildren;
  public:
	AIXMLElement(std::ostream& os, char const* name, int indentation);
	~AIXMLElement() noexcept(false);
	template<typename T>
	  void attribute(char const* name, T const& attribute);
	template<typename T>
	  void child(T const& element);
	template<typename T>
	  void child(char const* name, T const& element);
	template<typename FWD_ITERATOR>
	  void child(FWD_ITERATOR i1, FWD_ITERATOR const& i2);
  private:
	template<typename T>
	  void write_child(char const* name, T const& element);
	int open_child(void);
	void close_child(void);
};
template<typename T>
void AIXMLElement::attribute(char const* name, T const& attribute)
{
  std::ostringstream raw_attribute;
  raw_attribute << attribute;
  mOs << ' ' << name << "=\"" << LLXMLNode::escapeXML(raw_attribute.str()) << '"';
  if (!mOs.good())
  {
	std::ostringstream ss;
	ss << ' ' << name << "=\"" << LLXMLNode::escapeXML(raw_attribute.str()) << '"';
	THROW_FALERT("AIXMLElement_attribute_Failed_to_write_DATA", AIArgs("[DATA]", ss.str()));
  }
}
template<typename T>
void AIXMLElement::child(T const& element)
{
  open_child();
  element.toXML(mOs, mIndentation);
  if (!mOs.good())
  {
	THROW_FALERT("AIXMLElement_child_Bad_ostream");
  }
  close_child();
}
template<>
void AIXMLElement::child(LLUUID const& element);
template<>
void AIXMLElement::child(LLMD5 const& element);
template<>
void AIXMLElement::child(LLDate const& element);
template<typename T>
void AIXMLElement::write_child(char const* name, T const& element)
{
  mOs << std::string(mIndentation, ' ') << '<' << name << '>' << element << "</" << name << ">\n";
  if (!mOs.good())
  {
	std::ostringstream ss;
	ss << std::string(mIndentation, ' ') << '<' << name << '>' << element << "</" << name << ">\\n";
	THROW_FALERT("AIXMLElement_write_child_Failed_to_write_DATA", AIArgs("[DATA]", ss.str()));
  }
}
template<typename T>
void AIXMLElement::child(char const* name, T const& element)
{
  open_child();
  write_child(name, element);
  close_child();
}
template<typename FWD_ITERATOR>
void AIXMLElement::child(FWD_ITERATOR i1, FWD_ITERATOR const& i2)
{
  while (i1 != i2)
  {
	child(*i1++);
  }
}
class AIXMLStream {
  protected:
	llofstream mOfs;
	AIXMLStream(const std::string& filename, bool standalone);
	~AIXMLStream();
};
class AIXMLRootElement : public AIXMLStream, public AIXMLElement
{
  public:
	AIXMLRootElement(const std::string& filename, char const* name, bool standalone = true) : AIXMLStream(filename, standalone), AIXMLElement(mOfs, name, 0) { }
};
class AIXMLElementParser
{
  private:
	U32 mVersion;
	std::string const& mFilename;
	std::string const& mFileDesc;
  protected:
	LLXmlTreeNode* mNode;
  protected:
	AIXMLElementParser(std::string const& filename, std::string const& file_desc, U32 version) : mVersion(version), mFilename(filename), mFileDesc(file_desc) { }
	virtual ~AIXMLElementParser() { }
	virtual std::string node_name(void) const { return "node '" + mNode->getName() + "'"; }
	template<typename T>
	  T read_integer(char const* type, std::string const& value) const;
	template<typename T>
	  T read_string(std::string const& value) const;
	template<typename T>
	  T read_child(LLXmlTreeNode* node) const;
  public:
	AIXMLElementParser(std::string const& filename, std::string const& file_desc, U32 version, LLXmlTreeNode* node) : mVersion(version), mFilename(filename), mFileDesc(file_desc), mNode(node) { }
	void attribute(char const* name, char const* required_value) const;
	template<typename T>
	  bool attribute(char const* name, T& attribute) const;
	template<typename T>
	  bool child(char const* name, T& child) const;
	bool child(LLUUID& uuid) const;
	bool child(LLMD5& md5) const;
	bool child(LLDate& date) const;
	template<typename CONTAINER>
	  void push_back_children(char const* name, CONTAINER& container) const;
	template<typename CONTAINER>
	  void insert_children(char const* name, CONTAINER& container) const;
	void setVersion(U32 version) { mVersion = version; }
	std::string const& filename(void) const { return mFilename; }
	std::string const& filedesc(void) const { return mFileDesc; }
	U32 version(void) const { return mVersion; }
};
template<typename T>
inline T AIXMLElementParser::read_string(std::string const& value) const
{
  return T(value);
}
template<>
LLMD5 AIXMLElementParser::read_string(std::string const& value) const;
template<>
LLDate AIXMLElementParser::read_string(std::string const& value) const;
template<>
U8 AIXMLElementParser::read_string(std::string const& value) const;
template<>
S8 AIXMLElementParser::read_string(std::string const& value) const;
template<>
U16 AIXMLElementParser::read_string(std::string const& value) const;
template<>
S16 AIXMLElementParser::read_string(std::string const& value) const;
template<>
U32 AIXMLElementParser::read_string(std::string const& value) const;
template<>
S32 AIXMLElementParser::read_string(std::string const& value) const;
template<>
F32 AIXMLElementParser::read_string(std::string const& value) const;
template<>
F64 AIXMLElementParser::read_string(std::string const& value) const;
template<>
bool AIXMLElementParser::read_string(std::string const& value) const;
template<typename T>
bool AIXMLElementParser::attribute(char const* name, T& attribute) const
{
  std::string value;
  if (!mNode->getAttributeString(name, value))
  {
	return false;
  }
  attribute = read_string<T>(value);
  return true;
}
template<typename T>
inline T AIXMLElementParser::read_child(LLXmlTreeNode* node) const
{
  return AIXMLElementParser(mFilename, mFileDesc, mVersion, node);
}
template<>
inline std::string AIXMLElementParser::read_child(LLXmlTreeNode* node) const
{
  return node->getContents();
}
template<>
LLMD5 AIXMLElementParser::read_child(LLXmlTreeNode* node) const;
template<>
LLUUID AIXMLElementParser::read_child(LLXmlTreeNode* node) const;
template<>
LLDate AIXMLElementParser::read_child(LLXmlTreeNode* node) const;
template<>
S32 AIXMLElementParser::read_child(LLXmlTreeNode* node) const;
template<>
F32 AIXMLElementParser::read_child(LLXmlTreeNode* node) const;
template<>
bool AIXMLElementParser::read_child(LLXmlTreeNode* node) const;
template<typename T>
bool AIXMLElementParser::child(char const* name, T& child) const
{
  LLXmlTreeNode* node = mNode->getChildByName(name);
  if (!node)
  {
	return false;
  }
  child = read_child<T>(node);
  return true;
}
template<typename CONTAINER>
void AIXMLElementParser::insert_children(char const* name, CONTAINER& container) const
{
  for (LLXmlTreeNode* node = mNode->getFirstChild(); node; node = mNode->getNextChild())
  {
	if (!node->hasName(name))
	  continue;
	container.insert(read_child<typename CONTAINER::value_type>(node));
  }
}
template<typename CONTAINER>
void AIXMLElementParser::push_back_children(char const* name, CONTAINER& container) const
{
  for (LLXmlTreeNode* node = mNode->getFirstChild(); node; node = mNode->getNextChild())
  {
	if (!node->hasName(name))
	  continue;
	container.push_back(read_child<typename CONTAINER::value_type>(node));
  }
}
class AIXMLParser : public AIXMLElementParser
{
  private:
	std::string mFilename;
	std::string mFileDesc;
	LLXmlTree mXmlTree;
	U32 mVersionMajor;
	U32 mVersionMinor;
  public:
	AIXMLParser(std::string const& filename, char const* file_desc, std::string const& name, U32 major_version);
	U32 version_major(void) const { return mVersionMajor; }
	U32 version_minor(void) const { return mVersionMinor; }
  protected:
	std::string node_name(void) const { return "root node"; }
};
#endif
