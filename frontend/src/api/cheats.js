const API_BASE = '/api/v1'

export class ApiError extends Error {
  constructor(status, code, message) {
    super(message)
    this.name = 'ApiError'
    this.status = status
    this.code = code
  }
}

async function readResponse(response) {
  let body = null
  try {
    body = await response.json()
  } catch {
    // A malformed error response is still represented by its HTTP status.
  }
  if (!response.ok) {
    throw new ApiError(
      response.status,
      body?.error || 'request_failed',
      body?.message || `O serviço respondeu com HTTP ${response.status}.`,
    )
  }
  return body
}

export async function getCheatState() {
  const response = await fetch(`${API_BASE}/cheats`, { cache: 'no-store' })
  return readResponse(response)
}

export async function setCheatEnabled(id, enabled) {
  const response = await fetch(`${API_BASE}/cheats/${encodeURIComponent(id)}`, {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
      'X-EZ-Cheats-Request': '1',
    },
    body: JSON.stringify({ enabled }),
  })
  return readResponse(response)
}
