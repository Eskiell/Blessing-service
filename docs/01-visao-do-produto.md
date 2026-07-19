# Visão do produto

## Resumo

O projeto será uma experiência privada e local para consultar uma biblioteca de backups de jogos e, futuramente, transferir conteúdo do armazenamento da rede para um PS5.

Neste momento, o objetivo não é implementar download, montagem ou instalação. A primeira entrega será um aplicativo homebrew com interface própria, exibida integralmente dentro do app no PS5. O aplicativo consultará um serviço hospedado por outro equipamento da mesma rede local.

## Problema a resolver

O usuário possui backups dos próprios jogos em um HD disponível na rede local e quer acessá-los no PS5 por meio de uma interface visual, sem depender de serviços externos.

## Ambiente conhecido

- PS5 com ambiente homebrew.
- kstuff.
- Payload Manager.
- ShadowMountPlus.
- HD e serviços mantidos em uma rede privada e local.
- Jogos atualmente descritos como arquivos ou imagens `.exFAT`.
- Endereço usado no teste atual do SDK: `http://192.168.15.122:5911`.

O endereço acima é somente um teste de validação do PS5 SDK. Ainda não existe nele a interface final nem o catálogo de jogos.

## Objetivo da primeira entrega

Disponibilizar um tile/ícone no PS5 que, quando acionado, abra o próprio aplicativo em tela cheia e apresente uma interface de catálogo.

O app será responsável por renderizar a interface. O servidor local será responsável por fornecer os dados do catálogo por uma API, e posteriormente os arquivos dos backups. A interface não dependerá do navegador do console nem de uma página HTML remota.

## Fluxo inicial do usuário

1. O serviço web local está ativo em um equipamento da rede.
2. O PS5 e o servidor estão na mesma rede local.
3. O usuário seleciona o aplicativo na tela do PS5.
4. O homebrew abre sua própria interface.
5. O app consulta a API local.
6. O catálogo é renderizado dentro do app.

## Princípios

- Funcionamento 100% local e privado.
- Nenhuma dependência obrigatória de nuvem ou serviço comercial.
- Aplicativo com interface própria, adequada para TV e controle.
- Dados do catálogo mantidos no servidor local.
- Separação entre a interface do app e a API do servidor.
- Componentes futuros desacoplados para permitir trocar Nextcloud por outra solução.
- Operações sobre backups somente mediante ação explícita do usuário.

## Fora do escopo da primeira entrega

- Descobrir ou indexar jogos no HD.
- Implementar todas as telas e recursos finais do catálogo.
- Integrar Nextcloud.
- Baixar ou copiar jogos para o PS5.
- Instalar, montar ou executar jogos.
- Gerenciar espaço livre e progresso de transferências.
- Autenticação, usuários ou acesso pela internet.

## Referência técnica

O projeto [PS5 Game Compressor](https://github.com/juma-sayeh/PS5-Game-Compressor) continua útil como referência de payload, servidor e execução de trabalhos no PS5. Entretanto, sua interface baseada no navegador não representa a experiência desejada neste projeto. Aqui, toda a apresentação e interação deverão ocorrer dentro do próprio app.

## Hipótese a validar

O termo `.exFAT` foi informado como formato atual dos jogos. Antes das fases de catálogo e transferência, será necessário confirmar se cada jogo é uma imagem/arquivo com extensão `.exfat`, uma imagem contendo um volume exFAT ou apenas conteúdo armazenado em um HD formatado como exFAT. Essa diferença não bloqueia o launcher.
