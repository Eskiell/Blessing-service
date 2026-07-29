# Arquitetura inicial

## Fluxo

```text
Usuário seleciona o tile em Media
              |
              v
PS5 lê o deeplink do param.json
              |
              v
http://127.0.0.1:5911/
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

## Inicialização e instalação

Existe somente `ezhelit-store.elf`. Ao iniciar, ele verifica se os assets do
tile já estão atualizados. Quando necessário, grava `param.json` e `icon0.png`
e registra o título por `libSceAppInstUtil`. Em seguida, o mesmo processo
continua executando o servidor e o worker.

O ELF não é copiado para `/user/app`; o tile instalado continua contendo apenas
os dois assets.

## Servidor

O launcher não testa, inicia ou encerra o servidor. Se `127.0.0.1:5911` estiver indisponível, a tela não poderá ser carregada. O tratamento visual dessa indisponibilidade pertence ao mecanismo que exibe o deeplink e à aplicação servida.

## Configuração inicial

- Title ID: `EZST00001`.
- Nome: `EZHELIT Store`.
- Deeplink: `http://127.0.0.1:5911/`.
- Categoria Media: `applicationCategoryType: 65536`.
