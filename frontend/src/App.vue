<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'

const status = ref({
  state: 'idle',
  label: 'Aguardando',
  receivedBytes: 0,
  totalBytes: 0,
  error: '',
})
const requestError = ref('')
let timer = null

const progress = computed(() => {
  if (!status.value.totalBytes) return 0
  return Math.min(
    100,
    Math.floor(
      (status.value.receivedBytes * 100) / status.value.totalBytes,
    ),
  )
})

const busy = computed(() =>
  ['connecting', 'downloading'].includes(status.value.state),
)

const detail = computed(() => {
  if (requestError.value) return requestError.value
  if (status.value.error) return status.value.error
  if (status.value.state !== 'downloading') return ''
  return `${progress.value}% — ${status.value.receivedBytes} / ${status.value.totalBytes} bytes`
})

async function refresh() {
  try {
    const response = await fetch('/api/v1/test-download/status', {
      cache: 'no-store',
    })
    if (!response.ok) throw new Error(`HTTP ${response.status}`)
    status.value = await response.json()
    requestError.value = ''
  } catch {
    requestError.value = 'Falha ao consultar o payload'
  }
}

async function startDownload() {
  requestError.value = ''
  try {
    const response = await fetch('/api/v1/test-download', {
      method: 'POST',
    })
    if (!response.ok) throw new Error(`HTTP ${response.status}`)
    await refresh()
  } catch {
    requestError.value = 'Não foi possível iniciar o download'
  }
}

onMounted(() => {
  refresh()
  timer = window.setInterval(refresh, 1000)
})

onBeforeUnmount(() => {
  if (timer) window.clearInterval(timer)
})
</script>

<template>
  <main class="screen">
    <section class="panel">
      <div class="badge">VUE TEST</div>
      <h1>EZHELIT Store</h1>
      <p class="subtitle">Prova de interface Vue no PS5</p>

      <button :disabled="busy" type="button" @click="startDownload">
        {{ busy ? 'Download em andamento' : 'Baixar arquivo de teste' }}
      </button>

      <div class="status">{{ status.label }}</div>

      <div v-if="busy || status.state === 'completed'" class="progress-track">
        <div class="progress-value" :style="{ width: `${progress}%` }" />
      </div>

      <div class="detail">{{ detail }}</div>
    </section>
  </main>
</template>
