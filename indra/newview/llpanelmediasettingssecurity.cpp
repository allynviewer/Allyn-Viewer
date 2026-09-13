/**
 * @file llpanelmediasettingssecurity.cpp
 * @brief LLPanelMediaSettingsSecurity class implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llpanelmediasettingssecurity.h"
#include "llpanelcontents.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llmediaentry.h"
#include "lltextbox.h"
#include "llfloaterwhitelistentry.h"
#include "llfloatermediasettings.h"
LLPanelMediaSettingsSecurity::LLPanelMediaSettingsSecurity() :
	mParent( NULL )
{
	mCommitCallbackRegistrar.add("Media.whitelistAdd",		boost::bind(&LLPanelMediaSettingsSecurity::onBtnAdd, this));
	mCommitCallbackRegistrar.add("Media.whitelistDelete",	boost::bind(&LLPanelMediaSettingsSecurity::onBtnDel, this));
	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_media_settings_security.xml");
}
BOOL LLPanelMediaSettingsSecurity::postBuild()
{
	mEnableWhiteList = getChild< LLCheckBoxCtrl >( LLMediaEntry::WHITELIST_ENABLE_KEY );
	mWhiteListList = getChild< LLScrollListCtrl >( LLMediaEntry::WHITELIST_KEY );
	mHomeUrlFailsWhiteListText = getChild<LLTextBox>( "home_url_fails_whitelist" );
	setDefaultBtn("whitelist_add");
	return true;
}
LLPanelMediaSettingsSecurity::~LLPanelMediaSettingsSecurity()
{
}
void LLPanelMediaSettingsSecurity::draw()
{
	LLPanel::draw();
}
void LLPanelMediaSettingsSecurity::initValues( void* userdata, const LLSD& media_settings , bool editable)
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;
	std::string base_key( "" );
	std::string tentative_key( "" );
	struct
	{
		std::string key_name;
		LLUICtrl* ctrl_ptr;
		std::string ctrl_type;
	} data_set [] =
	{
		{ LLMediaEntry::WHITELIST_ENABLE_KEY,	self->mEnableWhiteList,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::WHITELIST_KEY,			self->mWhiteListList,		"LLScrollListCtrl" },
		{ "", NULL , "" }
	};
	for( int i = 0; data_set[ i ].key_name.length() > 0; ++i )
	{
		base_key = std::string( data_set[ i ].key_name );
        tentative_key = base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX );
		bool enabled_overridden = false;
		if ( media_settings[ base_key ].isDefined() )
		{
			if ( data_set[ i ].ctrl_type == "LLCheckBoxCtrl" )
			{
				static_cast< LLCheckBoxCtrl* >( data_set[ i ].ctrl_ptr )->
					setValue( media_settings[ base_key ].asBoolean() );
			}
			else
			if ( data_set[ i ].ctrl_type == "LLScrollListCtrl" )
			{
				LLScrollListCtrl* list = static_cast< LLScrollListCtrl* >( data_set[ i ].ctrl_ptr );
				list->deleteAllItems();
				LLSD url_list = media_settings[ base_key ];
				llassert(data_set[ i ].ctrl_ptr == self->mWhiteListList);
				if (media_settings[ tentative_key ].asBoolean())
				{
					self->mWhiteListList->setEnabled(false);
					enabled_overridden = true;
				}
				else {
					LLSD::array_iterator iter = url_list.beginArray();
					while( iter != url_list.endArray() )
					{
						std::string entry = *iter;
						self->addWhiteListEntry( entry );
						++iter;
					}
				}
			};
			if ( ! enabled_overridden) data_set[ i ].ctrl_ptr->setEnabled(editable);
			data_set[ i ].ctrl_ptr->setTentative( media_settings[ tentative_key ].asBoolean() );
		};
	};
	self->updateWhitelistEnableStatus();
}
void LLPanelMediaSettingsSecurity::clearValues( void* userdata , bool editable)
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;
	self->mEnableWhiteList->clear();
	self->mWhiteListList->deleteAllItems();
	self->mEnableWhiteList->setEnabled(editable);
	self->mWhiteListList->setEnabled(editable);
}
void LLPanelMediaSettingsSecurity::preApply()
{
}
void LLPanelMediaSettingsSecurity::getValues( LLSD &fill_me_in, bool include_tentative )
{
    if (include_tentative || !mEnableWhiteList->getTentative())
		fill_me_in[LLMediaEntry::WHITELIST_ENABLE_KEY] = (LLSD::Boolean)mEnableWhiteList->getValue();
	if (include_tentative || !mWhiteListList->getTentative())
	{
		std::vector< LLScrollListItem* > whitelist_items = mWhiteListList->getAllData();
		std::vector< LLScrollListItem* >::iterator iter = whitelist_items.begin();
		fill_me_in[LLMediaEntry::WHITELIST_KEY] = LLSD::emptyArray();
		while( iter != whitelist_items.end() )
		{
			LLScrollListCell* cell = (*iter)->getColumn( ENTRY_COLUMN );
			std::string whitelist_url = cell->getValue().asString();
			fill_me_in[ LLMediaEntry::WHITELIST_KEY ].append( whitelist_url );
			++iter;
		};
	}
}
void LLPanelMediaSettingsSecurity::postApply()
{
}
const std::string LLPanelMediaSettingsSecurity::makeValidUrl( const std::string& src_url )
{
	LLURI candidate_url( src_url );
	if ( candidate_url.scheme().empty() )
	{
		const std::string default_scheme( "http://" );
		return default_scheme + src_url;
	};
	return src_url;
}
bool LLPanelMediaSettingsSecurity::urlPassesWhiteList( const std::string& test_url )
{
	if ( mWhiteListList->getTentative() ) return true;
	std::vector< std::string > whitelist_strings;
	whitelist_strings.clear();
    std::vector< LLScrollListItem* > whitelist_items = mWhiteListList->getAllData();
    std::vector< LLScrollListItem* >::iterator iter = whitelist_items.begin();
	while( iter != whitelist_items.end()  )
    {
		LLScrollListCell* cell = (*iter)->getColumn( ENTRY_COLUMN );
		std::string whitelist_url = cell->getValue().asString();
		whitelist_strings.push_back( whitelist_url );
		++iter;
    };
	const std::string valid_url = makeValidUrl( test_url );
	return LLMediaEntry::checkUrlAgainstWhitelist( valid_url, whitelist_strings );
}
void LLPanelMediaSettingsSecurity::updateWhitelistEnableStatus()
{
	const std::string valid_url = makeValidUrl( mParent->getHomeUrl() );
	if ( urlPassesWhiteList( valid_url ) )
	{
		mEnableWhiteList->setEnabled( true );
		mHomeUrlFailsWhiteListText->setVisible( false );
	}
	else
	{
		mEnableWhiteList->set( false );
		mEnableWhiteList->setEnabled( false );
		mHomeUrlFailsWhiteListText->setVisible( true );
	};
}
void LLPanelMediaSettingsSecurity::addWhiteListEntry( const std::string& entry )
{
	std::string home_url( "" );
	if ( mParent )
		home_url = mParent->getHomeUrl();
	const std::string valid_url = makeValidUrl( home_url );
	std::vector< std::string > whitelist_entries;
	whitelist_entries.push_back( entry );
	bool home_url_passes_entry = LLMediaEntry::checkUrlAgainstWhitelist( valid_url, whitelist_entries );
	LLSD row;
	if ( home_url_passes_entry || home_url.empty() )
	{
		row[ "columns" ][ ICON_COLUMN ][ "type" ] = "icon";
		row[ "columns" ][ ICON_COLUMN ][ "value" ] = "";
		row[ "columns" ][ ICON_COLUMN ][ "width" ] = 20;
	}
	else
	{
		row[ "columns" ][ ICON_COLUMN ][ "type" ] = "icon";
		row[ "columns" ][ ICON_COLUMN ][ "value" ] = "Parcel_Exp_Color.png";
		row[ "columns" ][ ICON_COLUMN ][ "width" ] = 20;
	};
	row[ "columns" ][ ENTRY_COLUMN ][ "type" ] = "text";
	row[ "columns" ][ ENTRY_COLUMN ][ "value" ] = entry;
	mWhiteListList->addElement( row );
};
void LLPanelMediaSettingsSecurity::onBtnAdd( void* userdata )
{
	LLPanelMediaSettingsSecurity* self = (LLPanelMediaSettingsSecurity*)userdata;
	LLFloaterWhiteListEntry::getInstance()->open();
	for(LLView* parent = self->getParent(); parent !=NULL; parent = parent->getParent())
	{
		if(dynamic_cast<LLFloater*>(parent))
		{
			LLFloaterWhiteListEntry::getInstance()->centerWithin(parent->getRect());
			break;
		}
	}
}
void LLPanelMediaSettingsSecurity::onBtnDel( void* userdata )
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;
	self->mWhiteListList->deleteSelectedItems();
	self->updateWhitelistEnableStatus();
}
void LLPanelMediaSettingsSecurity::setParent( LLFloaterMediaSettings* parent )
{
	mParent = parent;
};
