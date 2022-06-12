<script setup lang="ts">
import './styles/app.scss'
import { onMounted, ref, Ref } from 'vue'
import { apiService } from './services/api.service'
import ErrorAlert from './components/ErrorAlert.vue'
import LoadingSpinner from './components/LoadingSpinner.vue'
import CountdownAlert from './components/CountdownAlert.vue'

let clientId: string | null = null;
let clientRedirectUri: string | null = null
const emailInput: Ref<string> = ref('')
const emailError: Ref<string | null> = ref(null)
const passwordInput: Ref<string> = ref('')
const passwordError: Ref<string | null> = ref(null)
const errors: Ref<string[]> = ref([])
const loading: Ref<boolean> = ref(false)
const countdownAlert = ref()
const showCountdownAlert: Ref<boolean> = ref(false)
const countdownLive: Ref<boolean> = ref(false)

async function login(): Promise<void> {
  const email: string = emailInput.value.trim()
  const password: string = passwordInput.value
  emailError.value = null
  passwordError.value = null
  if (!email || !password || clientId == null) {
    if (!email) {
      emailError.value = 'Please enter your email address.'
    }
    if (!password) {
      passwordError.value = 'Please enter your password.'
    }
    return;
  }
  loading.value = true
  try {
    const response = await apiService.login(email, password, clientId)
    loading.value = false
    clientRedirectUri = response.redirect_uri
    showLoginSuccessAndRedirect()
  } catch (e: unknown) {
    console.error(e)
    showError('Failed to reach the server')
    loading.value = false
  }
}

function showError(error: string): void {
  errors.value = [error]
}

function clearErrors(): void {
  errors.value = []
}

function addError(error: string): void {
  errors.value = [...errors.value, error];
}

function getClientIdFromQuery(url: string): string {
  const splitByClientId: string[] = url.split("client_id=")
  if (splitByClientId.length < 2) {
    throw new Error(`No 'client_id' in query found!`)
  }
  return splitByClientId[1].split("&")[0]
}

function showLoginSuccessAndRedirect(): void {
  showCountdownAlert.value = true
  countdownAlert.value.startCountdown(5)
  countdownLive.value = true
}

function onCountdownReached(): void {
  if (clientRedirectUri != null) {
    window.location.href = clientRedirectUri
    // countdownLive.value = false
  }
}

onMounted(() => {
  document.title = 'Login to Authorization Server'
  clientId = getClientIdFromQuery(window.location.href)
  console.log(`Found 'client_id' in url: '${clientId}'`)
})
</script>

<template>
  <div class="container">
    <main>
      <h1>Login to Authorization Server</h1>
      <form @submit.prevent="login()">
        <div v-if="errors.length > 0" class="row">
          <ErrorAlert v-for="error in errors" :text="error" />
        </div>
        <div v-show="showCountdownAlert" class="row">
          <CountdownAlert :countdown-seconds="5" ref="countdownAlert" @countdown-reached="onCountdownReached()" />
        </div>
        <div class="row input-group">
          <label for="input-email">Email</label>
          <input type="email" name="email" id="input-email" v-model="emailInput" autocomplete="off"
            placeholder="Enter your email address" required :disabled="countdownLive">
          <div v-if="emailError != null" class="input-error">{{ emailError }}</div>
        </div>
        <div class="row input-group">
          <label for="input-password">Password</label>
          <input type="password" name="password" id="input-password" v-model="passwordInput"
            placeholder="Enter your password" required :disabled="countdownLive">
          <div v-if="passwordError != null" class="input-error">{{ passwordError }}</div>
        </div>
        <div class="row">
          <button id="login-button" type="submit" :disabled="loading || countdownLive">
            <div>Login</div>
            <LoadingSpinner v-show="loading" />
          </button>
        </div>
      </form>
    </main>
  </div>
</template>

<style lang="scss">
.container {
  max-width: 75ch;
  margin-inline: auto;
  flex-direction: column;
  padding-top: 3em;
}

main {
  padding-inline: 1em;
}

.input-group {
  display: flex;
  flex-direction: column;

  .input-error {
    color: var(--c-error);
  }
}

#login-button {
  margin-top: .5em;
}
</style>
