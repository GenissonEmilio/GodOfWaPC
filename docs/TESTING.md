# Verificacao de jogo e persistencia

O CI compila o host com `tests/fixtures/empty-game` e testa o protocolo SIO2,
arquivos do cartao e renderizacao do launcher. Essa fixture nao executa o jogo
e nao e usada para alegar cobertura da campanha.

Os testes com o jogo exigem uma ISO local e uma compilacao com o executavel
correto. Nenhuma ISO, checkpoint, cartao ou codigo gerado entra no Git.

## Sessao por passos

O argumento `--interactive` inicia o host sem janela e pausa no proximo vblank.
Ele aceita comandos pela entrada padrao. Para abrir uma sessao Windows:

```powershell
python scripts/control_session.py --runner build/windows-gow2/ps2run.exe games/gow2/disc/SCUS_974.81 --disc "C:\caminho\jogo.iso" --ntsc --memcard games/gow2/teste.ps2 --load-state games/gow2/checkpoint.state
```

No Linux, use `python3` e `build/linux-gow2/ps2run`. Para registrar entradas e
saida do runtime, acrescente `--record games/gow2/sessao.txt` antes do caminho
do executavel PS2. O registro continua local e nao entra no Git.

Comandos:

- `step 120`: executa 120 frames sem botoes e pausa novamente.
- `step 6 l1+square`: segura os dois botoes durante 6 frames.
- `step 60 - lx=0 ly=128`: movimenta o analogico esquerdo para a esquerda.
- `step 18 - rx=255`: movimenta o analogico direito para a direita.
- `capture games/gow2/teste.ppm`: captura o frame pelo renderizador real.
- `save games/gow2/checkpoint.state`: avanca 2 frames, salva no primeiro e
  pausa no segundo, mantendo a ultima entrada. Assim, o evento de vblank
  seguinte acompanha o checkpoint. Use `step 1 -` antes se quiser soltar os botoes.
- `quit`: fecha normalmente e grava alteracoes pendentes do cartao.

Use os nomes de botoes do argumento `--press`: `cross`, `circle`, `square`,
`triangle`, `l1`, `r1`, `l2`, `r2`, `start`, `select`, `l3`, `r3`, as direcoes
digitais e `lup`, `ldown`, `lleft`, `lright` para o analogico esquerdo. Ao passar
eixos explicitamente, prefira `lx`/`ly` em vez de `lup`/`lleft` na mesma acao.
Essa sessao controla apenas o runtime em teste e nao sintetiza teclas do sistema.

Teste de integracao com um checkpoint proprio:

```sh
python3 tests/test_step_control.py --runner build/linux-gow2/ps2run --elf games/gow2/disc/SCUS_974.81 --iso /caminho/jogo.iso --checkpoint games/gow2/checkpoint.state
```

## Diagnostico da GPU

O host utiliza os valores padrao do wgpu e aceita suas variaveis de ambiente
de diagnostico, como `WGPU_VALIDATION=1`. Se a selecao de adaptador falhar, o
log inclui o estado e a mensagem retornados pelo wgpu, antes da tentativa do
renderizador por software. Confira a linha `[gpu]` do log para identificar o
adaptador realmente usado.

`WGPU_ALLOW_UNDERLYING_NONCOMPLIANT_ADAPTER=1` permite testar explicitamente
um driver nao conforme. Essa opcao permanece desativada por padrao; resultados
obtidos com ela devem identificar o driver e o ambiente utilizados.

## Criterio para campanha e saves

Um retorno zero e a contagem de frames nao comprovam progresso na campanha.
Registre a area visivel, as acoes realizadas e o resultado de cada trecho.
Para validar um save do jogo: alcance um altar, salve pela interface original,
encerre o processo, inicie sem `--load-state`, carregue pelo menu original e
confira local, inventario e progresso. Repita nas duas plataformas, com o
cartao correspondente. Checkpoints do runtime e cartoes de teste SIO2 devem
ser identificados separadamente dos saves feitos no altar.
