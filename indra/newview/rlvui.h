/** 
 *
 * Copyright (c) 2009-2011, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt 
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */
#ifndef RLV_UI_H
#define RLV_UI_H
#include "llsingleton.h"
#include "rlvdefines.h"
class RlvUIEnabler : public LLSingleton<RlvUIEnabler>
{
protected:
	RlvUIEnabler();
	friend class LLSingleton<RlvUIEnabler>;
	friend class RlvHandler;
public:
	void onBehaviourToggle(ERlvBehaviour eBhvr, ERlvParamType eType);
protected:
	void onRefreshHoverText();
	void onToggleEdit();
	void onToggleMovement();
	void onToggleSendIM();
	void onToggleSetDebug();
	void onToggleSetEnv();
	void onToggleShowInv(bool fQuitting);
	void onToggleShowLoc();
	void onToggleShowMinimap();
	void onToggleShowNames(bool fQuitting);
	void onToggleShowNameTags(bool fQuitting);
	void onToggleShowWorldMap();
	void onToggleCamUnlock();
	void onToggleTp();
	void onToggleUnsit();
	void onToggleViewXXX();
	void onUpdateLoginLastLocation(bool fQuitting);
protected:
	void addGenericFloaterFilter(const std::string& strFloaterName);
	void removeGenericFloaterFilter(const std::string& strFloaterName);
public:
	static bool canViewParcelProperties();
	static bool canViewRegionProperties();
	static bool hasOpenIM(const LLUUID& idAgent);
	static bool hasOpenProfile(const LLUUID& idAgent);
	static bool isBuildEnabled();
protected:
	typedef boost::function<void(bool)> behaviour_handler_t;
	typedef std::multimap<ERlvBehaviour, behaviour_handler_t> behaviour_handler_map_t;
	behaviour_handler_map_t m_Handlers;
	std::multiset<std::string> m_FilteredFloaters;
};
#endif
