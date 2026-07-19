# Roadmap

## Fase 0 — Entendimento e planejamento

Objetivo: estabelecer o produto, a arquitetura e os limites antes de alterar o código.

Critérios de conclusão:

- visão do produto validada;
- responsabilidade do aplicativo validada;
- arquitetura local validada;
- decisões pendentes registradas;
- primeira entrega claramente delimitada.

## Fase 1 — Aplicativo mínimo

Objetivo: instalar/exibir um tile no PS5, abrir uma interface própria e apresentar dados obtidos de uma API local.

Critérios de sucesso:

- o tile aparece no PS5;
- o usuário consegue iniciá-lo pelo controle;
- o app abre sem depender do navegador;
- uma tela própria é renderizada;
- o controle permite navegar pela interface;
- o app consulta a URL base configurada;
- uma lista de teste fornecida pela API é exibida;
- falha de servidor indisponível não altera ou apaga dados do console;
- nenhuma função de transferência ou instalação é executada.

## Fase 2 — Servidor e catálogo local

Objetivo: construir a aplicação web que encontra e apresenta os backups armazenados no HD.

Entregas previstas:

- serviço local/open-source;
- indexação da biblioteca;
- API do catálogo;
- API local do catálogo e origem dos arquivos;
- capas, títulos, tamanhos e formatos;
- estados de vazio, carregamento e servidor indisponível.

Nextcloud é uma possibilidade, não uma decisão. Antes de adotá-lo, deverá ser comparado com uma API/servidor menor e específico para o projeto.

## Fase 3 — Transferência para o PS5

Objetivo: copiar um backup escolhido para um destino autorizado no console.

Requisitos mínimos previstos:

- confirmação explícita;
- verificação de espaço livre;
- progresso, velocidade e estimativa;
- retomada ou tratamento seguro de interrupções;
- validação de integridade;
- separação entre download e instalação/montagem;
- worker independente da tela atualmente aberta no app.

## Fase 4 — Integração com montagem/uso

Objetivo: estudar e implementar, quando tecnicamente apropriado, a integração com kstuff, Payload Manager e ShadowMountPlus para backups próprios.

Esta fase depende da confirmação do formato real dos backups e de testes controlados. Não faz parte do launcher inicial.

## Decisões pendentes

### Necessárias antes de implementar a Fase 1

- Qual camada gráfica será usada para renderizar a interface do app.
- Como será feita a leitura do controle.
- Qual cliente HTTP e parser JSON serão usados no ambiente do PS5.
- Se o launcher será instalado permanentemente ou iniciado pelo Payload Manager.
- URL provisória e estratégia para manter o endereço do servidor estável.
- Identificador, nome e ícone definitivos do app.

### Necessárias antes da Fase 2

- Equipamento e sistema operacional que hospedarão o servidor.
- Organização real dos arquivos no HD.
- Significado preciso do formato `.exFAT` informado.
- Metadados disponíveis para cada jogo.
- Nextcloud, compartilhamento de arquivos ou servidor específico.

### Necessárias antes das Fases 3 e 4

- Destino permitido no PS5.
- Protocolo de transferência.
- Formato que ShadowMountPlus espera encontrar.
- Estratégia de checksum e retomada.
- Comportamento após falha, falta de espaço ou reinicialização.

## Próxima validação

Antes de escrever código, validar com o usuário:

1. O entendimento registrado representa o produto desejado?
2. A API da Fase 1 deve usar inicialmente `http://192.168.15.122:5911` como endereço base?
3. O tile deverá ser permanente na tela do PS5?
4. Qual nome deverá aparecer no tile?
