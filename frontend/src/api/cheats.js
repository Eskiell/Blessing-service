const API_BASE = '/api/v1'

export async function getCheatState() {
  const response = await fetch(`${API_BASE}/cheats`, { cache: 'no-store' })
  if (!response.ok) throw new Error(`HTTP ${response.status}`)
  return response.json()
}

export async function setCheatEnabled(id, enabled) {
  const response = await fetch(`${API_BASE}/cheats/${encodeURIComponent(id)}`, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled }),
  })
  if (!response.ok) throw new Error(`HTTP ${response.status}`)
  return response.json()
}
