# Checklist de validação no PS5

Registre console, firmware, commit, backend e jogo em cada execução. Execute o
checklist separadamente para cada firmware/backend que será declarado como
suportado.

## Instalação

- [ ] `make host-test` passa em clone limpo.
- [ ] `make all` gera o ELF e incorpora o frontend.
- [ ] O payload inicia e cria o diretório de cheats quando ausente.
- [ ] `/health` e `/api/v1/version` respondem com `Cache-Control: no-store`.
- [ ] O frontend abre pelo IP e porta configurados.
- [ ] A primeira execução instala `Blessing` na aba Mídia.
- [ ] O tile abre `http://127.0.0.1:5911/` com a internet desativada.
- [ ] Uma segunda execução não cria tile duplicado.
- [ ] Uma build com assets alterados atualiza o tile existente.
- [ ] Após reiniciar, o tile permanece e volta a funcionar quando o ELF é
  executado novamente.
- [ ] Falha induzida no instalador do tile não impede o backend de iniciar.
- [ ] Assets já presentes não impedem uma nova tentativa de registrar o tile.
- [ ] O log mostra os códigos de inicialização e registro do Title ID.
- [ ] Uma notificação `Blessing - tile OK` aparece quando o registro funciona.
- [ ] Uma falha mostra na tela a etapa e o código hexadecimal correspondente.

## Jogo e arquivos

- [ ] Sem jogo, a API permanece disponível e mostra estado vazio.
- [ ] PS4 e PS5 exibem Title ID, versão, plataforma e nome corretos.
- [ ] JSON, SHN, MC4 e ShnExt são descobertos na precedência documentada.
- [ ] Arquivo ausente resulta em lista vazia, sem crash.
- [ ] Arquivo inválido mostra erro e não publica estado parcial.
- [ ] Substituir o arquivo provoca hot reload sem reiniciar o payload.

## Memória e ciclo de vida

- [ ] Ativar um cheat altera os bytes esperados e confirma por readback.
- [ ] Desativar restaura exatamente os bytes `off`.
- [ ] Falha induzida no meio de múltiplos patches executa rollback.
- [ ] `mdbg` funciona no firmware registrado.
- [ ] `kdirect`, quando declarado suportado, funciona no firmware registrado.
- [ ] Trocar de jogo/PID não herda cheats ativos do processo anterior.
- [ ] Encerramento normal do payload reverte cheats quando o mesmo jogo vive.
- [ ] Encerramento abrupto segue o comportamento documentado, sem escrever em
  PID reutilizado.

## HTTP e frontend

- [ ] GET e polling não se sobrepõem ao toggle no frontend.
- [ ] Toggle devolve o estado efetivamente aplicado.
- [ ] Falhas de parser, módulo e memória aparecem na interface.
- [ ] ID inexistente retorna 404; sem jogo, 409; falha de aplicação, 422.
- [ ] PUT sem origem/marcador ou vindo de outra origem retorna 403.
- [ ] Requisição acima de 8 KiB é recusada sem crash ou corrupção.

## Resultado

- Firmware:
- Backend:
- Console/modelo:
- Commit:
- Jogos testados:
- Resultado e observações:
