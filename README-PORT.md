# God of War II — Linux e Windows

Adaptacao local da versao **PS2 USA SCUS-97481**, baseada no
[PS2 Web Recomp](https://github.com/snuri00/ps2-web-recomp).
O codigo do Emotion Engine e recompilado estaticamente para C++; o runtime
ainda emula outros componentes do PS2. Nao e uma reimplementacao integral do
motor do jogo. `docs/UPSTREAM.md` descreve a arquitetura da base.

Esta e uma **previa experimental para testes**. A campanha inteira ainda nao
foi validada. O segundo confronto com o Colosso foi concluido no Windows,
com trechos de finalizacao repetidos no Linux; o primeiro altar teve save e
carga pelo menu original verificados nas duas plataformas. Consulte
`VALIDACAO.md` para o alcance dos testes e as limitacoes conhecidas.

## Abrir

- Windows: abra `dist/GodOfWarII-Windows/GodOfWarII.exe`.
- Linux x86-64: execute `dist/GodOfWarII-Linux/GodOfWarII.sh`.
- Nos pacotes compactados, extraia a pasta e abra o executavel ou script
  dentro dela, mantendo as bibliotecas e a pasta `fonts` ao lado.
- Selecione a ISO original USA e clique em **INICIAR JOGO**. A imagem permanece
  no local original; apenas o executavel de cerca de 2 MB e extraido para seu
  perfil. As duas camadas do DVD sao necessarias.
- O launcher aceita colar o caminho ou arrastar a ISO. No Linux, o botao
  Procurar utiliza `zenity`, se instalado.
- Para preparar a ISO sem janela: `GodOfWarII.exe --install "caminho.iso"`
  (ou `GodOfWarII.sh --install "/caminho/jogo.iso"` no Linux).

Requisitos do pacote Linux: glibc 2.39 ou superior (Ubuntu 24.04 ou equivalente),
SDL2 2.26 ou superior, driver grafico com Vulkan. As bibliotecas libc++
necessarias acompanham o pacote. Windows: Windows 10/11 x86-64, driver grafico
com D3D12 ou Vulkan. Nao e preciso instalar Python ou compiladores para jogar
um pacote ja compilado.

## Interface e controles

A interface usa tons escuros e dourados, painel sobre o jogo e um medidor
translucido de FPS inspirado na demonstracao fornecida. O menu original e os
graficos do jogo continuam vindo da sua ISO.

| Acao | Tecla padrao |
|---|---|
| Mover Kratos | Setas |
| Analogico direito / esquiva | I, J, K, L |
| X / confirmar | Z |
| Circulo | X |
| Quadrado | A |
| Triangulo | S |
| L1 / R1 | Q / W |
| L2 / R2 | 1 / 2 |
| Direcional digital / selecionar magia | T, F, G, H |
| L3 / R3 | C / V |
| Furia dos Titas, quando disponivel | C + V |
| Start / Select | Enter / Backspace |
| Configuracoes e pausa do runtime | F1 |
| Tela cheia | F11 |

O painel permite remapear os botoes e os dois analogicos no teclado, ajustar volume e zona morta,
exibir FPS e escolher resolucao de 1x a 3x. Resolucao e formato sao aplicados
na proxima abertura; volume e zona morta sao imediatos. O modo 16:9 exige
ativar Widescreen nas opcoes do proprio jogo. Controles reconhecidos pelo
SDL sao suportados, com ambos os analogicos e vibracao. O teste com um
controle fisico ainda esta pendente.

As setas controlam somente o analogico esquerdo. O direcional digital usa
T/F/G/H para que mover Kratos nao altere a magia selecionada. Configuracoes
antigas com as teclas padrao sao migradas; remapeamentos personalizados sao
preservados. Ao remapear, Escape cancela e Delete remove a tecla.

## Saves

O jogo utiliza um cartao de memoria virtual de 8 MB. Salve pelos pontos de
save do proprio jogo. A interface abre a pasta do perfil na aba Sobre.

- Windows: `%APPDATA%/LocalPorts/GodOfWarII/`
- Linux: `${XDG_DATA_HOME:-~/.local/share}/LocalPorts/GodOfWarII/`
- `Mcd001.ps2`: cartao; `settings.ini`: configuracoes; `SCUS_974.81`: cache
  extraido da ISO. Os arquivos de exploracao em `games/gow2` sao separados
  dos saves do jogador.

## Compilar a partir da ISO

Windows (Python 3 e Git previamente instalados):

```powershell
pwsh -File scripts/setup_windows.ps1
python scripts/build_gow2.py --iso "C:\caminho\God of War II (USA).iso" --package
```

Linux (Ubuntu 24.04):

```sh
sudo apt-get install clang-18 libc++-18-dev libc++abi-18-dev cmake ninja-build python3 libsdl2-dev libvulkan1
python3 scripts/build_gow2.py --iso "/caminho/God of War II (USA).iso" --package
```

O script valida o executavel, extrai os dois pequenos arquivos de boot,
traduz o MIPS para C++, compila, explora os destinos indiretos ausentes e
recompila os programas de VU1 observados. Nao copia a ISO. Use `--jobs 4`
para limitar compilacoes simultaneas ou `--no-explore` para recompilar uma
configuracao ja explorada. Esse modo tambem recompila os programas VU ja
capturados. Os relatorios ficam em `build/<plataforma>-gow2`, com hash do
executavel e revisao Git quando disponivel; arquivos-fonte sem Git registram
a revisao como desconhecida.

Somente o executavel com SHA256
`fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6`
e aceito por esta configuracao. Um patch de traducao que altere o executavel
exige outra recompilacao e nova validacao.

## Estado

Consulte **VALIDACAO.md**: compilar e chegar ao combate nao comprovam que
todas as fases, saves, videos e finais funcionam. Os pacotes contem codigo
recompilado e exigem sua propria ISO. A ISO, o executavel extraido, os cartoes
e os checkpoints nao acompanham o repositorio. A licenca MIT cobre o codigo
da base e das adaptacoes; os dados e o codigo original do jogo continuam
pertencendo aos respectivos titulares.

## Verificar o cartao virtual sem dados do jogo

Compile o alvo `gow2_memcard_check` e execute:

```sh
python3 tests/test_memory_card.py --runner build/linux-gow2/gow2_memcard_check
```

No Windows, use `python` e o caminho `build/windows-gow2/gow2_memcard_check.exe`.
O teste usa arquivos temporarios, grava pelo protocolo SIO2 e inicia processos
novos para conferir a releitura. Tambem verifica fechamento imediato, falha
de gravacao, preservacao do cartao anterior e nova tentativa. Isso nao substitui
salvar e carregar pelo altar dentro do jogo. O workflow `native.yml` compila
as duas plataformas com uma fixture sem dados do jogo e executa esses testes.
