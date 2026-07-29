# Plano do frontend

## Objetivo

Transformar a prova de conceito Vue em uma interface simples para:

1. visualizar os jogos disponíveis no catálogo local;
2. abrir os detalhes de um jogo;
3. iniciar e confirmar um download;
4. acompanhar o download mesmo depois de fechar e reabrir a tela;
5. informar claramente quando o servidor ou o payload estiver indisponível.

Instalação, montagem e manipulação dos jogos continuam fora desta fase.

## Premissas já validadas

- Vue 3 renderiza corretamente no PS5.
- O frontend pode ser incorporado como um único HTML dentro do ELF.
- O tile abre a interface local em `http://127.0.0.1:5911/`.
- A interface consegue consultar a API do payload.
- O worker continua o download independentemente da tela.

## Estrutura de navegação

A primeira versão terá três áreas:

```text
Catálogo -> Detalhes do jogo -> Confirmação do download
    |
    +-----------------------> Downloads
```

### Catálogo

Tela inicial com:

- nome da Store;
- situação do servidor;
- grade de jogos;
- capa, título e versão em cada card;
- indicação de arquivo já baixado ou download em andamento;
- acesso à área de downloads.

### Detalhes

Ao selecionar um jogo:

- capa maior;
- título e `titleId`;
- versão;
- tamanho;
- descrição, quando existir;
- origem disponível;
- estado local;
- botão `Baixar`.

### Downloads

Lista com:

- downloads na fila;
- download ativo;
- porcentagem e bytes transferidos;
- velocidade, quando o backend fornecer;
- concluídos, cancelados e falhos;
- ações de cancelar e tentar novamente, em fases posteriores.

## Navegação pelo controle

O frontend deve ser projetado primeiro para controle, não para mouse:

- direcional: mover o foco;
- `X`: selecionar ou confirmar;
- `O`: voltar ou cancelar;
- foco sempre visível;
- ao voltar dos detalhes, restaurar o card anteriormente selecionado;
- nenhuma ação importante depender de hover;
- evitar rolagem horizontal livre e elementos pequenos.

Na primeira implementação, a navegação poderá aproveitar elementos HTML focáveis e
eventos de teclado. Um gerenciador próprio de foco só será criado se os testes no
PS5 mostrarem necessidade.

## Componentes Vue propostos

```text
App.vue
├── AppHeader
├── ServerStatus
├── CatalogView
│   ├── GameGrid
│   └── GameCard
├── GameDetailsView
├── DownloadConfirm
├── DownloadsView
│   └── DownloadItem
├── ProgressBar
└── FeedbackMessage
```

Para começar, não será necessário adicionar uma biblioteca visual. CSS próprio
reduz tamanho, dependências e riscos de incompatibilidade com a WebView.

Também não é necessário Vue Router inicialmente. O aplicativo pode manter a tela
atual em um estado simples (`catalog`, `details` ou `downloads`). Rotas serão
avaliadas apenas se navegação e histórico se tornarem complexos.

## Estado do frontend

O Vue manterá somente estado de apresentação:

- tela atual;
- jogo selecionado;
- catálogo recebido;
- carregamento e mensagens de erro;
- cópia atual dos estados de download;
- posição de foco.

O frontend não será a fonte de verdade dos downloads. Ao abrir ou reabrir a Store,
ele consulta novamente o payload e reconstrói a tela.

## Comunicação com a API local

Contratos esperados:

```text
GET  /api/v1/packages
GET  /api/v1/downloads
POST /api/v1/downloads
GET  /api/v1/downloads/{id}
POST /api/v1/downloads/{id}/cancel
POST /api/v1/downloads/{id}/retry
```

O frontend enviará `packageId` e `downloadLinkId`; nunca URL ou caminho de destino
arbitrário. A validação e o local de gravação pertencem ao payload.

Enquanto a API real não estiver pronta, o frontend usará um catálogo fictício
servido pelo próprio payload. Isso permite validar a experiência antes de conectar
o HD ou escolher Nextcloud.

O contrato fictício inicial está em
`frontend/src/mocks/packages.json`. Durante o protótipo, o Vue poderá importar
esse arquivo diretamente. Quando o endpoint estiver disponível, a importação será
substituída por `GET /api/v1/packages` sem alterar o formato consumido pelos
componentes.

## Estados visuais obrigatórios

Cada tela deve prever:

- carregando;
- conteúdo disponível;
- catálogo vazio;
- servidor remoto indisponível;
- erro inesperado;
- download aguardando;
- download em andamento;
- download concluído;
- download com falha.

Erros devem aparecer na própria interface e oferecer uma ação clara, como
`Tentar novamente`. A tela nunca deve ficar apenas vazia.

## Direção visual inicial

- fundo escuro adequado à televisão;
- cards grandes;
- texto legível à distância;
- contraste forte no foco;
- poucas informações por tela;
- animações curtas e discretas;
- área segura nas bordas;
- progresso visível sem precisar abrir os detalhes.

O primeiro objetivo é usabilidade. Identidade visual, efeitos e refinamentos
entram depois que a navegação estiver validada no console.

## Sequência de desenvolvimento

### Marco F1 — Catálogo fictício

- substituir a tela `VUE TEST` por uma grade;
- servir 4 a 6 jogos fictícios pela API local;
- exibir capa substituta, título, versão e tamanho;
- implementar estados de carregamento, vazio e erro;
- validar grade e legibilidade no PS5.

### Marco F2 — Controle e detalhes

- navegar pelos cards usando o controle;
- destacar o foco;
- abrir os detalhes com `X`;
- voltar com `O`;
- restaurar o foco no catálogo;
- validar navegação repetidamente no PS5.

### Marco F3 — Download pelo catálogo

- adicionar confirmação;
- iniciar o download usando identificadores do catálogo;
- mostrar progresso no card e nos detalhes;
- impedir pedidos duplicados;
- reabrir a Store e recuperar o estado do worker.

### Marco F4 — Área de downloads

- listar fila, ativo, concluídos e falhos;
- adicionar mensagens úteis;
- incluir cancelamento e nova tentativa quando a API suportar;
- validar perda temporária de rede.

### Marco F5 — Catálogo remoto

- trocar os dados fictícios pelo catálogo do servidor local;
- carregar capas;
- tratar URLs relativas;
- adicionar atualização manual;
- manter uma mensagem útil quando o servidor estiver fora do ar.

### Marco F6 — Refinamento

- melhorar identidade visual;
- adicionar placeholders e skeletons leves;
- reduzir requisições desnecessárias;
- revisar desempenho com catálogo grande;
- testar overscan, resoluções e textos longos.

## Critério de sucesso da primeira entrega

O Marco F1 estará concluído quando o ELF abrir no PS5, mostrar uma grade de jogos
fictícios recebidos de `/api/v1/packages` e representar corretamente carregamento,
catálogo vazio e falha, sem alterar o download funcional já existente.

## Decisões adiadas

- framework visual;
- Vue Router;
- Pinia;
- busca, filtros e categorias;
- favoritos;
- múltiplos servidores;
- autenticação;
- vídeos e imagens de fundo;
- instalação ou montagem após o download.
