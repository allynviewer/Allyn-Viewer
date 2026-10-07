#include "llviewerprecompiledheaders.h"
#include "allyncrashreport.h"

#include "aihttpheaders.h"
#include "llappviewer.h"
#include "llbufferstream.h"
#include "lldir.h"
#include "llerrorcontrol.h"
#include "llfile.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
#include "llsd.h"
#include "llsdjson.h"
#include "llsdserialize.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"

#include <cstring>
#include <fstream>
#include <sstream>

extern S32 gLastExecDuration;

namespace
{
	const char* ALLYN_CRASH_URL = "https://allynviewer.discloud.app/api/crash";
	const char* ALLYN_CRASH_TOKEN = "allyn-viewer-crash-v1-6f3a9c18e2b74d05";
	const size_t kLogMax = 200 * 1024;
	const size_t kSessionCap = 4 * 1024 * 1024;
	const size_t kSessionKeep = 1024 * 1024;
	bool sSegunda = false;
	bool sEnvioPedido = false;
	char sSessionLog[MAX_PATH];
	char sPendingLog[MAX_PATH];
	char sEvento[MAX_PATH];
	char sStaticSrc[MAX_PATH];
	char sStaticDst[MAX_PATH];
	volatile LONG sPendenteEscrito = 0;

	std::string arquivoLog(const char* nome)
	{
		return gDirUtilp->getExpandedFilename(LL_PATH_LOGS, nome);
	}

	std::string arquivoDebug(const char* nome)
	{
		return gDirUtilp->getExpandedFilename(LL_PATH_DUMP, nome);
	}

	void copiarCaminho(char* dst, const std::string& src)
	{
		if (src.empty() || src.size() >= MAX_PATH)
		{
			dst[0] = 0;
			return;
		}
		memcpy(dst, src.c_str(), src.size() + 1);
	}

	void guardarCaminhos()
	{
		copiarCaminho(sSessionLog, arquivoLog("Allyn-session.log"));
		copiarCaminho(sPendingLog, arquivoLog("Allyn-session.pending.log"));
		copiarCaminho(sEvento, arquivoLog("Allyn-crash.event"));
		copiarCaminho(sStaticSrc, arquivoDebug("static_debug_info.log"));
		copiarCaminho(sStaticDst, arquivoLog("Allyn-debug-static.pending.xml"));
	}

	bool existe(const std::string& path)
	{
		return LLFile::isfile(path);
	}

	std::string lerFinal(const std::string& path, size_t maxBytes)
	{
		std::ifstream in(path.c_str(), std::ios::binary);
		if (!in)
			return std::string();
		in.seekg(0, std::ios::end);
		std::streamoff n = in.tellg();
		if (n <= 0)
			return std::string();
		std::streamoff start = 0;
		if ((size_t)n > maxBytes)
			start = n - (std::streamoff)maxBytes;
		in.seekg(start);
		std::string data((size_t)(n - start), '\0');
		in.read(&data[0], (std::streamsize)data.size());
		data.resize((size_t)in.gcount());
		if (start > 0)
		{
			size_t nl = data.find('\n');
			if (nl != std::string::npos)
				data.erase(0, nl + 1);
		}
		return data;
	}

	void cortarSeGrande(const std::string& path)
	{
		std::ifstream in(path.c_str(), std::ios::binary);
		if (!in)
			return;
		in.seekg(0, std::ios::end);
		std::streamoff n = in.tellg();
		if (n <= (std::streamoff)kSessionCap)
			return;
		std::streamoff start = n - (std::streamoff)kSessionKeep;
		in.seekg(start);
		std::string data((size_t)kSessionKeep, '\0');
		in.read(&data[0], (std::streamsize)data.size());
		data.resize((size_t)in.gcount());
		in.close();
		size_t nl = data.find('\n');
		if (nl != std::string::npos)
			data.erase(0, nl + 1);
		std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
		if (out)
			out.write(data.data(), (std::streamsize)data.size());
	}

	bool copiarArquivo(const std::string& de, const std::string& para)
	{
		std::ifstream in(de.c_str(), std::ios::binary);
		if (!in)
			return false;
		std::ofstream out(para.c_str(), std::ios::binary | std::ios::trunc);
		if (!out)
			return false;
		out << in.rdbuf();
		return out.good();
	}

	void apagarSeExistir(const std::string& path)
	{
		if (existe(path))
			LLFile::remove(path);
	}

	std::string minusculo(const std::string& s)
	{
		std::string o = s;
		for (size_t i = 0; i < o.size(); ++i)
		{
			unsigned char c = (unsigned char)o[i];
			if (c >= 'A' && c <= 'Z')
				o[i] = (char)(c - 'A' + 'a');
		}
		return o;
	}

	bool contem(const std::string& baixo, const char* agulha)
	{
		return baixo.find(agulha) != std::string::npos;
	}

	bool linhaDescartada(const std::string& baixo)
	{
		const char* agulhas[] = {
			"password", "passwd", "secret", "authorization", "session",
			"logging in", "from_id", "give_money", "<password", "xmlrpc",
			"home region", "crossed_region", "currentregion", "currentsimhost",
			"parcelmusic", "parcelmedia"
		};
		for (size_t i = 0; i < sizeof(agulhas) / sizeof(agulhas[0]); ++i)
		{
			if (contem(baixo, agulhas[i]))
				return true;
		}
		return false;
	}

	void trocarFaixa(std::string& s, size_t i, size_t n, const char* por)
	{
		s.replace(i, n, por);
	}

	void redactIpv4(std::string& s)
	{
		for (size_t i = 0; i < s.size();)
		{
			size_t j = i;
			int grupos = 0;
			bool ok = true;
			while (grupos < 4)
			{
				if (j >= s.size() || s[j] < '0' || s[j] > '9')
				{
					ok = false;
					break;
				}
				size_t k = j;
				int val = 0;
				while (k < s.size() && s[k] >= '0' && s[k] <= '9')
				{
					val = val * 10 + (s[k] - '0');
					if (val > 255 || k - j > 2)
					{
						ok = false;
						break;
					}
					++k;
				}
				if (!ok || k == j)
				{
					ok = false;
					break;
				}
				j = k;
				++grupos;
				if (grupos < 4)
				{
					if (j >= s.size() || s[j] != '.')
					{
						ok = false;
						break;
					}
					++j;
				}
			}
			if (ok && grupos == 4)
			{
				bool borda = (i == 0 || !((s[i - 1] >= '0' && s[i - 1] <= '9') || s[i - 1] == '.'));
				bool fim = (j >= s.size() || !((s[j] >= '0' && s[j] <= '9') || s[j] == '.'));
				if (borda && fim)
				{
					trocarFaixa(s, i, j - i, "[ip]");
					i += 4;
					continue;
				}
			}
			++i;
		}
	}

	void redactEmail(std::string& s)
	{
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] != '@')
				continue;
			size_t a = i;
			while (a > 0 && s[a - 1] != ' ' && s[a - 1] != '\t' && s[a - 1] != '<' && s[a - 1] != '"' && s[a - 1] != '\'')
				--a;
			size_t b = i + 1;
			bool ponto = false;
			while (b < s.size() && s[b] != ' ' && s[b] != '\t' && s[b] != '>' && s[b] != '"' && s[b] != '\'' && s[b] != ',')
			{
				if (s[b] == '.')
					ponto = true;
				++b;
			}
			if (a < i && ponto && b > i + 1)
			{
				trocarFaixa(s, a, b - a, "[email]");
				i = a + 6;
			}
		}
	}

	void redactUsuario(std::string& s, const char* prefixo, const char* reposicao)
	{
		std::string baixo = minusculo(s);
		std::string pref = minusculo(prefixo);
		size_t pos = 0;
		while ((pos = baixo.find(pref, pos)) != std::string::npos)
		{
			size_t fim = pos + pref.size();
			while (fim < s.size() && s[fim] != '\\' && s[fim] != '/' && s[fim] != ' ' && s[fim] != '\t' && s[fim] != '"')
				++fim;
			size_t ate = fim;
			if (ate < s.size() && (s[ate] == '\\' || s[ate] == '/'))
				++ate;
			std::string novo = reposicao;
			s.replace(pos, ate - pos, novo);
			baixo = minusculo(s);
			pos += novo.size();
		}
	}

	void redactModulo(std::string& s)
	{
		for (size_t i = 0; i < s.size();)
		{
			size_t mais = s.find("+0x", i);
			if (mais == std::string::npos && (mais = s.find("+0X", i)) == std::string::npos)
				break;
			size_t ini = mais;
			while (ini > 0 && s[ini - 1] != ' ' && s[ini - 1] != '\t' && s[ini - 1] != '"' && s[ini - 1] != '=' && s[ini - 1] != '(')
				--ini;
			size_t barra = ini;
			for (size_t p = ini; p < mais; ++p)
			{
				if (s[p] == '\\' || s[p] == '/')
					barra = p + 1;
			}
			if (barra > ini && barra < mais)
			{
				s.erase(ini, barra - ini);
				i = ini + (mais - barra);
			}
			else
			{
				i = mais + 3;
			}
		}
	}

	std::string limparTexto(const std::string& entrada)
	{
		std::string saida;
		saida.reserve(entrada.size());
		size_t i = 0;
		while (i < entrada.size())
		{
			size_t fim = entrada.find('\n', i);
			if (fim == std::string::npos)
				fim = entrada.size();
			std::string linha = entrada.substr(i, fim - i);
			if (!linha.empty() && linha[linha.size() - 1] == '\r')
				linha.erase(linha.size() - 1);
			std::string baixo = minusculo(linha);
			if (!linhaDescartada(baixo))
			{
				redactIpv4(linha);
				redactEmail(linha);
				redactUsuario(linha, "c:\\users\\", "C:\\Users\\[user]\\");
				redactUsuario(linha, "/users/", "/Users/[user]/");
				redactUsuario(linha, "/home/", "/home/[user]/");
				redactModulo(linha);
				saida += linha;
				saida += '\n';
			}
			i = fim + (fim < entrada.size() ? 1 : 0);
			if (saida.size() > kLogMax)
				break;
		}
		if (saida.size() > kLogMax)
			saida.resize(kLogMax);
		return saida;
	}

	std::string eventoNome()
	{
		switch (gLastExecEvent)
		{
		case LAST_EXEC_FROZE: return "FROZE";
		case LAST_EXEC_LLERROR_CRASH: return "LLERROR_CRASH";
		case LAST_EXEC_OTHER_CRASH: return "OTHER_CRASH";
		case LAST_EXEC_LOGOUT_FROZE: return "LOGOUT_FROZE";
		case LAST_EXEC_LOGOUT_CRASH: return "LOGOUT_CRASH";
		default: return "NORMAL";
		}
	}

	std::string ultimaLinhaCom(const std::string& texto, const char* agulha)
	{
		std::string achou;
		size_t i = 0;
		while (i < texto.size())
		{
			size_t fim = texto.find('\n', i);
			if (fim == std::string::npos)
				fim = texto.size();
			std::string linha = texto.substr(i, fim - i);
			if (linha.find(agulha) != std::string::npos)
				achou = linha;
			i = fim + (fim < texto.size() ? 1 : 0);
		}
		return achou;
	}

	std::string blocoFatal(const std::string& texto)
	{
		size_t pos = texto.rfind("FATAL_CRASH");
		if (pos == std::string::npos)
			return std::string();
		size_t fim = texto.find("\n\n", pos);
		std::string bloco = texto.substr(pos, fim == std::string::npos ? std::string::npos : fim - pos);
		if (bloco.size() > 1000)
			bloco.resize(1000);
		return bloco;
	}

	std::string hashTexto(const std::string& s)
	{
		unsigned long long h = 1469598103934665603ull;
		for (size_t i = 0; i < s.size(); ++i)
		{
			h ^= (unsigned char)s[i];
			h *= 1099511628211ull;
		}
		std::ostringstream o;
		o << std::hex << h;
		return o.str();
	}

	void limparPendentes()
	{
		apagarSeExistir(arquivoLog("Allyn-session.pending.log"));
		apagarSeExistir(arquivoLog("Allyn-debug-static.pending.xml"));
		apagarSeExistir(arquivoLog("Allyn-crash.event"));
	}

	void apagarNaoEssenciais()
	{
		const char* nomes[] = {
			"AllynUpdate.log",
			"Allyn-heap-diag.log",
			"Allyn-heap-diag.last",
			"Allyn-heap.pending.txt",
			"pbr_classic_diag.log",
			"texture_white.log"
		};
		for (size_t i = 0; i < sizeof(nomes) / sizeof(nomes[0]); ++i)
		{
			apagarSeExistir(arquivoLog(nomes[i]));
			apagarSeExistir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, nomes[i]));
		}
	}

	void apagarSessaoSemCrash()
	{
		apagarNaoEssenciais();
		apagarSeExistir(arquivoLog("Allyn-session.log"));
	}

	void gravarEvento()
	{
		std::ofstream out(arquivoLog("Allyn-crash.event").c_str(), std::ios::binary | std::ios::trunc);
		if (out)
			out << eventoNome();
	}

	std::string lerEvento()
	{
		std::ifstream in(arquivoLog("Allyn-crash.event").c_str(), std::ios::binary);
		if (!in)
			return std::string();
		std::string salvo;
		std::getline(in, salvo);
		return salvo;
	}

	void gravarEnviado(const std::string& hash)
	{
		std::ofstream out(arquivoLog("Allyn-crash.sent").c_str(), std::ios::binary | std::ios::trunc);
		if (out)
			out << hash;
	}

	std::string lerEnviado()
	{
		std::ifstream in(arquivoLog("Allyn-crash.sent").c_str(), std::ios::binary);
		if (!in)
			return std::string();
		std::stringstream ss;
		ss << in.rdbuf();
		return ss.str();
	}

	void abrirLogSessao(bool cortar)
	{
		std::string session = arquivoLog("Allyn-session.log");
		if (cortar)
			cortarSeGrande(session);
		LLError::logToFile(session);
	}

	std::string campoTexto(const LLSD& sd, const char* chave)
	{
		if (!sd.has(chave))
			return std::string();
		const LLSD& v = sd[chave];
		if (v.isString())
			return v.asString();
		if (v.isInteger())
			return llformat("%d", v.asInteger());
		if (v.isReal())
			return llformat("%.0f", v.asReal());
		return std::string();
	}
}

class AllynCrashResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	explicit AllynCrashResponder(const std::string& hash) : mHash(hash) {}

	void completedRaw(LLChannelDescriptors const&, buffer_ptr_t const&) override
	{
		sEnvioPedido = false;
		if (mStatus == HTTP_OK)
		{
			gravarEnviado(mHash);
			limparPendentes();
		}
		else if (mStatus == HTTP_BAD_REQUEST || mStatus == HTTP_UNAUTHORIZED)
		{
			limparPendentes();
		}
	}

	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const override
	{
		return responderIgnore_timeout;
	}

	char const* getName() const override
	{
		return "AllynCrashResponder";
	}

private:
	std::string mHash;
};

void AllynCrashReport::gravarPendenteDeExcecao(unsigned long codigo)
{
	if (sPendingLog[0] == 0)
		return;
	if (InterlockedCompareExchange(&sPendenteEscrito, 1, 0) != 0)
		return;
	HANDLE mutex = CreateMutexA(NULL, FALSE, "Local\\AllynViewerCrashPending");
	bool dono = false;
	if (mutex)
	{
		DWORD espera = WaitForSingleObject(mutex, 2000);
		dono = espera == WAIT_OBJECT_0 || espera == WAIT_ABANDONED;
	}
	CopyFileA(sSessionLog, sPendingLog, FALSE);
	CopyFileA(sStaticSrc, sStaticDst, FALSE);
	HANDLE arquivo = CreateFileA(sPendingLog, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (arquivo != INVALID_HANDLE_VALUE)
	{
		char linha[40];
		memcpy(linha, "Excecao Windows 0x", 17);
		unsigned long valor = codigo;
		for (int i = 7; i >= 0; --i)
		{
			linha[17 + i] = "0123456789ABCDEF"[valor & 0xF];
			valor >>= 4;
		}
		linha[25] = '\r';
		linha[26] = '\n';
		DWORD escrito = 0;
		WriteFile(arquivo, linha, 27, &escrito, NULL);
		CloseHandle(arquivo);
	}
	HANDLE evento = CreateFileA(sEvento, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (evento != INVALID_HANDLE_VALUE)
	{
		DWORD escrito = 0;
		WriteFile(evento, "OTHER_CRASH", 11, &escrito, NULL);
		CloseHandle(evento);
	}
	if (dono)
		ReleaseMutex(mutex);
	if (mutex)
		CloseHandle(mutex);
}

void AllynCrashReport::prepararSessao(bool segundaInstancia)
{
	sSegunda = segundaInstancia;
	if (!gDirUtilp)
		return;
	guardarCaminhos();
	if (segundaInstancia)
		return;
	apagarNaoEssenciais();
	bool anormal = gLastExecEvent != LAST_EXEC_NORMAL;
	std::string session = arquivoLog("Allyn-session.log");
	if (anormal)
	{
		std::string pending = arquivoLog("Allyn-session.pending.log");
		if (existe(session))
		{
			llstat stPending;
			llstat stSession;
			bool pendingMaior = LLFile::stat(pending, &stPending) == 0 && LLFile::stat(session, &stSession) == 0 && stPending.st_size >= stSession.st_size && stPending.st_size > 64;
			if (pendingMaior)
				apagarSeExistir(session);
			else
			{
				std::string extra = existe(pending) ? lerFinal(pending, 128) : std::string();
				apagarSeExistir(pending);
				LLFile::rename(session, pending);
				if (!extra.empty())
				{
					std::ofstream out(pending.c_str(), std::ios::binary | std::ios::app);
					if (out)
						out << extra;
				}
			}
		}
		std::string estatico = arquivoDebug("static_debug_info.log");
		if (existe(estatico))
			copiarArquivo(estatico, arquivoLog("Allyn-debug-static.pending.xml"));
		gravarEvento();
		abrirLogSessao(false);
	}
	else
	{
		apagarSessaoSemCrash();
		abrirLogSessao(false);
	}
}

void AllynCrashReport::descartarSessaoLimpa()
{
	if (sSegunda || !gDirUtilp)
		return;
	LLError::logToFile(std::string());
	apagarSessaoSemCrash();
}

void AllynCrashReport::enviarSePendente()
{
	if (sEnvioPedido || !gDirUtilp)
		return;
	HANDLE mutex = CreateMutexA(NULL, FALSE, "Local\\AllynViewerCrashSend");
	if (mutex)
	{
		DWORD espera = WaitForSingleObject(mutex, 0);
		if (espera != WAIT_OBJECT_0 && espera != WAIT_ABANDONED)
		{
			CloseHandle(mutex);
			return;
		}
	}
	if (gSavedSettings.getS32("CrashSubmitBehavior") == 0)
	{
		if (mutex)
		{
			ReleaseMutex(mutex);
			CloseHandle(mutex);
		}
		return;
	}
	std::string sessionPending = arquivoLog("Allyn-session.pending.log");
	std::string staticPending = arquivoLog("Allyn-debug-static.pending.xml");
	if (!existe(sessionPending) && !existe(staticPending))
	{
		if (mutex)
		{
			ReleaseMutex(mutex);
			CloseHandle(mutex);
		}
		return;
	}

	std::string log = limparTexto(lerFinal(sessionPending, 150 * 1024));
	if (log.size() > kLogMax)
		log.resize(kLogMax);

	std::string os;
	std::string cpu;
	std::string ram;
	std::string gpu;
	std::string gl;
	std::string canal = LLVersionInfo::getChannel();
	std::string versao = LLVersionInfo::getVersion();
	std::string build = llformat("%d", LLVersionInfo::getBuild());
	{
		std::ifstream in(staticPending.c_str(), std::ios::binary);
		LLSD sd;
		if (in && LLSDSerialize::fromXML(sd, in))
		{
			if (sd.has("OSInfo"))
				os = campoTexto(sd, "OSInfo");
			if (sd.has("GraphicsCard"))
				gpu = campoTexto(sd, "GraphicsCard");
			if (sd.has("CPUInfo") && sd["CPUInfo"].has("CPUString"))
				cpu = sd["CPUInfo"]["CPUString"].asString();
			if (sd.has("RAMInfo") && sd["RAMInfo"].has("Physical"))
				ram = llformat("%d", sd["RAMInfo"]["Physical"].asInteger());
			if (sd.has("GLInfo") && sd["GLInfo"].has("GLVersion"))
				gl = sd["GLInfo"]["GLVersion"].asString();
			if (sd.has("ClientInfo"))
			{
				const LLSD& info = sd["ClientInfo"];
				if (info.has("Name"))
					canal = info["Name"].asString();
				if (info.has("MajorVersion"))
				{
					versao = llformat("%d.%d.%d",
						info["MajorVersion"].asInteger(),
						info["MinorVersion"].asInteger(),
						info["PatchVersion"].asInteger());
				}
				if (info.has("BuildVersion"))
					build = llformat("%d", info["BuildVersion"].asInteger());
			}
		}
	}
	os = limparTexto(os);
	cpu = limparTexto(cpu);
	gpu = limparTexto(gpu);
	gl = limparTexto(gl);
	if (!os.empty() && os[os.size() - 1] == '\n')
		os.erase(os.size() - 1);
	if (!cpu.empty() && cpu[cpu.size() - 1] == '\n')
		cpu.erase(cpu.size() - 1);
	if (!gpu.empty() && gpu[gpu.size() - 1] == '\n')
		gpu.erase(gpu.size() - 1);
	if (!gl.empty() && gl[gl.size() - 1] == '\n')
		gl.erase(gl.size() - 1);

	std::string motivo = ultimaLinhaCom(log, "Excecao Windows");
	if (motivo.empty())
		motivo = ultimaLinhaCom(log, "ERROR:");
	if (motivo.empty())
		motivo = blocoFatal(log);
	if (motivo.empty())
		motivo = "encerrou sem exceção";
	if (motivo.size() > 1000)
		motivo.resize(1000);

	LLSD corpo;
	corpo["versao"] = versao;
	corpo["canal"] = canal;
	corpo["build"] = build;
	std::string evento = lerEvento();
	if (evento.empty())
		evento = eventoNome();
	corpo["evento"] = evento;
	corpo["duracao"] = gLastExecDuration;
	corpo["os"] = os;
	corpo["cpu"] = cpu;
	corpo["ram"] = ram;
	corpo["gpu"] = gpu;
	corpo["gl"] = gl;
	corpo["motivo"] = motivo;
	corpo["log"] = log;

	std::string payload;
	try
	{
		payload = LlsdToJson(corpo).dump();
	}
	catch (...)
	{
		corpo["log"] = std::string();
		payload = LlsdToJson(corpo).dump();
	}
	std::string hash = hashTexto(payload);
	if (lerEnviado() == hash)
	{
		limparPendentes();
		if (mutex)
		{
			ReleaseMutex(mutex);
			CloseHandle(mutex);
		}
		return;
	}
	U8* raw = new U8[payload.size()];
	memcpy(raw, payload.data(), payload.size());
	AIHTTPHeaders headers;
	headers.addHeader("Content-Type", "application/json");
	headers.addHeader("X-Allyn-Token", ALLYN_CRASH_TOKEN);
	headers.addHeader("Accept", "application/json");
	sEnvioPedido = true;
	LLHTTPClient::postRaw(ALLYN_CRASH_URL, raw, (S32)payload.size(), new AllynCrashResponder(hash), headers);
}
