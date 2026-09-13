/** 
 * @file importtracker.cpp
 * @brief A utility for importing linksets from XML.
 * Discrete wuz here
 */
#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "llcontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "importtracker.h"
#include "lltooldraganddrop.h"
ImportTracker gImportTracker;
extern LLAgent gAgent;
void ImportTracker::expectRez()
{
	numberExpected++;
	state = WAND;
}
LLViewerObject* find(U32 local)
{
	S32 i;
	S32 total = gObjectList.getNumObjects();
	for (i = 0; i < total; i++)
	{
		LLViewerObject *objectp = gObjectList.getObject(i);
		if (objectp)
		{
			if(objectp->getLocalID() == local)return objectp;
		}
	}
	return NULL;
}
void ImportTracker::get_update(S32 newid, BOOL justCreated, BOOL createSelected)
{
	switch (state)
	{
		case WAND:
			if(justCreated && createSelected)
			{
				numberExpected--;
				if(numberExpected<=0)
					state=IDLE;
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ObjectImage);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_ObjectLocalID,  (U32)newid);
				msg->addStringFast(_PREHASH_MediaURL, NULL);
				LLPrimitive obj;
				obj.setNumTEs(U8(10));
				S32 shinnyLevel = 0;
				static LLCachedControl<std::string> sshinystr(gSavedSettings, "EmeraldBuildPrefs_Shiny");
				std::string shinystr = sshinystr;
				if(shinystr == "Low") shinnyLevel = 1;
				else if(shinystr == "Medium") shinnyLevel = 2;
				else if(shinystr == "High") shinnyLevel = 3;
				for (int i = 0; i < 10; i++)
				{
					LLTextureEntry tex =  LLTextureEntry(LLUUID(gSavedSettings.getString("EmeraldBuildPrefs_Texture")));
					tex.setColor(gSavedSettings.getColor4("EmeraldBuildPrefs_Color"));
					tex.setAlpha(1.0 - ((gSavedSettings.getF32("EmeraldBuildPrefs_Alpha")) / 100.0));
					tex.setGlow(gSavedSettings.getF32("EmeraldBuildPrefs_Glow"));
					if(gSavedSettings.getBOOL("EmeraldBuildPrefs_FullBright"))
					{
						tex.setFullbright(TEM_FULLBRIGHT_MASK);
					}
					tex.setShiny((U8) shinnyLevel & TEM_SHINY_MASK);
					obj.setTE(U8(i), tex);
				}
				obj.packTEMessage(gMessageSystem);
				msg->sendReliable(gAgent.getRegion()->getHost());
				msg->newMessage("ObjectFlagUpdate");
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addU32Fast(_PREHASH_ObjectLocalID, (U32)newid );
				msg->addBOOLFast(_PREHASH_UsePhysics, gSavedSettings.getBOOL("EmeraldBuildPrefs_Physical"));
				msg->addBOOLFast(_PREHASH_IsTemporary, gSavedSettings.getBOOL("EmeraldBuildPrefs_Temporary"));
				msg->addBOOLFast(_PREHASH_IsPhantom, gSavedSettings.getBOOL("EmeraldBuildPrefs_Phantom") );
				msg->addBOOL("CastsShadows", false );
				msg->sendReliable(gAgent.getRegion()->getHost());
				if(gSavedSettings.getBOOL("EmeraldBuildPrefs_EmbedItem"))
				{
					LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem((LLUUID)gSavedPerAccountSettings.getString("EmeraldBuildPrefs_Item"));
					LLViewerObject* objectp = find((U32)newid);
					if(objectp)
						if(item)
						{
							if(item->getType()==LLAssetType::AT_LSL_TEXT)
							{
								LLToolDragAndDrop::dropScript(objectp,
									item,
									TRUE,
									LLToolDragAndDrop::SOURCE_AGENT,
									gAgent.getID());
							}else
							{
								LLToolDragAndDrop::dropInventory(objectp,item,LLToolDragAndDrop::SOURCE_AGENT,gAgent.getID());
							}
						}
				}
				if (gAgent.getRegion()->getCapability("AgentPreferences").empty()) break;
				msg->newMessageFast(_PREHASH_ObjectPermissions);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_HeaderData);
				msg->addBOOLFast(_PREHASH_Override, FALSE);
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_ObjectLocalID, (U32)newid);
				msg->addU8Fast(_PREHASH_Field,	PERM_NEXT_OWNER);
				msg->addU8Fast(_PREHASH_Set, PERM_SET_TRUE);
				U32 flags = 0;
				if ( gSavedSettings.getBOOL("ObjectsNextOwnerCopy") )
				{
					flags |= PERM_COPY;
				}
				if ( gSavedSettings.getBOOL("ObjectsNextOwnerModify") )
				{
					flags |= PERM_MODIFY;
				}
				bool next_owner_trans;
				if ( next_owner_trans = gSavedSettings.getBOOL("ObjectsNextOwnerTransfer") )
				{
					flags |= PERM_TRANSFER;
				}
				msg->addU32Fast(_PREHASH_Mask, flags);
				msg->sendReliable(gAgent.getRegion()->getHost());
				if (!next_owner_trans)
				{
					msg->newMessageFast(_PREHASH_ObjectPermissions);
					msg->nextBlockFast(_PREHASH_AgentData);
					msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
					msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
					msg->nextBlockFast(_PREHASH_HeaderData);
					msg->addBOOLFast(_PREHASH_Override, false);
					msg->nextBlockFast(_PREHASH_ObjectData);
					msg->addU32Fast(_PREHASH_ObjectLocalID, (U32)newid);
					msg->addU8Fast(_PREHASH_Field,	PERM_NEXT_OWNER);
					msg->addU8Fast(_PREHASH_Set, PERM_SET_FALSE);
					msg->addU32Fast(_PREHASH_Mask, PERM_TRANSFER);
					msg->sendReliable(gAgent.getRegion()->getHost());
				}
			}
		break;
	}
}
#if 0
struct InventoryImportInfo
{
	U32 localid;
	LLAssetType::EType type;
	LLInventoryType::EType inv_type;
	LLWearableType::EType wear_type;
	LLTransactionID tid;
	LLUUID assetid;
	std::string name;
	std::string description;
	bool compiled;
	std::string filename;
	U32 perms;
};
void insert(LLViewerInventoryItem* item, LLViewerObject* objectp, InventoryImportInfo* data)
{
	if(!item)
	{
		return;
	}
	if(objectp)
	{
		LLToolDragAndDrop::dropScript(objectp,
							item,
							TRUE,
							LLToolDragAndDrop::SOURCE_AGENT,
							gAgent.getID());
	}
	delete data;
	gImportTracker.asset_insertions -= 1;
	if(gImportTracker.asset_insertions == 0)
	{
		gImportTracker.finish();
	}
}
class JCImportTransferCallback : public LLInventoryCallback
{
public:
	JCImportTransferCallback(InventoryImportInfo* idata)
	{
		data = idata;
	}
	void fire(const LLUUID &inv_item)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(inv_item);
		LLViewerObject* objectp = find(data->localid);
		insert(item, objectp, data);
	}
private:
	InventoryImportInfo* data;
};
class JCImportInventoryResponder : public LLAssetUploadResponder
{
public:
	JCImportInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type, InventoryImportInfo* idata) : LLAssetUploadResponder(post_data, vfile_id, asset_type)
	{
		data = idata;
	}
	JCImportInventoryResponder(const LLSD& post_data, const std::string& file_name,
											   LLAssetType::EType asset_type) : LLAssetUploadResponder(post_data, file_name, asset_type)
	{
	}
	void uploadComplete(const LLSD& content)
	{
		LLPointer<LLInventoryCallback> cb = new JCImportTransferCallback(data);
		LLPermissions perm;
		LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
			gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH), data->tid, data->name,
			data->description, data->type, LLInventoryType::defaultForAssetType(data->type), data->wear_type,
			LLFloaterPerms::getNextOwnerPerms("Uploads"),
			cb);
	}
	char const* getName(void) const { return "JCImportInventoryResponder"; }
private:
	InventoryImportInfo* data;
};
class JCPostInvUploadResponder : public LLAssetUploadResponder
{
public:
	JCPostInvUploadResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type, LLUUID item, InventoryImportInfo* idata) : LLAssetUploadResponder(post_data, vfile_id, asset_type)
	{
		item_id = item;
		data = idata;
	}
	JCPostInvUploadResponder(const LLSD& post_data,
								const std::string& file_name,
											   LLAssetType::EType asset_type) : LLAssetUploadResponder(post_data, file_name, asset_type)
	{
	}
	void uploadComplete(const LLSD& content)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(item_id);
		LLViewerObject* objectp = find(data->localid);
		insert(item, objectp, data);
	}
	char const* getName(void) const { return "JCPostInvUploadResponder"; }
private:
	LLUUID item_id;
	InventoryImportInfo* data;
};
class JCPostInvCallback : public LLInventoryCallback
{
public:
	JCPostInvCallback(InventoryImportInfo* idata)
	{
		data = idata;
	}
	void fire(const LLUUID &inv_item)
	{
		S32 file_size;
		LLAPRFile infile;
		infile.open(data->filename, LL_APR_RB, LLAPRFile::access_t(), &file_size);
		if (infile.getFileHandle())
		{
			LLVFile file(gVFS, data->assetid, data->type, LLVFile::WRITE);
			file.setMaxSize(file_size);
			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while ((file_size = infile.read(copy_buf, buf_size)))
			{
				file.write(copy_buf, file_size);
			}
			switch(data->type)
			{
			case LLAssetType::AT_NOTECARD:
				{
					*
					S32 size = gVFS->getSize(data->assetid, data->type);
					U8* buffer = new U8[size];
					gVFS->getData(data->assetid, data->type, buffer, 0, size);
					edit->setText(LLStringExplicit((char*)buffer));
					std::string card;
					edit->exportBuffer(card);
					cmdline_printchat("Encoded notecard");;
					edit->die();
					delete buffer;
					file.remove();
					LLVFile newfile(gVFS, data->assetid, data->type, LLVFile::APPEND);
					newfile.setMaxSize(size);
					newfile.write((const U8*)card.c_str(),size);*
					std::string agent_url = gAgent.getRegion()->getCapability("UpdateNotecardAgentInventory");
					LLSD body;
					body["item_id"] = inv_item;
					LLHTTPClient::post(agent_url, body,
								new JCPostInvUploadResponder(body, data->assetid, data->type,inv_item,data));
				}
				break;
			case LLAssetType::AT_LSL_TEXT:
				{
					std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgent");
					LLSD body;
					body["item_id"] = inv_item;
					S32 size = gVFS->getSize(data->assetid, data->type);
					U8* buffer = new U8[size];
					gVFS->getData(data->assetid, data->type, buffer, 0, size);
					std::string script((char*)buffer);
					BOOL domono = FALSE;
					*
					{
						domono = TRUE;
					}else if(script.find("//lsl2\n") != std::string::npos)
					{
						domono = FALSE;
					}*
					delete buffer;
					buffer = 0;
					body["target"] = (domono == TRUE) ? "mono" : "lsl2";
					LLHTTPClient::post(url, body, new JCPostInvUploadResponder(body, data->assetid, data->type,inv_item,data));
				}
				break;
			default:
				break;
			}
		}
	}
private:
	InventoryImportInfo* data;
};
void JCImportInventorycallback(const LLUUID& uuid, void* user_data, S32 result, LLExtStat ext_status)
{
	if(result == LL_ERR_NOERR)
	{
		InventoryImportInfo* data = (InventoryImportInfo*)user_data;
		LLPointer<LLInventoryCallback> cb = new JCImportTransferCallback(data);
		LLPermissions perm;
		LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
			gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH), data->tid, data->name,
			data->description, data->type, LLInventoryType::defaultForAssetType(data->type), data->wear_type,
			LLFloaterPerms::getNextOwnerPerms("Uploads"),
			cb);
	}else cmdline_printchat("err: "+std::string(LLAssetStorage::getErrorString(result)));
}
void ImportTracker::send_inventory(LLSD& prim)
{
	U32 local_id = prim["LocalID"].asInteger();
	if (prim.has("inventory"))
	{
		std::string assetpre = asset_dir + gDirUtilp->getDirDelimiter();
		LLSD inventory = prim["inventory"];
		for (LLSD::array_iterator inv = inventory.beginArray(); inv != inventory.endArray(); ++inv)
		{
			LLSD item = (*inv);
			InventoryImportInfo* data = new InventoryImportInfo;
			data->localid = local_id;
			LLTransactionID tid;
			tid.generate();
			LLUUID assetid = tid.makeAssetID(gAgent.getSecureSessionID());
			data->tid = tid;
			data->assetid = assetid;
			data->type = LLAssetType::lookup(item["type"].asString());
			data->name = item["name"].asString();
			data->description = item["desc"].asString();
			if(item.has("item_id"))
			{
				std::string filename = assetpre + item["item_id"].asString() + "." + item["type"].asString();
				if(LLFile::isfile(filename))
				{
					data->filename = filename;
				}else
				{
					delete data;
					continue;
				}
			}else
			{
				delete data;
				continue;
			}
			data->wear_type = NOT_WEARABLE;
			{
				data->inv_type = LLInventoryType::defaultForAssetType(data->type);
				data->compiled = false;
				switch(data->type)
				{
				case LLAssetType::AT_TEXTURE:
				case LLAssetType::AT_TEXTURE_TGA:
					{
						std::string url = gAgent.getRegion()->getCapability("NewFileAgentInventory");
						S32 file_size;
						LLAPRFile infile ;
						infile.open(data->filename, LL_APR_RB, LLAPRFile::access_t(), &file_size);
						if (infile.getFileHandle())
						{
							LLVFile file(gVFS, data->assetid, data->type, LLVFile::WRITE);
							file.setMaxSize(file_size);
							const S32 buf_size = 65536;
							U8 copy_buf[buf_size];
							while ((file_size = infile.read(copy_buf, buf_size)))
							{
								file.write(copy_buf, file_size);
							}
							LLSD body;
							body["folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
							body["asset_type"] = LLAssetType::lookup(data->type);
							body["inventory_type"] = LLInventoryType::lookup(data->inv_type);
							body["name"] = data->name;
							body["description"] = data->description;
							body["next_owner_mask"] = LLSD::Integer(U32_MAX);
							body["group_mask"] = LLSD::Integer(U32_MAX);
							body["everyone_mask"] = LLSD::Integer(U32_MAX);
							body["expected_upload_cost"] = LLAgentBenefits::current().getTextureUploadCost();
							LLHTTPClient::post(url, body, new JCImportInventoryResponder(body, data->assetid, data->type,data));
						}
					}
					break;
				case LLAssetType::AT_CLOTHING:
				case LLAssetType::AT_BODYPART:
					{
						S32 file_size;
						LLAPRFile infile;
						infile.open(data->filename, LL_APR_RB, LLAPRFile::access_t(), &file_size);
						if (infile.getFileHandle())
						{
							LLVFile file(gVFS, data->assetid, data->type, LLVFile::WRITE);
							file.setMaxSize(file_size);
							const S32 buf_size = 65536;
							U8 copy_buf[buf_size];
							while ((file_size = infile.read(copy_buf, buf_size)))
							{
								file.write(copy_buf, file_size);
							}
							LLFILE* fp = LLFile::fopen(data->filename, "rb");
							if(fp)
							{
								LLViewerWearable* wearable = new LLWearable(LLUUID::null);
								wearable->importFile( fp );
								{
									data->wear_type = wearable->getType();
								}
								delete wearable;
							}
							gAssetStorage->storeAssetData(data->tid, data->type,
												JCImportInventorycallback,
												(void*)data,
												FALSE,
												TRUE,
												FALSE);
						}
					}
					break;
				case LLAssetType::AT_NOTECARD:
					{
						LLPointer<LLInventoryCallback> cb = new JCPostInvCallback(data);
						LLPermissions perm;
						LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
						create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH), data->tid, data->name,
							data->description, data->type, LLInventoryType::defaultForAssetType(data->type), data->wear_type,
							LLFloaterPerms::getNextOwnerPerms("Notecards"),
							cb);
					}
					break;
				case LLAssetType::AT_LSL_TEXT:
					{
						LLPointer<LLInventoryCallback> cb = new JCPostInvCallback(data);
						LLPermissions perm;
						LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
						create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH), data->tid, data->name,
							data->description, data->type, LLInventoryType::defaultForAssetType(data->type), data->wear_type,
							LLFloaterPerms::getNextOwnerPerms("Scripts"),
							cb);
					}
					break;
				case LLAssetType::AT_SCRIPT:
				case LLAssetType::AT_GESTURE:
				default:
					break;
				}
				asset_insertions += 1;
			}
		}
	}
}
void ImportTracker::send_properties(LLSD& prim, int counter)
{
	if(prim.has("properties"))
	{
		if(counter == 1)
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ObjectPermissions);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addBOOLFast(_PREHASH_Override, FALSE);
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
			msg->addU8Fast(_PREHASH_Field,	PERM_NEXT_OWNER);
			msg->addU8Fast(_PREHASH_Set,	PERM_SET_TRUE);
			msg->addU32Fast(_PREHASH_Mask,		U32(atoi(prim["next_owner_mask"].asString().c_str())));
			*
			msg->newMessageFast(_PREHASH_ObjectPermissions);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addBOOLFast(_PREHASH_Override, data->mOverride);*
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
			msg->addU8Fast(_PREHASH_Field,	PERM_GROUP);
			msg->addU8Fast(_PREHASH_Set,	PERM_SET_TRUE);
			msg->addU32Fast(_PREHASH_Mask,		U32(atoi(prim["group_mask"].asString().c_str())));
			*
			msg->newMessageFast(_PREHASH_ObjectPermissions);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addBOOLFast(_PREHASH_Override, data->mOverride);*
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
			msg->addU8Fast(_PREHASH_Field,	PERM_EVERYONE);
			msg->addU8Fast(_PREHASH_Set,	PERM_SET_TRUE);
			msg->addU32Fast(_PREHASH_Mask,		U32(atoi(prim["everyone_mask"].asString().c_str())));
			msg->sendReliable(gAgent.getRegion()->getHost());
			msg->newMessageFast(_PREHASH_ObjectSaleInfo);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_LocalID, prim["LocalID"].asInteger());
			LLSaleInfo sale_info;
			BOOL a;
			U32 b;
			sale_info.fromLLSD(prim["sale_info"],a,b);
			sale_info.packMessage(msg);
			msg->sendReliable(gAgent.getRegion()->getHost());
		}
	}
}
void ImportTracker::send_vectors(LLSD& prim,int counter)
{
	LLVector3 position = ((LLVector3)prim["position"] * rootrot) + root;
	LLSD rot = prim["rotation"];
	LLQuaternion rotq;
	rotq.mQ[VX] = (F32)(rot[0].asReal());
	rotq.mQ[VY] = (F32)(rot[1].asReal());
	rotq.mQ[VZ] = (F32)(rot[2].asReal());
	rotq.mQ[VW] = (F32)(rot[3].asReal());
	LLVector3 rotation;
	if(counter == 1)
		rotation = rotq.packToVector3();
	else
		rotation = (rotq * rootrot).packToVector3();
	LLVector3 scale = prim["scale"];
	U8 data[256];
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MultipleObjectUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg->addU8Fast(_PREHASH_Type, U8(0x01));
	htonmemcpy(&data[0], &(position.mV), MVT_LLVector3, 12);
	msg->addBinaryDataFast(_PREHASH_Data, data, 12);
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg->addU8Fast(_PREHASH_Type, U8(0x02));
	htonmemcpy(&data[0], &(rotation.mV), MVT_LLQuaternion, 12);
	msg->addBinaryDataFast(_PREHASH_Data, data, 12);
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg->addU8Fast(_PREHASH_Type, U8(0x04));
	htonmemcpy(&data[0], &(scale.mV), MVT_LLVector3, 12);
	msg->addBinaryDataFast(_PREHASH_Data, data, 12);
	msg->sendReliable(gAgent.getRegion()->getHost());
}
void ImportTracker::send_shape(LLSD& prim)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectShape);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	LLVolumeParams params;
	params.fromLLSD(prim["volume"]);
	LLVolumeMessage::packVolumeParams(&params, msg);
	msg->sendReliable(gAgent.getRegion()->getHost());
}
void ImportTracker::send_image(LLSD& prim)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectImage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg->addStringFast(_PREHASH_MediaURL, NULL);
	LLPrimitive obj;
	LLSD tes = prim["textures"];
	obj.setNumTEs(U8(tes.size()));
	for (int i = 0; i < tes.size(); i++)
	{
		LLTextureEntry tex;
		tex.fromLLSD(tes[i]);
		obj.setTE(U8(i), tex);
	}
	obj.packTEMessage(gMessageSystem);
	msg->sendReliable(gAgent.getRegion()->getHost());
}
void send_chat_from_viewer(std::string utf8_out_text, EChatType type, S32 channel);
void ImportTracker::send_extras(LLSD& prim)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectExtraParams);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	LLPrimitive obj;
	if (prim.has("flexible"))
	{
		LLFlexibleObjectData flexi;
		flexi.fromLLSD(prim["flexible"]);
		U8 tmp[MAX_OBJECT_PARAMS_SIZE];
		LLDataPackerBinaryBuffer dpb(tmp, MAX_OBJECT_PARAMS_SIZE);
		if (flexi.pack(dpb))
		{
			U32 datasize = (U32)dpb.getCurrentSize();
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
			msg->addU16Fast(_PREHASH_ParamType, 0x10);
			msg->addBOOLFast(_PREHASH_ParamInUse, true);
			msg->addU32Fast(_PREHASH_ParamSize, datasize);
			msg->addBinaryDataFast(_PREHASH_ParamData, tmp, datasize);
		}
	}
	if (prim.has("light"))
	{
		LLLightParams light;
		light.fromLLSD(prim["light"]);
		U8 tmp[MAX_OBJECT_PARAMS_SIZE];
		LLDataPackerBinaryBuffer dpb(tmp, MAX_OBJECT_PARAMS_SIZE);
		if (light.pack(dpb))
		{
			U32 datasize = (U32)dpb.getCurrentSize();
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
			msg->addU16Fast(_PREHASH_ParamType, 0x20);
			msg->addBOOLFast(_PREHASH_ParamInUse, true);
			msg->addU32Fast(_PREHASH_ParamSize, datasize);
			msg->addBinaryDataFast(_PREHASH_ParamData, tmp, datasize);
		}
	}
	if (prim.has("sculpt"))
	{
		LLSculptParams sculpt;
		sculpt.fromLLSD(prim["sculpt"]);
		U8 tmp[MAX_OBJECT_PARAMS_SIZE];
		LLDataPackerBinaryBuffer dpb(tmp, MAX_OBJECT_PARAMS_SIZE);
		if (sculpt.pack(dpb))
		{
			U32 datasize = (U32)dpb.getCurrentSize();
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
			msg->addU16Fast(_PREHASH_ParamType, 0x30);
			msg->addBOOLFast(_PREHASH_ParamInUse, true);
			msg->addU32Fast(_PREHASH_ParamSize, datasize);
			msg->addBinaryDataFast(_PREHASH_ParamData, tmp, datasize);
		}
	}
	msg->sendReliable(gAgent.getRegion()->getHost());
}
void ImportTracker::send_namedesc(LLSD& prim)
{
	if(prim.has("name"))
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectName);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, prim["LocalID"].asInteger());
		msg->addStringFast(_PREHASH_Name, prim["name"]);
		msg->sendReliable(gAgent.getRegion()->getHost());
	}
	if(prim.has("description"))
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectDescription);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		LLVector3 scale = prim["scale"];
		if((scale.mV[VX] > 10.) || (scale.mV[VY] > 10.) || (scale.mV[VZ] > 10.) )
			prim["description"] = llformat("<%d, %d, %d>", (U8)scale.mV[VX], (U8)scale.mV[VY], (U8)scale.mV[VZ]);
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, prim["LocalID"].asInteger());
		msg->addStringFast(_PREHASH_Description, prim["description"]);
		msg->sendReliable(gAgent.getRegion()->getHost());
	}
}
void ImportTracker::link()
{
	if(linkset.size() == 256)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectLink);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		LLSD::array_iterator prim = linkset.beginArray();
		++prim;
		for (; prim != linkset.endArray(); ++prim)
		{
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, (*prim)["LocalID"].asInteger());
		}
		msg->sendReliable(gAgent.getRegion()->getHost());
		LLMessageSystem* msg2 = gMessageSystem;
		msg2->newMessageFast(_PREHASH_ObjectLink);
		msg2->nextBlockFast(_PREHASH_AgentData);
		msg2->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg2->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		LLSD prim2 = linkset[0];
		msg2->nextBlockFast(_PREHASH_ObjectData);
		msg2->addU32Fast(_PREHASH_ObjectLocalID, (prim2)["LocalID"].asInteger());
		prim2 = linkset[1];
		msg2->nextBlockFast(_PREHASH_ObjectData);
		msg2->addU32Fast(_PREHASH_ObjectLocalID, (prim2)["LocalID"].asInteger());
		msg2->sendReliable(gAgent.getRegion()->getHost());
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectLink);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		for (LLSD::array_iterator prim = linkset.beginArray(); prim != linkset.endArray(); ++prim)
		{
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, (*prim)["LocalID"].asInteger());
		}
		msg->sendReliable(gAgent.getRegion()->getHost());
	}
	LL_INFOS() << "FINISHED IMPORT" << LL_ENDL;
	if (linkset[0].has("Attachment"))
	{
		LL_INFOS() << "OBJECT IS ATTACHMENT, WAITING FOR POSITION PACKETS.." << LL_ENDL;
		state = POSITIONING;
		wear(linkset[0]);
	}
	else
		cleanUp();
}
void ImportTracker::wear(LLSD &prim)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectAttach);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU8Fast(_PREHASH_AttachmentPoint, U8(prim["Attachment"].asInteger()));
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg->addQuatFast(_PREHASH_Rotation, LLQuaternion(0.0f, 0.0f, 0.0f, 1.0f));
	msg->sendReliable(gAgent.getRegion()->getHost());
	LLVector3 position = prim["attachpos"];
	LLSD rot = prim["attachrot"];
	LLQuaternion rotq;
	rotq.mQ[VX] = (F32)(rot[0].asReal());
	rotq.mQ[VY] = (F32)(rot[1].asReal());
	rotq.mQ[VZ] = (F32)(rot[2].asReal());
	rotq.mQ[VW] = (F32)(rot[3].asReal());
	LLVector3 rotation = rotq.packToVector3();
	U8 data[256];
	LLMessageSystem* msg2 = gMessageSystem;
	msg2->newMessageFast(_PREHASH_MultipleObjectUpdate);
	msg2->nextBlockFast(_PREHASH_AgentData);
	msg2->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg2->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg2->nextBlockFast(_PREHASH_ObjectData);
	msg2->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg2->addU8Fast(_PREHASH_Type, U8(0x01 | 0x08));
	htonmemcpy(&data[0], &(position.mV), MVT_LLVector3, 12);
	msg2->addBinaryDataFast(_PREHASH_Data, data, 12);
	msg2->nextBlockFast(_PREHASH_ObjectData);
	msg2->addU32Fast(_PREHASH_ObjectLocalID, prim["LocalID"].asInteger());
	msg2->addU8Fast(_PREHASH_Type, U8(0x02 | 0x08));
	htonmemcpy(&data[0], &(rotation.mV), MVT_LLQuaternion, 12);
	msg2->addBinaryDataFast(_PREHASH_Data, data, 12);
	msg2->sendReliable(gAgent.getRegion()->getHost());
	LL_INFOS() << "POSITIONED, IMPORT COMPLETED" << LL_ENDL;
	cleanUp();
}
void ImportTracker::plywood_above_head()
{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectAdd);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU8Fast(_PREHASH_Material, 3);
		msg->addU32Fast(_PREHASH_AddFlags, FLAGS_CREATE_SELECTED);
		LLVolumeParams	volume_params;
		volume_params.setType(0x01, 0x10);
		volume_params.setBeginAndEndS(0.f, 1.f);
		volume_params.setBeginAndEndT(0.f, 1.f);
		volume_params.setRatio(1, 1);
		volume_params.setShear(0, 0);
		LLVolumeMessage::packVolumeParams(&volume_params, msg);
		msg->addU8Fast(_PREHASH_PCode, 9);
		msg->addVector3Fast(_PREHASH_Scale, LLVector3(0.52345f, 0.52346f, 0.52347f));
		LLQuaternion rot;
		msg->addQuatFast(_PREHASH_Rotation, rot);
		LLViewerRegion *region = gAgent.getRegion();
		if (!localids.size())
			root = (initialPos + linksetoffset);
		msg->addVector3Fast(_PREHASH_RayStart, root);
		msg->addVector3Fast(_PREHASH_RayEnd, root);
		msg->addU8Fast(_PREHASH_BypassRaycast, (U8)TRUE );
		msg->addU8Fast(_PREHASH_RayEndIsIntersection, (U8)FALSE );
		msg->addU8Fast(_PREHASH_State, (U8)0);
		msg->addUUIDFast(_PREHASH_RayTargetID, LLUUID::null);
		msg->sendReliable(region->getHost());
}
#endif
