# GodOfWaPC — God of War II para Linux e Windows

Adaptacao local de **God of War II USA (SCUS-97481)** para computadores x86-64,
baseada em [PS2 Web Recomp](https://github.com/snuri00/ps2-web-recomp).

Abra `dist/GodOfWarII-Windows/GodOfWarII.exe` no Windows ou
`dist/GodOfWarII-Linux/GodOfWarII.sh` no Linux, selecione sua ISO e clique em
**INICIAR JOGO**. Durante o jogo, **F1** abre as configuracoes e pausa a
execucao; **F11** alterna tela cheia.

- [Instrucoes, controles e compilacao](README-PORT.md)
- [Resultados de validacao e limitacoes](VALIDACAO.md)
- [Creditos e versoes](CREDITOS.md)
- [Arquitetura e documentacao original da base](docs/UPSTREAM.md)

O Emotion Engine e recompilado estaticamente para C++; outros componentes do
PS2 ainda usam emulacao no runtime. Este projeto nao e uma reimplementacao
integral do motor. Use o relatorio de validacao para distinguir comportamento
testado de funcionalidades ainda nao comprovadas. Nenhuma ISO e distribuida.
