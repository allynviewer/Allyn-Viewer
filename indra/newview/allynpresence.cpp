#include "llviewerprecompiledheaders.h"
#include "allynpresence.h"

#include <sstream>

#include "aihttpheaders.h"
#include "hippogridmanager.h"
#include "llagent.h"
#include "llbufferstream.h"
#include "llhttpclient.h"
#include "llsdjson.h"
#include "llversioninfo.h"

namespace
{
	const char* ALLYN_PRESENCE_URL = "https://allynviewer.discloud.app/api/presenca";
	const char* ALLYN_STATUS_URL = "https://allynviewer.discloud.app/api/status";
	const char* ALLYN_PRESENCE_TOKEN = "allyn-viewer-presenca-v1-9c4e2b71a8d6f3";
	const F32 ALLYN_PRESENCE_INTERVALO = 60.f;
	const F32 ALLYN_STATUS_INTERVALO = 30.f;

	bool sAtivo = false;
	bool sBuscandoStatus = false;
	bool sPrimeiroStatus = true;
	S32 sSecondLifeOnline = -1;
	S32 sAllynOnline = -1;
	LLFrameTimer sHeartbeat;
	LLFrameTimer sStatusTimer;
}

class AllynStatusResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer) override
	{
		sBuscandoStatus = false;
		if (mStatus != HTTP_OK)
		{
			return;
		}

		LLBufferStream istr(channels, buffer.get());
		std::stringstream corpo;
		corpo << istr.rdbuf();
		LLSD dados = LlsdFromJsonString(corpo.str());
		if (!dados.isMap())
		{
			return;
		}

		if (dados.has("secondLifeOnline"))
		{
			sSecondLifeOnline = dados["secondLifeOnline"].asInteger();
		}
		if (dados.has("allynOnline"))
		{
			sAllynOnline = dados["allynOnline"].asInteger();
		}
	}

	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const override
	{
		return responderIgnore_timeout;
	}

	char const* getName() const override
	{
		return "AllynStatusResponder";
	}
};

bool AllynPresence::podeEnviar()
{
	if (gAgent.getID().isNull())
	{
		return false;
	}
	if (!gHippoGridManager || !gHippoGridManager->getConnectedGrid())
	{
		return false;
	}
	return gHippoGridManager->getConnectedGrid()->isSecondLife();
}

void AllynPresence::enviar(const char* acao)
{
	if (!podeEnviar())
	{
		return;
	}

	LLSD corpo;
	corpo["id"] = gAgent.getID().asString();
	corpo["versao"] = LLVersionInfo::getVersion();
	corpo["acao"] = std::string(acao);

	AIHTTPHeaders headers;
	headers.addHeader("X-Allyn-Token", ALLYN_PRESENCE_TOKEN);
	headers.addHeader("Accept", "application/json");

	LLHTTPClient::post(ALLYN_PRESENCE_URL, corpo, new LLHTTPClient::ResponderIgnore(), headers);
}

void AllynPresence::requestStatus()
{
	if (sBuscandoStatus)
	{
		return;
	}

	sBuscandoStatus = true;
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "application/json");
	LLHTTPClient::get(ALLYN_STATUS_URL, new AllynStatusResponder(), headers);
}

S32 AllynPresence::getAllynOnline()
{
	return sAllynOnline;
}

S32 AllynPresence::getSecondLifeOnline()
{
	return sSecondLifeOnline;
}

bool AllynPresence::hasStatus()
{
	return sSecondLifeOnline >= 0 || sAllynOnline >= 0;
}

void AllynPresence::start()
{
	if (!podeEnviar())
	{
		return;
	}

	sAtivo = true;
	sHeartbeat.reset();
	LL_INFOS("AllynPresence") << "Presença iniciada para " << gAgent.getID() << LL_ENDL;
	enviar("heartbeat");
}

void AllynPresence::stop()
{
	if (!sAtivo)
	{
		return;
	}

	enviar("logout");
	sAtivo = false;
	LL_INFOS("AllynPresence") << "Presença encerrada" << LL_ENDL;
}

void AllynPresence::idle()
{
	if (sPrimeiroStatus || sStatusTimer.getElapsedTimeF32() >= ALLYN_STATUS_INTERVALO)
	{
		sPrimeiroStatus = false;
		sStatusTimer.reset();
		requestStatus();
	}

	if (!sAtivo)
	{
		return;
	}

	if (sHeartbeat.getElapsedTimeF32() >= ALLYN_PRESENCE_INTERVALO)
	{
		sHeartbeat.reset();
		enviar("heartbeat");
	}
}
