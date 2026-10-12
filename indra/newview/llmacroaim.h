#ifndef LL_LLMACROAIM_H
#define LL_LLMACROAIM_H

#include "llsingleton.h"
#include "lluuid.h"
#include "llframetimer.h"
#include "v3math.h"
#include <string>

class LLVOAvatar;

class LLMacroAim : public LLSingleton<LLMacroAim>
{
public:
	enum EMode
	{
		MODE_OFF,
		MODE_LOCK,
		MODE_ASSIST
	};

	LLMacroAim();
	~LLMacroAim();

	void update();
	void draw();
	void reset();
	void toggleLock();
	void toggleAssist();

	EMode getMode() const { return mMode; }

private:
	F32 getMaxRange() const;
	static bool isCandidate(const LLVOAvatar* avatarp);
	static bool rayHitsHitbox(const LLVOAvatar* avatarp, const LLVector3& origin, const LLVector3& dir, F32 max_t, F32& out_t);
	static LLVOAvatar* resolveAvatar(const LLUUID& id);
	bool checkPreconditions() const;
	void updateHoverTarget();
	void setHoverTarget(const LLUUID& id, F32 distance);
	void setMode(EMode mode, const LLUUID& lock_target);
	void updateLock(F32 dt);
	void endLock(const char* reason);
	void updateAssist(F32 dt);
	void endAssist(const char* reason);
	void clearAssistTarget(const char* reason);
	bool updateAssistLineOfSight(const LLVOAvatar* avatarp, const LLVector3& origin);
	static bool assistEnabledSetting();
	static F32 acquisitionRadius();
	static bool hasLineOfSight(const LLVOAvatar* avatarp, const LLVector3& origin);
	static bool angularError(const LLVOAvatar* avatarp, const LLVector3& origin, const LLVector3& view_dir, F32 max_range, F32& out_error, LLVector3& out_dir);
	static LLVector3 currentViewDirection();
	static bool computeAimError(const LLVector3& view_dir, const LLVector3& want_dir, const LLVector3& skyward, F32& out_yaw, F32& out_pitch);
	static void applySteeredRotation(F32 yaw, F32 pitch, const LLVector3& skyward, F32 interpolant, F32 max_step);
	void loadStrings();
	bool iffShowsName(const LLUUID& id) const;
	bool hudName(const LLUUID& id, const LLVOAvatar* avatarp, std::string& out_name) const;

	EMode mMode;
	bool mPreconditionsMet;
	LLUUID mHoverTargetId;
	LLUUID mLockTargetId;
	LLUUID mAssistTargetId;
	LLUUID mAssistSwitchId;
	LLUUID mAssistRejectedId;
	F32 mHoverDistance;
	LLFrameTimer mHoverLostTimer;
	LLFrameTimer mAssistLostTimer;
	LLFrameTimer mAssistSwitchTimer;
	LLFrameTimer mAssistRejectedTimer;
	LLFrameTimer mLosCheckTimer;
	LLFrameTimer mLosLostTimer;
	bool mLosVisible;
	bool mStringsLoaded;
	std::string mLockHint;
	std::string mAssistHint;
	std::string mLockedText;
	std::string mAssistOnText;
};

#endif
