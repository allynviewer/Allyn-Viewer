#ifndef LL_LLPREFSMACRO_H
#define LL_LLPREFSMACRO_H

#include "llpanel.h"

class LLPrefsMacro : public LLPanel
{
public:
	LLPrefsMacro();
	virtual ~LLPrefsMacro();

	void apply();
	void cancel();

private:
	void refreshValues();

	bool mAssistEnabled;
	F32 mAcquisition;
	F32 mStrength;
};

#endif
