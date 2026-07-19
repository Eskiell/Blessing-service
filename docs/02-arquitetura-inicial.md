# Arquitetura inicial

## Fluxo

```text
Usuário seleciona o tile em Media
              |
              v
PS5 lê o deeplink do param.json
              |
              v
http://192.168.15.122:5911/
              |
              v
Servidor local entrega toda a tela
```

## Conteúdo instalado no PS5

```text
/user/app/EZST00001/
└── sce_sys/
    ├── param.json
    └── icon0.png
```

Não existe `eboot.elf` dentro do launcher.

## Instalação

Um instalador ELF separado é executado uma vez pelo Payload Manager. Sua única função é gravar os dois assets e registrar o tile com `libSceAppInstUtil`. Esse instalador não é incluído no launcher e não copia outro ELF para `/user/app`.

## Servidor

O launcher não testa, inicia ou encerra o servidor. Se `192.168.15.122:5911` estiver indisponível, a tela não poderá ser carregada. O tratamento visual dessa indisponibilidade pertence ao mecanismo que exibe o deeplink e, futuramente, à aplicação servida.

## Configuração inicial

- Title ID: `EZST00001`.
- Nome: `EZHELIT Store`.
- Deeplink: `http://192.168.15.122:5911/`.
- Categoria: `applicationCategoryType: 0`.
