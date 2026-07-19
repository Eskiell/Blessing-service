# Arquitetura inicial

## Separação de responsabilidades

### Aplicativo no PS5

- Aparecer como tile/ícone no console.
- Abrir uma interface própria em tela cheia.
- Renderizar o catálogo e receber comandos do controle.
- Consultar uma API HTTP na rede local.
- Não acessar diretamente o HD.
- Não transferir ou instalar jogos nesta fase.

### Servidor local

- Indexar e descrever a biblioteca.
- Fornecer capas, metadados e estados por uma API local.
- Futuramente fornecer os arquivos dos backups.
- Poderá usar Nextcloud ou outra solução local/open-source como armazenamento ou origem dos arquivos.

### HD de rede

- Armazenar os backups dos jogos.
- Permanecer acessível apenas dentro da rede privada.
- Não precisa ser conhecido diretamente pelo launcher.

## Fluxo da primeira versão

```text
Usuário
   |
   v
Tile do app no PS5
   |
   v
Interface nativa do homebrew
   |
   | consulta API pela rede local
   v
Servidor de catálogo
   |
   v
Dados, capas e disponibilidade dos jogos
```

## URL

Durante o desenvolvimento, a URL base da API pode ser `http://192.168.15.122:5911`.

O endereço IP não deve ser tratado como uma decisão permanente. Para evitar uma recompilação quando o servidor mudar, o planejamento deverá escolher uma das opções:

1. IP reservado no roteador, mantendo uma URL fixa.
2. Nome local resolvido por DNS, por exemplo `http://ezhelit-store.local:5911`.
3. Arquivo de configuração editável no PS5.
4. Tela de configuração no próprio app.

Para o primeiro protótipo, IP reservado e URL fixa são a opção de menor complexidade.

## Escolha arquitetural inicial

O aplicativo não deverá abrir uma página remota. Ele deverá buscar dados estruturados do servidor e renderizá-los com sua própria camada visual. O servidor HTTP atualmente implementado em `src/main.c`, que responde com uma página HTML estática, não atende a esse objetivo.

Essa constatação não autoriza remover ou reescrever o código atual. A estratégia de implementação será definida somente após a validação destes documentos.

## Evolução futura possível

```text
Interface do app no PS5
        |
        | API HTTP local
        v
Servidor de catálogo
        |
        +---- banco/índice e capas
        |
        +---- Nextcloud ou servidor de arquivos
                         |
                         v
                    HD de backups

Interface do app no PS5
        |
        | comando explícito
        v
Worker no PS5
        |
        +---- transferência com progresso
        +---- validação de integridade
        +---- integração futura com o fluxo de montagem
```

O app deve controlar e exibir as operações. Transferências longas futuras deverão pertencer a um worker no PS5 e não à tela ou ao ciclo de renderização, seguindo a separação conceitual observada no PS5 Game Compressor.

## Restrições iniciais

- A tecnologia de interface precisa ser compatível com o PS5 Payload SDK e com o ambiente homebrew disponível.
- O servidor precisa estar ativo antes de o launcher ser aberto.
- O PS5 precisa alcançar o endereço e a porta pela rede local.
- HTTP e JSON são candidatos simples para o protótipo; autenticação e HTTPS ficam para uma decisão posterior.
- A versão exata do firmware não orientará o desenho do produto, mas compatibilidade de build e execução poderá depender do ambiente homebrew usado no console.

## Interface do app

A primeira tela deverá provar quatro capacidades básicas:

1. inicializar o ambiente gráfico;
2. receber entrada do controle;
3. consultar o servidor local;
4. renderizar uma lista ou grade de jogos recebida da API.

A escolha entre uma biblioteca gráfica compatível, renderização direta ou outra camada suportada pelo SDK será investigada antes da implementação. A arquitetura não pressupõe WebView ou navegador embutido.
