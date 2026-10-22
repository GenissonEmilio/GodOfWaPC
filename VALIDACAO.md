# Validacao local

Atualizado em: 22/10/2026. Estado: **adaptacao experimental executavel nas duas plataformas**.
A campanha completa ainda nao foi validada; este relatorio nao certifica um
port integral do jogo.

## Disco e recompilacao

- ISO USA de 8.533.671.936 bytes, volume `GODOFWAR2`, duas camadas reconhecidas.
- Boot `cdrom0:\SCUS_974.81;1`, versao 1.01, NTSC.
- Executavel de 2.125.688 bytes; CRC PCSX2 `2F123FD8`.
- SHA256 do executavel:
  `fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6`.
- Traducao EE: 4.444 funcoes, 477.499 instrucoes, 118 unidades C++.
- VU1: 669 programas compilados a partir das capturas e destinos observados.
  Programas ainda nao observados podem utilizar o interpretador do runtime.
- Base: PS2 Web Recomp, commit `6526f4803d98e10de50bea3af104a4d26e9df033`.

## Resultados

| Verificacao | Windows x86-64 | Linux x86-64 |
|---|---|---|
| Compilacao nativa e link | LLVM-MinGW / Clang 23 | Ubuntu 24.04 / Clang 18 / libc++ |
| Inicializacao a partir da ISO | 12.000 frames ate o primeiro combate | 1.200 frames de abertura |
| Combate renderizado | Movimento e ataques na primeira sala | Checkpoint Windows no frame 10.000, executado ate 10.030 |
| Progresso adicional em Rhodes | Primeira sala, portao, corredor, escada; primeiro confronto com o Colosso concluido e chegada ao banho | Trechos reproduzidos do combate, balista, comandos e chegada ao banho a partir dos checkpoints Windows |
| Launcher e fonte | Captura do renderizador SDL | Captura do renderizador SDL |
| F1 pausa o jogo e mantem painel ativo | PASS, frame estavel durante 600 ms | PASS, frame estavel durante 600 ms |
| Retomar pelo F1 | PASS | PASS |
| Audio PCM nao silencioso | Abertura e combate; pico de 26.214 no combate | Abertura e combate; pico de 17.887 no combate |
| Preparar ISO pelo `--install` | PASS | PASS |
| Leitura DVD9 acima de 4 GiB | Tres setores conferidos | Mesmos tres setores conferidos |
| Caminhos com acentos | ISO e cartao com nomes Unicode, 60 frames | Caminhos POSIX usados nos demais testes |
| Cartao SIO2: gravar e reler apos reiniciar processo | PASS, 10 execucoes | PASS, 10 execucoes |
| Cartao SIO2: falha, nova tentativa e substituicao | PASS, arquivo anterior preservado | PASS, arquivo anterior preservado |
| Cartao SIO2: gravacao forcada ao fechar | PASS | PASS |
| Save no altar, reinicio do processo e carga pelo menu original | PASS, slot 1 em Rhodes Bathhouse; inventario conferido | PASS, slot 2 gravado no Linux e carregado em novo processo; inventario conferido |
| Leitura do cartao vazio pelo menu original | Mensagem de ausencia de dados, boot de 900 frames usando analogico | Mesma mensagem, boot de 900 frames com Dozen |
| Teclado: analogicos, direcional digital e L3/R3 | PASS, oito casos | PASS, oito casos |
| Sessao por passos: captura, checkpoint e reinicio | PASS | PASS, llvmpipe e Dozen |
| Abrir bau verde pela interface do jogo | PASS, sequencia de 152 frames | PASS; na compilacao com 669 programas VU, diferenca RGB media de 0,00445 por canal |
| Ataques contra o Colosso e tutorial de orbes azuis | PASS, replay de 3.650 frames | PASS, mesma sequencia; diferenca RGB media de 0,00422 por canal |
| Balista, quatro comandos de combate e transicao ao banho | PASS, replay de 2.644 frames na compilacao com 669 programas VU; imagem RGB identica a referencia | PASS, mesma sequencia e imagem RGB identica |
| Balanco no gancho e salto ate o corredor de saida | PASS, replay de 185 frames | PASS, mesma sequencia e imagem RGB identica |
| Escada e sequencia do pe do Colosso | PASS, replay de 660 frames; parede rompida | PASS, mesma sequencia e imagem RGB identica |
| Saida da parede para o chao da segunda arena | PASS, replay de 46 frames; apresentacao do confronto acionada | PASS, mesma sequencia e imagem RGB identica |
| Segunda luta: sequencia Circulo, Circulo, Quadrado, Quadrado | PASS, replay de 1.114 frames | PASS, mesma sequencia e imagem RGB identica |
| Segunda luta: Triangulo, Triangulo, X, X, resistencia e chegada ao palacio | PASS, replay de 2.724 frames | PASS, mesma sequencia e imagem RGB identica |

As setas do teclado movimentam apenas o analogico esquerdo. O direcional
digital, que seleciona magia, usa T/H/G/F; I/L/K/J controlam o analogico direito
e C/V correspondem a L3/R3. O teste tambem confere direcoes opostas,
remapeamento e migracao das configuracoes antigas sem substituir mapas
personalizados. O menu de carregar foi navegado com o analogico, sem o
direcional digital, nas duas compilacoes atuais. Os testes de 900 frames
terminaram normalmente: 18,75 segundos no Windows e 78,47 segundos no Linux
WSL2/Dozen. Esses tempos incluem o boot e nao sao uma medicao de FPS em combate.

Os replays locais associam cada resultado ao SHA256 do executavel, do
checkpoint e da sequencia de comandos. A comparacao de imagem verifica o
quadro observado ao fim do trecho; nao certifica todas as animacoes ou a
campanha inteira. Na compilacao com 669 programas VU, o Windows repetiu tambem
a balista e os quatro comandos do primeiro confronto, chegando ao banho no
frame 51.145 com imagem identica a referencia. Os dois trechos foram repetidos
no Linux: ataques ate o tutorial de orbes azuis no frame 47.248 e balista ate
o banho no frame 51.145. A imagem final da balista foi identica nos dois sistemas.

Tambem foram repetidos nas duas plataformas o balanco e salto pelo gancho
(frames 7.835 a 8.020 de uma nova sessao) e a subida com a sequencia do pe do
Colosso (13.151 a 13.811 dessa sessao). As capturas finais dos dois trechos
foram identicas. A transicao da parede para o chao da segunda arena tambem foi
repetida nas duas compilacoes, com imagem identica no frame 20.299 de outra
sessao. O segundo confronto com o Colosso foi concluido no Windows, com
chegada a Rhodes Palace. As duas sequencias de quatro botoes foram repetidas
nas duas plataformas: a primeira ate o frame 26.425 e a segunda, incluindo a
resistencia com L1/R1 e a transicao para o palacio, ate o frame 33.587. As
imagens finais foram identicas. A captura de chegada ao palacio ainda faz
parte da cena de entrada; movimentacao e abertura do bau seguinte foram
observadas depois no Windows. Esses replays partem de checkpoints e nao
representam uma campanha continua completa nas duas plataformas.

A previa experimental e disponibilizada para testes do usuario antes da
validacao integral da campanha. O restante da campanha e os demais altares
permanecem pendentes; somente o primeiro altar tem save e carga a frio
verificados nas duas plataformas.

O teste Linux do menu com limite de 1.400 frames excedeu seu prazo de 240
segundos sob llvmpipe e foi interrompido no frame 925. A captura do frame 800
ja mostrava a resposta esperada para o cartao vazio. Esse caso funcional foi
observado, mas a execucao longa nao e registrada como aprovada.

Os testes de PCM medem as amostras produzidas pelo runtime com dispositivo de
audio de teste. Nao equivalem a uma revisao auditiva de todas as musicas, falas
e efeitos. A execucao de 12.000 frames terminou normalmente pelo limite de
teste, sem destinos EE ausentes. A verificacao final Windows de 600 frames
tambem terminou normalmente, apos implementar a consulta `cdvdman #50` usada
no boot. O log anterior de entrada no jogo contem `ERROR 55 84010002` emitido
pelo IOP; o combate continuou e produziu PCM. A origem dessa mensagem ainda
precisa ser investigada.

O leitor DVD9 foi comparado com leitura independente da ISO por Python, usando
os setores 2.100.523, 4.166.817 e 4.166.831. Hashes FNV-1a de 64 bits:
`971199dc2fa20692`, `76b05f80d77e3034`, `f307c2a04663c84d`.

As imagens dos menus nas duas plataformas tiveram diferenca media inferior a
0,16 por canal na escala 0-255. Os primeiros testes Linux foram executados no
WSL2, com Vulkan Mesa llvmpipe por CPU; isso verifica funcionamento, sem
demonstrar desempenho de uma GPU Linux real ou equivalencia de todos os drivers.
Nesse ambiente, o driver tambem emitiu avisos `libunwind: bad fde` durante o
teste do pacote. A execucao, o painel e o PCM continuaram funcionando.

Em 22/10, foi compilado Mesa Dozen 25.2.8 em uma pasta isolada do WSL2, sem
substituir o driver do sistema. Com a opcao explicita de diagnostico para
adaptadores nao conformes, o runtime selecionou `Microsoft Direct3D12
(Intel(R) Iris(R) Xe Graphics)`. A captura da area do Colosso e o teste de
avanco de frames, captura, checkpoint e reinicio passaram. Dozen se identifica
como implementacao Vulkan nao conforme para testes; ele nao acompanha o
pacote Linux. Esse resultado nao substitui validacao em Linux nativo e outros
drivers.

Os pacotes foram executados a partir de outra pasta, com `PATH` restrito aos
utilitarios do sistema: preparacao da ISO, captura do launcher e combate a
partir do checkpoint com pausa, retomada e PCM. A verificacao dos arquivos
SHA256 e dos arquivos compactados passou. O arquivo Linux preserva permissao
de execucao para o script e o binario.

Logs, checkpoints e capturas locais ficam em `games/gow2/`, fora dos pacotes.
Cada pacote inclui `build-report.json` e `SHA256SUMS.txt`. A ISO continua no
caminho fornecido pelo usuario e nao acompanha os pacotes.

As novas verificacoes de persistencia utilizam o protocolo SIO2 do runtime e
arquivos reais, em processos separados. Corrigiram-se o descarte do estado
pendente apos falha de escrita, a substituicao direta do arquivo anterior e a
ausencia de gravacao final ao sair antes de 50 frames. Os cartoes de teste nao
contem um save da campanha. A compilacao atual grava em arquivo temporario,
substitui o destino somente apos sucesso e conserva a tentativa em memoria
quando a escrita falha.

O save real Windows foi criado pela interface do altar de Rhodes Bathhouse,
incluindo a formatacao do cartao novo. O jogo confirmou `Save Complete`.
Apos encerrar o processo, outro processo iniciou sem `--load-state`, leu o
slot 1 pelo menu original e retornou ao altar. O inventario restaurou as
Laminas de Atena no nivel 5 e 624 orbes vermelhos; uma captura do inventario
anterior ao save foi comparada a captura apos a carga (diferenca media RGB
0,166). Isso valida esse save e esse ponto da campanha no Windows.

No Linux, uma copia do cartao Windows foi lida pelo menu original. O jogo
restaurou o altar e gravou o progresso no slot 2 desse cartao separado,
confirmando `Save Complete`. Outro processo Linux, tambem sem checkpoint,
carregou o slot 2 e restaurou o altar, as Laminas de Atena no nivel 5 e os
624 orbes. A diferenca media RGB do inventario em relacao a captura Windows
foi 0,311 por canal. Os dois processos Linux encerraram normalmente. O cartao
Windows inicial foi preservado. Isso verifica tambem a transferencia de um
save Windows para Linux; os testes usam o ambiente WSL2/Dozen descrito acima.

## Limites de verificacao

- A execucao Windows concluiu os dois primeiros confrontos com o Colosso e
  chegou a Rhodes Palace. Os trechos indicados foram repetidos no Linux.
  Fases posteriores, campanha completa e finais ainda nao foram validados.
  Podem existir chamadas ou programas VU nao cobertos.
- Salvar e carregar passou nos dois sistemas no primeiro altar. Os demais
  pontos de save e a campanha completa ainda precisam de teste. Os checkpoints
  dos replays continuam separados dos saves originais do jogo.
- Remapeamento e suporte SDL a gamepads foram implementados; nenhum controle
  fisico foi conectado para testar analogicos, vibracao ou todos os botoes.
- As opcoes 2x/3x, 16:9 e diferentes GPUs ainda precisam de verificacao visual
  e medicao. Nao ha promessa de 60 FPS em qualquer maquina.
- Apenas o executavel USA com o hash acima e aceito. Outras regioes e patches
  de traducao que alterem esse executavel exigem nova recompilacao.
- O EE e recompilado, enquanto IOP e outros dispositivos ainda usam emulacao.
  Esta adaptacao nao substitui integralmente o motor original por codigo novo.
