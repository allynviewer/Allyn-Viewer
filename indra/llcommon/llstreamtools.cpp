/** 
 * @file llstreamtools.cpp
 * @brief some helper functions for parsing legacy simstate and asset files.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "linden_common.h"
#include <iostream>
#include <string>
#include "llstreamtools.h"
bool skip_whitespace(std::istream& input_stream)
{
	int c = input_stream.peek();
	while (('\t' == c || ' ' == c) && input_stream.good())
	{
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}
bool skip_emptyspace(std::istream& input_stream)
{
	int c = input_stream.peek();
	while ( input_stream.good()
			&& ('\t' == c || ' ' == c || '\n' == c || '\r' == c) )
	{
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}
bool skip_comments_and_emptyspace(std::istream& input_stream)
{
	while (skip_emptyspace(input_stream))
	{
		int c = input_stream.peek();
		if ('#' == c )
		{
			while ('\n' != c && input_stream.good())
			{
				c = input_stream.get();
			}
		}
		else
		{
			break;
		}
	}
	return input_stream.good();
}
bool skip_line(std::istream& input_stream)
{
	int c;
	do
	{
		c = input_stream.get();
	} while ('\n' != c  &&  input_stream.good());
	return input_stream.good();
}
bool skip_to_next_word(std::istream& input_stream)
{
	int c = input_stream.peek();
	while ( input_stream.good()
			&& (   (c >= 'a' && c <= 'z')
		   		|| (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9')
				|| '_' == c ) )
	{
		input_stream.get();
		c = input_stream.peek();
	}
	while ( input_stream.good()
			&& !(   (c >= 'a' && c <= 'z')
		   		 || (c >= 'A' && c <= 'Z')
				 || (c >= '0' && c <= '9')
				 || '_' == c ) )
	{
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}
bool skip_to_end_of_next_keyword(const char* keyword, std::istream& input_stream)
{
	size_t key_length = strlen(keyword);
	if (0 == key_length)
	{
		return false;
	}
	while (input_stream.good())
	{
		skip_emptyspace(input_stream);
		int c = input_stream.get();
		if (keyword[0] != c)
		{
			skip_line(input_stream);
		}
		else
		{
			size_t key_index = 1;
			while ( key_index < key_length
					&&	keyword[key_index - 1] == c
			   		&& input_stream.good())
			{
				key_index++;
				c = input_stream.get();
			}
			if (key_index == key_length
				&& keyword[key_index-1] == c)
			{
				c = input_stream.peek();
				if (' ' == c || '\t' == c || '\r' == c || '\n' == c)
				{
					return true;
				}
				else
				{
					skip_line(input_stream);
				}
			}
			else
			{
				skip_line(input_stream);
			}
		}
	}
	return false;
}
bool get_word(std::string& output_string, std::istream& input_stream)
{
	skip_emptyspace(input_stream);
	int c = input_stream.peek();
	while ( !isspace(c)
			&& '\n' != c
			&& '\r' != c
			&& input_stream.good() )
	{
		output_string += c;
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}
bool get_word(std::string& output_string, std::istream& input_stream, int n)
{
	skip_emptyspace(input_stream);
	int char_count = 0;
	int c = input_stream.peek();
	while (!isspace(c)
			&& '\n' != c
			&& '\r' != c
			&& input_stream.good()
			&& char_count < n)
	{
		char_count++;
		output_string += c;
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}
bool get_line(std::string& output_string, std::istream& input_stream)
{
	output_string.clear();
	int c = input_stream.get();
	while (input_stream.good())
	{
		output_string += c;
		if ('\n' == c)
		{
			break;
		}
		c = input_stream.get();
	}
	return input_stream.good();
}
bool get_line(std::string& output_string, std::istream& input_stream, int n)
{
	output_string.clear();
	int char_count = 0;
	int c = input_stream.get();
	while (input_stream.good() && char_count < n)
	{
		char_count++;
		output_string += c;
		if ('\n' == c)
		{
			break;
		}
		if (char_count >= n)
		{
			output_string.append("\n");
			break;
		}
		c = input_stream.get();
	}
	return input_stream.good();
}
bool remove_last_char(char c, std::string& line)
{
	size_t line_size = line.size();
	if (line_size > 1
		&& c == line[line_size - 1])
	{
		line.replace(line_size - 1, 1, "");
		return true;
	}
	return false;
}
void unescape_string(std::string& line)
{
	int line_size = line.size();
	int index = 0;
	while (index < line_size - 1)
	{
		if ('\\' == line[index])
		{
			if ('\\' == line[index + 1])
			{
				line.replace(index, 2, "\\");
				line_size--;
			}
			else if ('n' == line[index + 1])
			{
				line.replace(index, 2, "\n");
				line_size--;
			}
		}
		index++;
	}
}
void escape_string(std::string& line)
{
	int line_size = line.size();
	int index = 0;
	while (index < line_size)
	{
		if ('\\' == line[index])
		{
			line.replace(index, 1, "\\\\");
			line_size++;
			index++;
		}
		else if ('\n' == line[index])
		{
			line.replace(index, 1, "\\n");
			line_size++;
			index++;
		}
		index++;
	}
}
void replace_newlines_with_whitespace(std::string& line)
{
	int line_size = line.size();
	int index = 0;
	while (index < line_size)
	{
		if ('\n' == line[index])
		{
			line.replace(index, 1, " ");
		}
		index++;
	}
}
void remove_double_quotes(std::string& line)
{
	int index = 0;
	int line_size = line.size();
	while (index < line_size)
	{
		if ('"' == line[index])
		{
			int count = 1;
			while (index + count < line_size
				   && '"' == line[index + count])
			{
				count++;
			}
			line.replace(index, count, "");
			line_size -= count;
		}
		else
		{
			index++;
		}
	}
}
void get_keyword_and_value(std::string& keyword,
						   std::string& value,
						   const std::string& line)
{
	int line_size = line.size();
	int line_index = 0;
	char c;
	while (line_index < line_size)
	{
		c = line[line_index];
		if (!LLStringOps::isSpace(c))
		{
			break;
		}
		line_index++;
	}
	keyword.clear();
	while (line_index < line_size)
	{
		c = line[line_index];
		if (LLStringOps::isSpace(c) || '\r' == c || '\n' == c)
		{
			break;
		}
		keyword += c;
		line_index++;
	}
	value.clear();
	if (keyword.size() > 0
		&& '\r' != line[line_index]
		&& '\n' != line[line_index])
	{
		while (line_index < line_size
				&& (' ' == line[line_index]
					|| '\t' == line[line_index]) )
		{
			line_index++;
		}
		while (line_index < line_size)
		{
			c = line[line_index];
			if ('\r' == c || '\n' == c)
			{
				break;
			}
			value += c;
			line_index++;
		}
	}
}
std::streamsize fullread(
	std::istream& istr,
	char* buf,
	std::streamsize requested)
{
	std::streamsize got;
	std::streamsize total = 0;
	istr.read(buf, requested);
	got = istr.gcount();
	total += got;
	while(got && total < requested)
	{
		if(istr.fail())
		{
			if(istr.bad()) return total;
			istr.clear();
		}
		istr.read(buf + total, requested - total);
		got = istr.gcount();
		total += got;
	}
	return total;
}
std::istream& operator>>(std::istream& str, const char *tocheck)
{
	char c = '\0';
	const char *p;
	p = tocheck;
	while (*p && !str.bad())
	{
		str.get(c);
		if (c != *p)
		{
			str.setstate(std::ios::failbit);
			break;
		}
		p++;
	}
	return str;
}
