<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import catalogData from './mocks/packages.json'

const packages = ref(catalogData.packages)
const catalog = ref(catalogData.catalog)
const catalogState = ref('ready')
const requestError = ref('')
const download = ref({
  state: 'idle',
  label: 'Nenhum download ativo',
  receivedBytes: 0,
  totalBytes: 0,
  error: '',
})
let timer = null

const progress = computed(() => {
  if (!download.value.totalBytes) return 0
  return Math.min(
    100,
    Math.floor(
      (download.value.receivedBytes * 100) / download.value.totalBytes,
    ),
  )
})

const downloadBusy = computed(() =>
  ['connecting', 'downloading'].includes(download.value.state),
)

const downloadVisible = computed(
  () => download.value.state !== 'idle' || requestError.value,
)

function formatBytes(bytes) {
  if (!Number.isFinite(bytes) || bytes <= 0) return 'Tamanho desconhecido'
  const units = ['B', 'KB', 'MB', 'GB', 'TB']
  const unit = Math.min(
    Math.floor(Math.log(bytes) / Math.log(1024)),
    units.length - 1,
  )
  const value = bytes / 1024 ** unit
  return `${value.toFixed(unit >= 3 ? 1 : 0)} ${units[unit]}`
}

function initials(title) {
  return title
    .split(' ')
    .slice(0, 2)
    .map((word) => word[0])
    .join('')
    .toUpperCase()
}

function linkTypes(item) {
  return [...new Set(item.downloadLinks.map((link) => link.type))]
}

async function refreshDownload() {
  try {
    const response = await fetch('/api/v1/test-download/status', {
      cache: 'no-store',
    })
    if (!response.ok) throw new Error(`HTTP ${response.status}`)
    download.value = await response.json()
    requestError.value = ''
  } catch {
    requestError.value = 'Não foi possível consultar o serviço de downloads'
  }
}

async function startTestDownload() {
  requestError.value = ''
  try {
    const response = await fetch('/api/v1/test-download', {
      method: 'POST',
    })
    if (!response.ok) throw new Error(`HTTP ${response.status}`)
    await refreshDownload()
  } catch {
    requestError.value = 'Não foi possível iniciar o download de teste'
  }
}

function selectPackage(item) {
  // A abertura dos detalhes pertence ao Marco F2.
  requestError.value = `${item.title} selecionado — detalhes entram na próxima etapa`
}

onMounted(() => {
  refreshDownload()
  timer = window.setInterval(refreshDownload, 1000)
})

onBeforeUnmount(() => {
  if (timer) window.clearInterval(timer)
})
</script>

<template>
  <main class="screen">
    <header class="header">
      <div>
        <p class="eyebrow">CATÁLOGO LOCAL</p>
        <h1>{{ catalog.name }}</h1>
      </div>

      <div class="header-actions">
        <span class="server-status">
          <span class="status-dot" />
          Catálogo simulado
        </span>
        <button
          class="downloads-button"
          :disabled="downloadBusy"
          type="button"
          @click="startTestDownload"
        >
          {{ downloadBusy ? `${progress}% baixado` : 'Testar download' }}
        </button>
      </div>
    </header>

    <section class="content" aria-live="polite">
      <div v-if="catalogState === 'loading'" class="empty-state">
        <h2>Carregando catálogo...</h2>
      </div>

      <div v-else-if="catalogState === 'error'" class="empty-state">
        <h2>Catálogo indisponível</h2>
        <p>Verifique o servidor local e tente novamente.</p>
      </div>

      <div v-else-if="packages.length === 0" class="empty-state">
        <h2>Nenhum jogo encontrado</h2>
        <p>O catálogo está funcionando, mas ainda não possui pacotes.</p>
      </div>

      <template v-else>
        <div class="section-heading">
          <div>
            <h2>Todos os jogos</h2>
            <p>{{ packages.length }} títulos disponíveis</p>
          </div>
        </div>

        <div class="game-grid">
          <button
            v-for="(item, index) in packages"
            :key="item.id"
            class="game-card"
            type="button"
            :style="{ '--card-index': index }"
            @click="selectPackage(item)"
          >
            <span class="cover" aria-hidden="true">
              <span class="cover-mark">{{ initials(item.title) }}</span>
              <span class="cover-id">{{ item.titleId }}</span>
            </span>

            <span class="game-info">
              <span class="game-title">{{ item.title }}</span>
              <span class="game-meta">
                Versão {{ item.version }} · {{ formatBytes(item.sizeBytes) }}
              </span>
              <span class="badges">
                <span
                  v-for="type in linkTypes(item)"
                  :key="type"
                  class="type-badge"
                  :class="`type-${type}`"
                >
                  {{ type }}
                </span>
              </span>
            </span>
          </button>
        </div>
      </template>
    </section>

    <aside v-if="downloadVisible" class="download-status">
      <div class="download-copy">
        <strong>{{ requestError || download.label }}</strong>
        <span v-if="downloadBusy">
          {{ formatBytes(download.receivedBytes) }} de
          {{ formatBytes(download.totalBytes) }}
        </span>
      </div>
      <div v-if="downloadBusy || download.state === 'completed'" class="progress">
        <div class="progress-value" :style="{ width: `${progress}%` }" />
      </div>
    </aside>

    <footer class="footer">
      <span><kbd>✕</kbd> Selecionar</span>
      <span>Frontend Vue · catálogo mock v{{ catalogData.schemaVersion }}</span>
    </footer>
  </main>
</template>
