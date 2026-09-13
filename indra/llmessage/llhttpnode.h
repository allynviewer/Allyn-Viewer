/** 
 * @file llhttpnode.h
 * @brief Declaration of classes for generic HTTP/LSL/REST handling.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#ifndef LL_LLHTTPNODE_H
#define LL_LLHTTPNODE_H
#include "llpointer.h"
#include "llrefcount.h"
#include "llsd.h"
class LLChainIOFactory;
class LLHTTPNode
{
public:
	LLHTTPNode();
	virtual ~LLHTTPNode();
public:
	virtual LLSD simpleGet() const;
	virtual LLSD simplePut(const LLSD& input) const;
	virtual LLSD simplePost(const LLSD& input) const;
	virtual LLSD simpleDel(const LLSD& context) const;
	class Response : public LLRefCount
	{
	protected:
		virtual ~Response();
	public:
		virtual void result(const LLSD&) = 0;
		virtual void extendedResult(S32 code, const std::string& message, const LLSD& headers) = 0;
		virtual void status(S32 code, const std::string& message) = 0;
		virtual void statusUnknownError(S32 code);
		virtual void notFound(const std::string& message);
		virtual void notFound();
		virtual void methodNotAllowed();
		virtual void addHeader(const std::string& name, const std::string& value);
	protected:
		LLSD mHeaders;
	};
	typedef LLPointer<Response> ResponsePtr;
	virtual void get(ResponsePtr, const LLSD& context) const;
	virtual void put(
		ResponsePtr,
		const LLSD& context,
		const LLSD& input) const;
	virtual void post(
		ResponsePtr,
		const LLSD& context,
		const LLSD& input) const;
	virtual void del(ResponsePtr, const LLSD& context) const;
	virtual void options(ResponsePtr, const LLSD& context) const;
public:
	virtual LLHTTPNode* getChild(const std::string& name, LLSD& context) const;
	virtual bool handles(const LLSD& remainder, LLSD& context) const;
	virtual bool validate(const std::string& name, LLSD& context) const;
	const LLHTTPNode* traverse(const std::string& path, LLSD& context) const;
	virtual void addNode(const std::string& path, LLHTTPNode* nodeToAdd);
	LLSD allNodePaths() const;
	const LLHTTPNode* rootNode() const;
	const LLHTTPNode* findNode(const std::string& name) const;
	enum EHTTPNodeContentType
	{
		CONTENT_TYPE_LLSD,
		CONTENT_TYPE_TEXT
	};
	virtual EHTTPNodeContentType getContentType() const { return CONTENT_TYPE_LLSD; }
		class Description
		{
		public:
			void shortInfo(const std::string& s){ mInfo["description"] = s; }
			void longInfo(const std::string& s)	{ mInfo["details"] = s; }
			void getAPI() { mInfo["api"].append("GET"); }
			void putAPI() { mInfo["api"].append("PUT");  }
			void postAPI() { mInfo["api"].append("POST"); }
			void delAPI() { mInfo["api"].append("DELETE"); }
			void input(const std::string& s)	{ mInfo["input"] = s; }
			void output(const std::string& s)	{ mInfo["output"] = s; }
			void source(const char* f, int l)	{ mInfo["__file__"] = f;
												  mInfo["__line__"] = l; }
			LLSD getInfo() const { return mInfo; }
		private:
			LLSD mInfo;
		};
	virtual void describe(Description&) const;
	virtual const LLChainIOFactory* getProtocolHandler() const;
private:
	class Impl;
	Impl& impl;
};
class LLSimpleResponse : public LLHTTPNode::Response
{
public:
	static LLPointer<LLSimpleResponse> create();
	void result(const LLSD& result);
	void extendedResult(S32 code, const std::string& body, const LLSD& headers);
	void status(S32 code, const std::string& message);
	void print(std::ostream& out) const;
	S32 mCode;
	std::string mMessage;
protected:
	~LLSimpleResponse();
private:
        LLSimpleResponse() : mCode(0) {}
};
std::ostream& operator<<(std::ostream& out, const LLSimpleResponse& resp);
class LLHTTPRegistrar
{
public:
	class NodeFactory
	{
	public:
		virtual ~NodeFactory();
		virtual LLHTTPNode* build() const = 0;
	};
	static void buildAllServices(LLHTTPNode& root);
	static void registerFactory(const std::string& path, NodeFactory& factory);
};
template < class NodeType >
class LLHTTPRegistration
{
public:
	LLHTTPRegistration(const std::string& path)
	{
		LLHTTPRegistrar::registerFactory(path, mFactory);
	}
private:
	class ThisNodeFactory : public LLHTTPRegistrar::NodeFactory
	{
	public:
		virtual LLHTTPNode* build() const { return new NodeType; }
	};
	ThisNodeFactory	mFactory;
};
template < class NodeType>
class LLHTTPParamRegistration
{
public:
	LLHTTPParamRegistration(const std::string& path, LLSD params) :
		mFactory(params)
	{
		LLHTTPRegistrar::registerFactory(path, mFactory);
	}
private:
	class ThisNodeFactory : public LLHTTPRegistrar::NodeFactory
	{
	public:
		ThisNodeFactory(LLSD params) : mParams(params) {}
		virtual LLHTTPNode* build() const { return new NodeType(mParams); }
	private:
		LLSD mParams;
	};
	ThisNodeFactory	mFactory;
};
#endif
