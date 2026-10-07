#ifndef ALLYN_CRASHREPORT_H
#define ALLYN_CRASHREPORT_H

class AllynCrashReport
{
public:
	static void prepararSessao(bool segundaInstancia);
	static void descartarSessaoLimpa();
	static void enviarSePendente();
	static void gravarPendenteDeExcecao(unsigned long codigo);

private:
	AllynCrashReport() = delete;
};

#endif
