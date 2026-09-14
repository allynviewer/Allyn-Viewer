/**
 * @file llchataitranslate.h
 * @brief AI-backed chat/IM translation (OpenAI / Anthropic / OpenCode Zen / OpenRouter compatible)
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#ifndef LL_LLCHATAITRANSLATE_H
#define LL_LLCHATAITRANSLATE_H
#include "llsingleton.h"
#include "llsd.h"
#include "lluuid.h"
#include "v4color.h"
#include <boost/function.hpp>
#include <list>
#include <map>
#include <string>
#include <vector>
class LLChat;
class LLChatAITranslate : public LLSingleton<LLChatAITranslate>
{
	friend class LLSingleton<LLChatAITranslate>;
public:
	typedef boost::function<void(bool success, const std::string& text_or_error)> Callback;
	bool isEnabled() const;
	bool translateIncoming() const;
	bool translateOutgoing() const;
	bool coversIM() const;
	bool coversNearby() const;
	bool showIncomingBoth() const;
	bool showOutgoingBoth() const;
	bool groupsEnabled() const;
	bool isGroupAllowed(const LLUUID& group_id) const;
	bool shouldTranslateSession(class LLFloaterIMPanel* panel) const;
	std::string sourceLang() const;
	std::string targetLang() const;
	LLColor4 incomingColor() const;
	LLColor4 outgoingColor() const;
	LLUUID sessionConfigKey(class LLFloaterIMPanel* panel) const;
	void ensureSessionConfig(class LLFloaterIMPanel* panel, bool& enabled,
							 std::string& source, std::string& target,
							 bool& incoming, bool& outgoing);
	bool hasSessionTranslation(class LLFloaterIMPanel* panel);
	bool sessionTranslateIncoming(class LLFloaterIMPanel* panel);
	bool sessionTranslateOutgoing(class LLFloaterIMPanel* panel);
	std::string sessionSourceLang(class LLFloaterIMPanel* panel);
	std::string sessionTargetLang(class LLFloaterIMPanel* panel);
	void setSessionTranslation(const LLUUID& key, bool enabled,
							   const std::string& source, const std::string& target,
							   bool incoming = true, bool outgoing = true);
	bool getSessionTranslation(const LLUUID& key, bool& enabled,
							   std::string& source, std::string& target,
							   bool& incoming, bool& outgoing);
	void resyncSessionConfigsFromGlobalPrefs();
	void testConnection(Callback cb);
	void translate(const std::string& text, const std::string& from_lang, const std::string& to_lang, Callback cb,
				 S32 retries_left = 1, const std::string& retry_hint = std::string());
	void handleIncomingNearby(LLChat chat, bool only_history, const std::string& raw_mesg);
	bool handleIncomingIM(class LLFloaterIMPanel* floater, const std::string& msg, const LLColor4& color,
						  bool link_name, const LLUUID& source_id, const std::string& from_name);
	bool maybeDeferOutgoingNearby(const std::string& text, U8 type, S32 channel);
	bool maybeDeferOutgoingIM(class LLFloaterIMPanel* panel, const std::string& utf8_text, bool is_action);
	static void applyProviderPreset(const std::string& provider);
	void onRequestFinished();
	void enqueue(const std::string& body, Callback cb, bool connection_test,
				 const std::string& source_text, const std::string& canary,
				 const std::string& cache_from, const std::string& cache_to,
				 S32 retries_left = 1);
	void cacheStore(const std::string& from_lang, const std::string& to_lang,
					const std::string& text, const std::string& translated);
	static std::vector<LLUUID> parseGroupIdList(const std::string& csv);
	static std::string joinGroupIdList(const std::vector<LLUUID>& ids);
	bool isAgentNoTranslate(const LLUUID& id) const;
	void addAgentNoTranslate(const LLUUID& id, const std::string& name);
	void removeAgentsNoTranslate(const uuid_vec_t& ids);
	LLSD getNoTranslateAgents() const;
private:
	LLChatAITranslate();
	~LLChatAITranslate();
	void loadSessionConfigs();
	void saveSessionConfigs() const;
	void loadNoTranslateAgents() const;
	void saveNoTranslateAgents() const;
	void notifyNoTranslateChanged() const;
	void pumpQueue();
	std::string buildRequestBody(const std::string& user_content, S32 max_tokens,
								 const std::string& canary) const;
	std::string buildUrl() const;
	void fillHeaders(class AIHTTPHeaders& headers) const;
	std::string apiStyle() const;
	bool cacheLookup(const std::string& from_lang, const std::string& to_lang,
					 const std::string& text, std::string& out) const;
	S32 mInFlight;
	bool mPassThroughOutgoing;
	bool mPumpScheduled;
	static const S32 MAX_IN_FLIGHT = 12;
	static const S32 MAX_BATCH_SIZE = 10;
	static const size_t MAX_QUEUE = 120;
	static const size_t MAX_CACHE_ENTRIES = 1024;
	static const S32 DEFAULT_RETRIES = 1;
	void schedulePump();
	void loadPhraseCache();
	void savePhraseCache() const;
	Callback wrapWithTimeout(Callback cb, const std::string& fallback_on_timeout);
	F32 adaptiveTimeoutSec() const;
	void noteTranslateLatency(F32 seconds);
	struct CacheEntry
	{
		std::string key;
		std::string value;
	};
	mutable std::list<CacheEntry> mCacheLRU;
	mutable std::map<std::string, std::list<CacheEntry>::iterator> mCacheIndex;
	mutable bool mPhraseCacheLoaded;
	mutable bool mPhraseCacheDirty;
	mutable bool mPhraseCacheSaveScheduled;
	F32 mLatencyEma;
	F32 mLatencyPeak;
	S32 mLatencySamples;
	LLSD mSessionConfigs;
	bool mSessionConfigsLoaded;
	mutable LLSD mNoTranslateAgents;
	mutable bool mNoTranslateLoaded;
};
#endif
