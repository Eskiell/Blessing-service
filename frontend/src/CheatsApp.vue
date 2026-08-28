<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { getCheatState, setCheatEnabled } from './api/cheats.js'

const state = ref({ connected: false, game: null, cheats: [], backend: 'unknown' })
const loading = ref(true)
const error = ref('')
const pendingIds = ref(new Set())
let refreshTimer = null

const enabledCount = computed(() => state.value.cheats.filter((cheat) => cheat.enabled).length)

async function refresh({ quiet = false } = {}) {
  if (!quiet) loading.value = true
  try {
    state.value = await getCheatState()
    error.value = ''
  } catch {
    state.value = { connected: false, game: null, cheats: [], backend: 'unknown' }
    error.value = 'Não foi possível conectar ao serviço de cheats.'
  } finally {
    loading.value = false
  }
}

async function toggleCheat(cheat) {
  if (pendingIds.value.has(cheat.id)) return
  pendingIds.value = new Set(pendingIds.value).add(cheat.id)
  error.value = ''
  try {
    const updated = await setCheatEnabled(cheat.id, !cheat.enabled)
    state.value.cheats = state.value.cheats.map((item) =>
      item.id === updated.id ? { ...item, ...updated } : item,
    )
  } catch {
    error.value = `Não foi possível alterar “${cheat.name}”.`
  } finally {
    const next = new Set(pendingIds.value)
    next.delete(cheat.id)
    pendingIds.value = next
  }
}

onMounted(() => {
  refresh()
  refreshTimer = window.setInterval(() => refresh({ quiet: true }), 3000)
})

onBeforeUnmount(() => {
  if (refreshTimer) window.clearInterval(refreshTimer)
})
</script>

<template>
  <main class="screen">
    <header class="header">
      <div><p class="eyebrow">EZ CHEATS</p><h1>Cheats do jogo</h1></div>
      <div class="connection" :class="{ offline: !state.connected }">
        <span class="status-dot" />
        {{ state.connected ? 'Serviço conectado' : 'Serviço desconectado' }}
      </div>
    </header>

    <section v-if="state.game" class="game-panel">
      <div class="game-mark" aria-hidden="true">{{ state.game.titleId.slice(0, 4) }}</div>
      <div class="game-copy">
        <p class="eyebrow">JOGO EM EXECUÇÃO</p>
        <h2>{{ state.game.name }}</h2>
        <p>{{ state.game.titleId }} · Versão {{ state.game.version }} · {{ state.game.platform.toUpperCase() }}</p>
      </div>
      <dl class="game-stats">
        <div><dt>Cheats ativos</dt><dd>{{ enabledCount }}/{{ state.cheats.length }}</dd></div>
        <div><dt>Backend</dt><dd>{{ state.backend }}</dd></div>
      </dl>
    </section>

    <section class="content" aria-live="polite">
      <div v-if="loading" class="empty-state">
        <span class="spinner" /><h2>Procurando jogo em execução...</h2>
      </div>
      <div v-else-if="!state.game" class="empty-state">
        <div class="empty-icon">—</div><h2>Nenhum jogo detectado</h2>
        <p>Abra um jogo compatível para carregar os cheats disponíveis.</p>
        <button type="button" @click="refresh()">Tentar novamente</button>
      </div>
      <div v-else-if="state.cheats.length === 0" class="empty-state">
        <div class="empty-icon">0</div><h2>Nenhum cheat disponível</h2>
        <p>Adicione um arquivo compatível com o título e a versão do jogo.</p>
      </div>
      <template v-else>
        <div class="section-heading">
          <div><h2>Disponíveis</h2><p>{{ state.cheats.length }} opções carregadas</p></div>
          <button class="refresh-button" type="button" @click="refresh()">Atualizar</button>
        </div>
        <div class="cheat-list">
          <button
            v-for="cheat in state.cheats" :key="cheat.id" class="cheat-row"
            :class="{ enabled: cheat.enabled }" :disabled="pendingIds.has(cheat.id)"
            type="button" role="switch" :aria-checked="cheat.enabled" @click="toggleCheat(cheat)"
          >
            <span class="cheat-state" aria-hidden="true"><span class="cheat-state-thumb" /></span>
            <span class="cheat-copy"><strong>{{ cheat.name }}</strong><small>{{ cheat.description || 'Sem descrição' }}</small></span>
            <span v-if="cheat.author" class="author">por {{ cheat.author }}</span>
            <span class="state-label">{{ pendingIds.has(cheat.id) ? 'Aplicando...' : cheat.enabled ? 'Ativo' : 'Inativo' }}</span>
          </button>
        </div>
      </template>
    </section>

    <aside v-if="error" class="error-message">{{ error }}</aside>
    <footer class="footer"><span><kbd>✕</kbd> Alternar cheat</span><span>Atualização automática a cada 3 segundos</span></footer>
  </main>
</template>
