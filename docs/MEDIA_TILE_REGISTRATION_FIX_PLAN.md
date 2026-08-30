# PR 17 — Repetir o registro do tile da aba Mídia

## Problema

O PR 16 considerava o tile atualizado apenas porque `param.json` e `icon0.png`
já estavam presentes e iguais em `/user/app/EZCHT0001/sce_sys/`. Se a primeira
chamada de registro falhasse depois da escrita, execuções seguintes retornavam
antes de chamar `SceAppInstUtil`; os arquivos existiam, mas nenhum tile aparecia
na aba Mídia.

## Escopo

- [x] Separar a atualização dos assets do registro do Title ID.
- [x] Tentar registrar `EZCHT0001` em toda execução do ELF.
- [x] Não regravar `param.json` e `icon0.png` quando estiverem iguais.
- [x] Registrar os resultados de inicialização e instalação no console.
- [x] Mostrar uma notificação nativa no PS5 com sucesso ou código da falha.
- [x] Manter qualquer falha do tile não fatal para o serviço de cheats.
- [ ] Confirmar no PS5 que o tile aparece após executar o ELF corrigido.

## Critérios de aceite

- Assets existentes não impedem uma nova tentativa de registro.
- O log informa os códigos retornados por NetCtl, UserService, AppInstUtil e
  pelo registro do título.
- A tela do PS5 mostra `tile OK` ou a etapa e o código que falharam.
- O frontend e o motor de cheats continuam iniciando se o registro falhar.
- Os testes de host e o build completo do ELF passam.

## Fora do escopo

- Remover automaticamente arquivos deixados pela tentativa anterior.
- Modificar o manifesto, o ícone, a porta ou o Title ID.
- Alterar ShellUI, instalar PKG ou iniciar o ELF automaticamente após reboot.
