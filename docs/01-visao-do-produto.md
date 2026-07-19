# Visão do produto

## Objetivo atual

Criar um launcher na área Media do PS5 que abra exclusivamente o endereço:

```text
http://192.168.15.122:5911/
```

O conteúdo visual vem integralmente do servidor local. O launcher espera que o endereço já esteja disponível e não inicia nenhum serviço.

## Responsabilidade do launcher

- aparecer como tile na área Media;
- mostrar o nome e o ícone do projeto;
- abrir o endereço configurado por meio de um deeplink;
- não conter ou iniciar `eboot.elf`;
- não copiar, atualizar ou manipular payloads ELF;
- não acessar diretamente o HD;
- não transferir, instalar ou montar jogos.

## Ambiente conhecido

- rede 100% privada e local;
- kstuff, Payload Manager e ShadowMountPlus;
- HD com backups próprios;
- jogos descritos atualmente como `.exFAT`;
- servidor provisório em `192.168.15.122:5911`.

## Fases futuras

O servidor local poderá apresentar o catálogo do HD e, posteriormente, coordenar transferências. Nextcloud é uma possibilidade ainda não escolhida. Essas funções não fazem parte do launcher atual.

## Referência técnica

O [PS5 Game Compressor](https://github.com/juma-sayeh/PS5-Game-Compressor) usa um tile formado por `param.json` e `icon0.png`, com um `deeplinkUri`. Esse é o padrão adotado nesta fase, alterando o deeplink para o servidor da rede.
