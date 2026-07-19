# Roadmap

## Fase 1 — Launcher em Media

- criar `param.json` com o deeplink;
- adicionar ícone;
- compilar o instalador separado;
- registrar o tile no PS5;
- confirmar que o tile abre `http://192.168.15.122:5911/`;
- confirmar que o app instalado não contém ELF.

## Fase 2 — Tela servida pela rede

- criar a interface do catálogo no servidor;
- adaptar o visual para TV e controle;
- mostrar estados de carregamento e indisponibilidade;
- validar o comportamento no PS5.

## Fase 3 — Catálogo real

- definir como o servidor acessará o HD;
- confirmar o formato real dos backups;
- indexar títulos, capas, formatos e tamanhos;
- exibir os jogos reais na interface.

## Fase 4 — Transferência

- definir um worker no PS5;
- verificar espaço livre e destino;
- transferir com progresso e retomada;
- validar integridade;
- manter transferência separada de montagem ou instalação.

## Decisões futuras

- IP fixo ou nome DNS local;
- uso ou não de Nextcloud;
- protocolo de transferência;
- integração posterior com ShadowMountPlus.
