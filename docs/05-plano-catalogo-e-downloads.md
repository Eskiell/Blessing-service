# Plano do catálogo e downloads persistentes

## Objetivo

Evoluir o protótipo de `src/main.c` para um serviço residente no PS5 que:

1. sirva a interface da EZHELIT Store;
2. consulte um catálogo hospedado na rede local;
3. aceite um pedido de download iniciado pela interface;
4. continue o download quando a tela da Store for fechada;
5. salve o arquivo em um destino interno previamente definido;
6. não instale nem monte o conteúdo nesta fase.

## Arquitetura proposta

```text
Servidor/NAS na rede local
├── GET /api/v1/packages
├── capas e metadados
└── arquivos para download
             |
             | HTTP na rede local
             v
Payload EZHELIT Store no PS5
├── servidor HTTP :5911
├── interface web embutida
├── cliente do catálogo remoto
├── API local de downloads
├── worker em segundo plano
└── estado persistido dos downloads
             |
             v
Armazenamento interno autorizado
```

O tile continua sendo apenas um deeplink. Para a versão com worker, ele deverá abrir preferencialmente `http://127.0.0.1:5911/`. O payload precisa estar em execução antes de o tile ser aberto, como acontece com Payload Manager e Game Compressor.

O endereço atual `http://192.168.15.122:5911/` funciona enquanto `192.168.15.122` for o IP do PS5, mas `127.0.0.1` evita que uma troca de IP quebre o tile.

## Responsabilidades

### Servidor da rede

- localizar os backups no HD;
- montar o catálogo;
- disponibilizar metadados e capas;
- servir os arquivos;
- não escrever no PS5 diretamente.

### Interface da Store

- solicitar o catálogo ao serviço do PS5;
- montar a grade de jogos;
- mostrar detalhes e opções de origem;
- pedir confirmação antes de baixar;
- acompanhar fila, progresso, velocidade e erros;
- não ser proprietária da execução do download.

### Worker no PS5

- buscar ou repassar o catálogo;
- validar o pedido recebido da interface;
- verificar espaço livre;
- escolher um link permitido;
- gravar primeiro em um arquivo temporário `.part`;
- persistir progresso e estado;
- continuar ativo quando a interface for fechada;
- validar tamanho e hash;
- renomear o arquivo somente após concluir e validar.

## Contrato inicial do catálogo

Endpoint remoto sugerido:

```text
GET http://SERVIDOR_LOCAL:PORTA/api/v1/packages
```

Resposta proposta:

```json
{
  "schemaVersion": 1,
  "catalog": {
    "id": "my-catalog",
    "name": "My Catalog",
    "updatedAt": "2026-07-19T23:00:00Z"
  },
  "packages": [
    {
      "id": "abcd12345-1.00",
      "titleId": "ABCD12345",
      "title": "Example Package",
      "version": "1.00",
      "format": "exfat",
      "sizeBytes": 1234567890,
      "sha256": "HASH_SHA256_DO_ARQUIVO",
      "coverUrl": "/covers/ABCD12345.jpg",
      "downloadLinks": [
        {
          "id": "local-primary",
          "name": "Servidor local",
          "type": "regular",
          "url": "/downloads/example-package.exfat"
        }
      ]
    }
  ]
}
```

Campos importantes acrescentados ao exemplo original:

- `schemaVersion`: permite evoluir o JSON sem quebrar clientes antigos;
- `id`: identifica unicamente o item, sem depender apenas do título;
- `sizeBytes`: necessário para verificar espaço e progresso;
- `sha256`: necessário para validar a integridade;
- `format`: evita inferir o formato apenas pela extensão;
- `updatedAt`: ajuda no cache e atualização do catálogo.
- `downloadLinks[].type`: classifica a origem como `regular` ou `premium`.

URLs relativas são preferíveis quando capas e arquivos estão no mesmo servidor do catálogo. O servidor ou o worker as resolve usando a URL-base configurada.

## API local do payload no PS5

Endpoints planejados:

```text
GET  /api/v1/packages
GET  /api/v1/downloads
POST /api/v1/downloads
GET  /api/v1/downloads/{id}
POST /api/v1/downloads/{id}/cancel
POST /api/v1/downloads/{id}/retry
```

Exemplo de criação:

```json
{
  "packageId": "abcd12345-1.00",
  "downloadLinkId": "local-primary"
}
```

A interface envia identificadores, não um caminho arbitrário de destino. O worker decide o diretório permitido e nunca aceita que a página escreva em qualquer lugar do sistema.

## Estados de um download

```text
queued -> checking -> downloading -> verifying -> completed
                         |               |
                         v               v
                       failed <----------+

downloading -> cancelling -> cancelled
```

O estado deverá conter pelo menos:

- identificador do trabalho;
- pacote e link escolhidos;
- arquivo temporário e destino final;
- bytes recebidos e total;
- velocidade e horário de início;
- estado atual;
- mensagem de erro;
- hash esperado e hash calculado.

## Persistência e sobrevivência

Fechar o tile ou a tela não pode cancelar o worker. A interface apenas consulta o estado atual quando for aberta novamente.

Para uma primeira versão:

- um download ativo por vez;
- fila em memória;
- estado gravado em JSON após mudanças relevantes;
- retomada HTTP usando `Range`, quando o servidor suportar;
- arquivo `.part` preservado após falha recuperável;
- operação marcada como interrompida após reinício do payload e oferecida para retomada.

O download sobrevive ao fechamento da interface enquanto o payload continuar executando. Sobreviver a reinício do console é uma capacidade posterior baseada no estado persistido e no relançamento do payload.

## Sequência de desenvolvimento

### Marco 1 — Interface estática no payload

- mover o HTML para um asset separado;
- servir `/`, CSS e JavaScript;
- manter `GET /health` para diagnóstico;
- validar abertura pelo tile.

### Marco 2 — Catálogo fictício

- implementar `GET /api/v1/packages` no payload;
- retornar JSON fictício;
- renderizar cards na tela;
- validar navegação pelo controle.

### Marco 3 — Catálogo remoto

- configurar a URL do servidor da rede;
- buscar e validar o JSON remoto;
- resolver capas e links relativos;
- mostrar servidor indisponível sem travar a interface.

### Marco 4 — Download de arquivo pequeno

- criar a API local de trabalhos;
- implementar worker separado da requisição HTTP;
- baixar um arquivo de teste para diretório temporário;
- acompanhar progresso após fechar e reabrir a tela.

### Marco 5 — Download seguro e retomável

- verificar espaço livre;
- usar `.part` e renomeação final;
- adicionar SHA-256;
- persistir estado;
- retomar downloads interrompidos;
- testar falta de espaço, rede perdida e servidor reiniciado.

### Marco 6 — Arquivo real controlado

- confirmar formato e diretório final;
- testar primeiro com um backup pequeno e não crítico;
- validar tamanho e hash;
- manter instalação e montagem fora deste marco.

## Decisões que podem esperar

- Nextcloud ou servidor próprio;
- múltiplos downloads simultâneos;
- instalação automática;
- montagem por ShadowMountPlus;
- descoberta automática do servidor;
- autenticação;
- atualização automática do payload.

## Critério de sucesso desta etapa

O planejamento estará provado quando um arquivo pequeno puder ser solicitado pela Store, continuar baixando após a tela ser fechada, aparecer com progresso ao reabrir o tile e terminar com tamanho e SHA-256 válidos.

## Validação parcial realizada

O primeiro download controlado foi concluído e salvo com sucesso usando `192.168.15.125:8080`. Isso valida conectividade, worker, escrita em `.part`, verificação por tamanho e renomeação final. SHA-256, persistência após reinício e catálogo JSON ainda não foram validados.
