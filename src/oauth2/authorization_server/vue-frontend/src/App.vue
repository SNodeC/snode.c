<script setup lang="ts">
import './styles/app.scss'
import { ref, Ref } from 'vue'
import { apiService } from './services/api.service';
import ErrorAlert from './components/ErrorAlert.vue'
import LoadingSpinner from './components/LoadingSpinner.vue';

const emailInput: Ref<string> = ref('')
const emailError: Ref<string | null> = ref(null)
const passwordInput: Ref<string> = ref('')
const passwordError: Ref<string | null> = ref(null)
const errors: Ref<string[]> = ref([])
const loading: Ref<boolean> = ref(false)

async function login(): Promise<void> {
  const email: string = emailInput.value.trim()
  const password: string = passwordInput.value
  emailError.value = null
  passwordError.value = null
  if (!email || !password) {
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
    await apiService.login(email, password)
    loading.value = false
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
</script>

<template>
  <div class="container">
    <main>
      <h1>Login to Authorization Server</h1>
      <form @submit.prevent="login()">
        <div v-if="errors.length > 0" class="row">
          <ErrorAlert v-for="error in errors" :text="error" />
        </div>
        <div class="row input-group">
          <label for="input-email">Email</label>
          <input type="email" name="email" id="input-email" v-model="emailInput" autocomplete="off"
            placeholder="Enter your email address" required>
          <div v-if="emailError != null" class="input-error">{{ emailError }}</div>
        </div>
        <div class="row input-group">
          <label for="input-password">Password</label>
          <input type="password" name="password" id="input-password" v-model="passwordInput"
            placeholder="Enter your password" required>
          <div v-if="passwordError != null" class="input-error">{{ passwordError }}</div>
        </div>
        <div class="row">
          <button id="login-button" type="submit" :disabled="loading">
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
