#ifndef SH_SHFLOATERMEDIATICKER_H
#define SH_SHFLOATERMEDIATICKER_H
#include "llfloater.h"
class LLIconCtrl;
class SHFloaterMediaTicker : public LLFloater, public LLSingleton<SHFloaterMediaTicker>
{
	friend class LLSingleton<SHFloaterMediaTicker>;
public:
	SHFloaterMediaTicker();
	virtual ~SHFloaterMediaTicker() {}
	BOOL postBuild();
	void draw();
	void onOpen();
	void onClose(bool app_quitting);
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	static void showInstance();
private:
	void updateTickerText();
	void drawOscilloscope();
	bool setPaused(bool pause);
	void resetTicker();
	bool setArtist(const std::string &artist);
	bool setTitle(const std::string &title);
	S32 countExtraChars(LLTextBox *texbox, const std::string &text);
	void iterateTickerOffset();
	enum ePlayState
	{
		STATE_PAUSED,
		STATE_PLAYING
	};
	ePlayState mPlayState;
	std::string mszLoading;
	std::string mszPaused;
	std::string mszArtist;
	std::string mszTitle;
	LLTimer mScrollTimer;
	LLTimer mLoadTimer;
	LLTimer mUpdateTimer;
	S32 mArtistScrollChars;
	S32 mTitleScrollChars;
	S32 mCurScrollChar;
	LLColor4 mOscillatorColor;
	LLIconCtrl* mTickerBackground;
	LLTextBox* mArtistText;
	LLTextBox* mTitleText;
	LLUICtrl* mVisualizer;
};
BOOL handle_ticker_enabled(void *);
void handle_ticker_toggle(void *);
#endif
