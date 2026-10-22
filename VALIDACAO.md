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
- VU1: 651 programas compilados a partir das capturas e destinos observados.
  Programas ainda nao observados podem utilizar o interpretador do runtime.
- Base: PS2 Web Recomp, commit `6526f4803d98e10de50bea3af104a4d26e9df033`.

## Resultados

| Verificacao | Windows x86-64 | Linux x86-64 |
|---|---|---|
| Compilacao nativa e link | LLVM-MinGW / Clang 23 | Ubuntu 24.04 / Clang 18 / libc++ |
| Inicializacao a partir da ISO | 12.000 frames ate o primeiro combate | 1.200 frames de abertura |
| Combate renderizado | Movimento e ataques na primeira sala | Checkpoint Windows no frame 10.000, executado ate 10.030 |
| Progresso adicional em Rhodes | Primeira sala, portao, corredor, escada e plataforma da balista no primeiro confronto com o Colosso | Checkpoint de entrada na area do Colosso, frame 37.685 ate 37.700 |
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
| Leitura do cartao vazio pelo menu original | Mensagem de ausencia de dados, boot de 900 frames usando analogico | Mesma mensagem capturada no frame 800 |
| Teclado: analogicos, direcional digital e L3/R3 | PASS, oito casos | PASS, oito casos |
| Sessao por passos: captura, checkpoint e reinicio | PASS | PASS, llvmpipe e Dozen |

As setas do teclado movimentam apenas o analogico esquerdo. O direcional
digital, que seleciona magia, usa T/H/G/F; I/L/K/J controlam o analogico direito
e C/V correspondem a L3/R3. O teste tambem confere direcoes opostas,
remapeamento e migracao das configuracoes antigas sem substituir mapas
personalizados. O menu de carregar foi navegado com o analogico, sem o
direcional digital, na compilacao Windows atual.

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

## Limites de verificacao

- A execucao Windows chegou ao primeiro confronto com o Colosso e a plataforma
  da balista. A conclusao desse confronto, fases posteriores, campanha completa
  e finais ainda nao foram validados. Podem existir chamadas ou programas VU
  nao cobertos.
- O cartao virtual e persistente, mas salvar e carregar em um ponto de save do
  proprio jogo ainda precisa de teste. O checkpoint usado acima e do runtime.
- Remapeamento e suporte SDL a gamepads foram implementados; nenhum controle
  fisico foi conectado para testar analogicos, vibracao ou todos os botoes.
- As opcoes 2x/3x, 16:9 e diferentes GPUs ainda precisam de verificacao visual
  e medicao. Nao ha promessa de 60 FPS em qualquer maquina.
- Apenas o executavel USA com o hash acima e aceito. Outras regioes e patches
  de traducao que alterem esse executavel exigem nova recompilacao.
- O EE e recompilado, enquanto IOP e outros dispositivos ainda usam emulacao.
  Esta adaptacao nao substitui integralmente o motor original por codigo novo.
