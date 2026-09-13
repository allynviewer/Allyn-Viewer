/** 
 * @file llfloaterregiondebugconsole.h
 * @author Brad Kittenbrink <brad@lindenlab.com>
 * @brief Quick and dirty console for region debug settings
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include "llfloaterregiondebugconsole.h"
#include "llagent.h"
#include "llhttpclient.h"
#include "llhttpnode.h"
#include "lllineeditor.h"
#include "lltexteditor.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
namespace
{
	console_reply_signal_t sConsoleReplySignal;
	const std::string PROMPT("\n\n> ");
	const std::string UNABLE_TO_SEND_COMMAND(
		"ERROR: The last command was not received by the server.");
	const std::string CONSOLE_UNAVAILABLE(
		"ERROR: No console available for this region/simulator.");
	const std::string CONSOLE_NOT_SUPPORTED(
		"This region does not support the simulator console.");
	class AsyncConsoleResponder : public LLHTTPClient::ResponderIgnoreBody
	{
	public:
		void httpFailure(void) { sConsoleReplySignal(UNABLE_TO_SEND_COMMAND); }
		char const* getName(void) const { return "AsyncConsoleResponder"; }
	};
	class ConsoleResponder : public LLHTTPClient::ResponderWithResult
	{
	public:
		ConsoleResponder(LLTextEditor *output) : mOutput(output)
		{
		}
		void httpFailure(void)
		{
			if (mOutput)
			{
				mOutput->appendText(
					UNABLE_TO_SEND_COMMAND + PROMPT,
					false, false);
			}
		}
		void httpSuccess(void)
		{
			if (mOutput)
			{
				mOutput->appendText(
					mContent.asString() + PROMPT, false, false);
			}
		}
		char const* getName(void) const { return "ConsoleResponder"; }
		LLTextEditor * mOutput;
	};
	class ConsoleResponseNode : public LLHTTPNode
	{
	public:
		void post(
			LLHTTPNode::ResponsePtr reponse,
			const LLSD& context,
			const LLSD& input) const
		{
			LL_INFOS() << "Received response from the debug console: "
				<< input << LL_ENDL;
			sConsoleReplySignal(input["body"].asString());
		}
	};
}
boost::signals2::connection LLFloaterRegionDebugConsole::setConsoleReplyCallback(const console_reply_signal_t::slot_type& cb)
{
	return sConsoleReplySignal.connect(cb);
}
LLFloaterRegionDebugConsole::LLFloaterRegionDebugConsole()
: LLFloater(), mOutput(NULL)
{
	mReplySignalConnection = sConsoleReplySignal.connect(
		boost::bind(
			&LLFloaterRegionDebugConsole::onReplyReceived,
			this,
			_1));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_region_debug_console.xml");
}
LLFloaterRegionDebugConsole::~LLFloaterRegionDebugConsole()
{
	mReplySignalConnection.disconnect();
}
BOOL LLFloaterRegionDebugConsole::postBuild()
{
	LLLineEditor* input = getChild<LLLineEditor>("region_debug_console_input");
	input->setEnableLineHistory(true);
	input->setCommitCallback(boost::bind(&LLFloaterRegionDebugConsole::onInput, this, _1, _2));
	input->setFocus(true);
	input->setCommitOnFocusLost(false);
	mOutput = getChild<LLTextEditor>("region_debug_console_output");
	std::string url = gAgent.getRegion()->getCapability("SimConsoleAsync");
	if (url.empty())
	{
		url = gAgent.getRegion()->getCapability("SimConsole");
		if (url.empty())
		{
			mOutput->appendText(
				CONSOLE_NOT_SUPPORTED + PROMPT,
				false, false);
			return TRUE;
		}
	}
	mOutput->appendText("> ", false, false);
	return TRUE;
}
void LLFloaterRegionDebugConsole::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
}
void LLFloaterRegionDebugConsole::onInput(LLUICtrl* ctrl, const LLSD& param)
{
	LLLineEditor* input = static_cast<LLLineEditor*>(ctrl);
	std::string text = input->getText() + "\n";
	std::string url = gAgent.getRegion()->getCapability("SimConsoleAsync");
	if (url.empty())
	{
		url = gAgent.getRegion()->getCapability("SimConsole");
		if (url.empty())
		{
			text += CONSOLE_UNAVAILABLE + PROMPT;
		}
		else
		{
			LLHTTPClient::post(
				url,
				LLSD(input->getText()),
				new ConsoleResponder(mOutput));
		}
	}
	else
	{
		LLHTTPClient::post(
			url,
			LLSD(input->getText()),
			new AsyncConsoleResponder);
	}
	mOutput->appendText(text, false, false);
	input->clear();
}
void LLFloaterRegionDebugConsole::onReplyReceived(const std::string& output)
{
	mOutput->appendText(output + PROMPT, false, false);
}
LLHTTPRegistration<ConsoleResponseNode>
	gHTTPRegistrationMessageDebugConsoleResponse(
		"/message/SimConsoleResponse");
