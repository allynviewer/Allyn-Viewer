#ifndef ALLYN_PRESENCE_H
#define ALLYN_PRESENCE_H

class AllynPresence
{
public:
	static void start();
	static void stop();
	static void idle();
	static void requestStatus();
	static S32 getAllynOnline();
	static S32 getSecondLifeOnline();
	static bool hasStatus();

private:
	static bool podeEnviar();
	static void enviar(const char* acao);
};

#endif
