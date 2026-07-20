# Roadmap

## Fase 1 — Launcher em Media

Status: concluída e validada no PS5 em 19 de julho de 2026.

- [x] criar `param.json` com o deeplink;
- [x] adicionar ícone;
- [x] compilar o instalador separado;
- [x] registrar o tile na área Media do PS5;
- [x] confirmar nome, ícone e metadados;
- [x] confirmar que o app instalado não contém ELF;
- [x] abrir o launcher no console real.

O registro correto em Media depende de `applicationCategoryType: 65536` e da inicialização dos serviços de rede, usuário e `AppInstUtil`, seguindo o fluxo do Payload Manager.

## Fase 2 — Tela servida pela rede

- evoluir `src/main.c` para servir os assets da interface;
- adaptar o visual para TV e controle;
- mostrar estados de carregamento e indisponibilidade;
- validar o comportamento no PS5.

## Fase 3 — Catálogo real

- definir como o servidor acessará o HD;
- confirmar o formato real dos backups;
- indexar títulos, capas, formatos e tamanhos;
- exibir os jogos reais na interface.

## Fase 4 — Transferência

Prova inicial concluída: download HTTP fixo executado por worker no payload e arquivo salvo com sucesso no armazenamento interno do PS5.

- implementar um worker residente no payload do PS5;
- verificar espaço livre e destino;
- transferir com progresso e retomada;
- validar integridade;
- manter transferência separada de montagem ou instalação.

Detalhes: [05-plano-catalogo-e-downloads.md](./05-plano-catalogo-e-downloads.md).

## Decisões futuras

- IP fixo ou nome DNS local;
- uso ou não de Nextcloud;
- protocolo de transferência;
- integração posterior com ShadowMountPlus.
