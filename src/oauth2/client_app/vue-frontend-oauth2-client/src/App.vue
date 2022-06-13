<script setup lang="ts">
import { onMounted } from 'vue';
import './styles/app.scss'

const AUTHORIZATION_SERVER_URL: string = 'http://localhost:8082/'
const CLIENT_ID: string = '911a821a-ea2d-11ec-8e2e-08002771075f'
const REDIRECT_URI: string = 'http://localhost:8081/oauth2'
const STATE: string = 'defaultstate'
const SCOPE: string = 'defaultscope'

const AUTH_REQUEST_URI: string = `${AUTHORIZATION_SERVER_URL}/authorize?client_id=${CLIENT_ID}&redirect_uri=${REDIRECT_URI}&state=${STATE}&scope=${SCOPE}`

async function initiateOAuth2(): Promise<void> {
  try {
    await fetch(AUTH_REQUEST_URI, {
      method: 'post',
    })
  } catch (error: unknown) {
    console.error(error);
  }
}

onMounted(() => {
  document.title = 'OAuth2 Client App'
})
</script>

<template>
  <div class="container">
    <h1>OAuth2 Client</h1>
    <a :href="AUTH_REQUEST_URI" @click.prevent="initiateOAuth2()">
      Connect Your Account
    </a>
  </div>
</template>

<style>
</style>
