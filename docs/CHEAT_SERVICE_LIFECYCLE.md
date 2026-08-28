# Ciclo de vida do serviço de cheats

O `CheatService` serializa `refresh`, consultas e toggles com um mutex. As
consultas recebem um `ServiceSnapshot` por valor, sem ponteiros para patches ou
para o estado interno, portanto uma resposta HTTP pode ser construída sem
manter o mutex ou disputar memória que esteja sendo recarregada.

## Jogo e processo

- O estado é identificado por PID, App ID e Title ID.
- Ao detectar outro processo ou jogo, o serviço descarta o arquivo e todos os
  estados de ativação anteriores e carrega o arquivo novamente.
- O serviço nunca escreve no PID anterior depois que a plataforma deixa de
  confirmá-lo como o jogo atual. Esse PID pode já ter terminado ou sido
  reutilizado, então tentar restaurá-lo seria menos seguro que descartar o
  estado local.
- No encerramento normal do serviço, cheats ativos são revertidos somente se a
  plataforma confirmar que o mesmo PID, App ID e Title ID continuam ativos.

Um encerramento abrupto do payload ou do jogo não permite uma restauração
confiável. Nesse caso, nenhuma garantia de reversão é feita: se o jogo terminou,
sua memória deixou de existir; se o payload terminou primeiro, os patches podem
permanecer até o jogo encerrar. O serviço não persiste estados de ativação e
começa desativado na próxima execução.

## Hot reload

Antes de substituir um arquivo carregado, o serviço desativa todos os cheats
ativos usando os patches antigos. Somente depois disso o repositório publica o
novo parse. Se a reversão falhar, o arquivo antigo permanece íntegro e o reload
é recusado. Se o novo arquivo for inválido, o parse anterior permanece
disponível, porém com os cheats já desativados; o erro é exposto no snapshot.

Arquivos novos nunca herdam flags `enabled`, mesmo quando preservam IDs ou
nomes. Isso evita associar um estado confirmado a patches que podem ter mudado.
