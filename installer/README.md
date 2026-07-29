# Assets do launcher EZHELIT Store

Esta pasta contém os assets embutidos no payload único:

- `sce_sys/param.json`, com o deeplink local `http://127.0.0.1:5911/`;
- `sce_sys/icon0.png`.

Não existe mais um instalador separado. `src/app_installer.c` incorpora estes
arquivos em `ezhelit-store.elf`, registra o tile se necessário e então
`src/main.c` continua executando o servidor e o worker.

O launcher gravado em `/user/app/EZST00001` ainda contém apenas `param.json` e
`icon0.png`; o payload permanece separado e é iniciado pelo loader.
