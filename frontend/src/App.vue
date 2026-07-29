<script setup>
import {
  computed,
  nextTick,
  onBeforeUnmount,
  onMounted,
  ref,
} from 'vue'
import catalogData from './mocks/packages.json'

const packages = ref(catalogData.packages)
const catalog = ref(catalogData.catalog)
const catalogState = ref('ready')
const requestError = ref('')
const selectedPackage = ref(null)
const selectedLink = ref(null)
const confirmOpen = ref(false)
const selectedIndex = ref(-1)
const cardElements = []
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

function setCardElement(element, index) {
  if (element) cardElements[index] = element
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

function openDetails(item, index) {
  selectedPackage.value = item
  selectedLink.value = item.downloadLinks[0] || null
  selectedIndex.value = index
  window.history.pushState({ view: 'details', packageId: item.id }, '')
  nextTick(() => document.querySelector('.back-button')?.focus())
}

function closeDetails() {
  if (!selectedPackage.value) return
  window.history.back()
}

function openConfirmation() {
  if (!selectedLink.value || downloadBusy.value) return
  confirmOpen.value = true
  window.history.pushState({ view: 'confirm' }, '')
  nextTick(() => document.querySelector('.confirm-download')?.focus())
}

function cancelConfirmation() {
  if (!confirmOpen.value) return
  window.history.back()
}

function handlePopstate() {
  if (confirmOpen.value) {
    confirmOpen.value = false
    nextTick(() => document.querySelector('.start-download')?.focus())
    return
  }
  if (selectedPackage.value) {
    selectedPackage.value = null
    selectedLink.value = null
    nextTick(() => cardElements[selectedIndex.value]?.focus())
  }
}

async function confirmDownload() {
  requestError.value = ''
  try {
    const response = await fetch('/api/v1/downloads', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        packageId: selectedPackage.value.id,
        downloadLinkId: selectedLink.value.id,
      }),
    })
    if (!response.ok) throw new Error(`HTTP ${response.status}`)
    window.history.back()
    await refreshDownload()
  } catch {
    requestError.value = 'Não foi possível iniciar o download simulado'
  }
}

function handleKeydown(event) {
  if (
    (confirmOpen.value || selectedPackage.value) &&
    (event.key === 'Escape' || event.keyCode === 27)
  ) {
    event.preventDefault()
    window.history.back()
  }
}

onMounted(() => {
  refreshDownload()
  timer = window.setInterval(refreshDownload, 1000)
  window.addEventListener('popstate', handlePopstate)
  window.addEventListener('keydown', handleKeydown)
})

onBeforeUnmount(() => {
  if (timer) window.clearInterval(timer)
  window.removeEventListener('popstate', handlePopstate)
  window.removeEventListener('keydown', handleKeydown)
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

    <section v-if="!selectedPackage" class="content" aria-live="polite">
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
            :ref="(element) => setCardElement(element, index)"
            class="game-card"
            type="button"
            :style="{ '--card-index': index }"
            @click="openDetails(item, index)"
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

    <section v-else class="details">
      <button class="back-button" type="button" @click="closeDetails">
        <span aria-hidden="true">←</span>
        Voltar ao catálogo
      </button>

      <div class="details-layout">
        <div
          class="details-cover"
          :style="{ '--card-index': selectedIndex }"
          aria-hidden="true"
        >
          <span>{{ initials(selectedPackage.title) }}</span>
          <small>{{ selectedPackage.titleId }}</small>
        </div>

        <div class="details-copy">
          <p class="eyebrow">{{ selectedPackage.titleId }}</p>
          <h2>{{ selectedPackage.title }}</h2>
          <p class="details-description">
            {{ selectedPackage.description }}
          </p>

          <dl class="details-metadata">
            <div>
              <dt>Versão</dt>
              <dd>{{ selectedPackage.version }}</dd>
            </div>
            <div>
              <dt>Tamanho</dt>
              <dd>{{ formatBytes(selectedPackage.sizeBytes) }}</dd>
            </div>
            <div>
              <dt>Formato</dt>
              <dd>{{ selectedPackage.format }}</dd>
            </div>
          </dl>

          <div class="sources">
            <h3>Origens disponíveis</h3>
            <button
              v-for="link in selectedPackage.downloadLinks"
              :key="link.id"
              class="source-row"
              :class="{ selected: selectedLink?.id === link.id }"
              type="button"
              @click="selectedLink = link"
            >
              <span>{{ link.name }}</span>
              <span class="type-badge" :class="`type-${link.type}`">
                {{ link.type }}
              </span>
            </button>
          </div>

          <button
            class="start-download"
            type="button"
            :disabled="downloadBusy || !selectedLink"
            @click="openConfirmation"
          >
            {{ downloadBusy ? 'Download em andamento' : 'Baixar' }}
          </button>
        </div>
      </div>
    </section>

    <div v-if="confirmOpen" class="modal-backdrop">
      <section class="confirm-dialog" role="dialog" aria-modal="true">
        <p class="eyebrow">CONFIRMAR DOWNLOAD</p>
        <h2>{{ selectedPackage.title }}</h2>
        <p>
          Para esta validação, o payload receberá os identificadores abaixo,
          mas ainda baixará o arquivo técnico de teste.
        </p>
        <dl>
          <div>
            <dt>Pacote</dt>
            <dd>{{ selectedPackage.id }}</dd>
          </div>
          <div>
            <dt>Origem</dt>
            <dd>{{ selectedLink.name }} · {{ selectedLink.type }}</dd>
          </div>
        </dl>
        <div class="confirm-actions">
          <button type="button" @click="cancelConfirmation">Cancelar</button>
          <button
            class="confirm-download"
            type="button"
            @click="confirmDownload"
          >
            Confirmar
          </button>
        </div>
      </section>
    </div>

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
      <span>
        <kbd>{{ selectedPackage ? '○' : '✕' }}</kbd>
        {{ selectedPackage ? 'Voltar' : 'Selecionar' }}
      </span>
      <span>Frontend Vue · catálogo mock v{{ catalogData.schemaVersion }}</span>
    </footer>
  </main>
</template>
