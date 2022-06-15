<script setup lang="ts">
import { onMounted, Ref, ref } from 'vue';
import './styles/app.scss'
import { authServerService } from './services/authServer.service'

const authServerUrl: string = 'http://localhost:8082/oauth2'
const clientId: string = '911a821a-ea2d-11ec-8e2e-08002771075f'
const redirectUri: string = 'http://localhost:8081/oauth2'
const state: string = 'defaultstate'
const scope: string = 'defaultscope'

let authRequestUri: string = `${authServerUrl}/authorize?response_type=code`
authRequestUri += `&client_id=${clientId}`
authRequestUri += `&redirect_uri=${redirectUri}`
authRequestUri += `&state=${state}`
authRequestUri += `&scope=${scope}`

const showInitiazeLink: Ref<boolean> = ref(true)

async function initiateOAuth2(): Promise<void> {
  window.location.href = authRequestUri
  // try {
  //   await fetch(AUTH_REQUEST_URI, {
  //     method: 'get',
  //   })
  // } catch (error: unknown) {
  //   console.error(error);
  // }
}

function getQueryParamsFromUrl(url: string): Map<string, string> {
  const queryParams: Map<string, string> = new Map()
  const splitUrl: string[] = url.split('?')
  if (splitUrl.length > 1) {
    const paramString: string = splitUrl[1]
    const paramKeyValuePairs: string[] = paramString.split('&')
    paramKeyValuePairs.forEach((pair) => {
      const splitPair: string[] = pair.split('=')
      queryParams.set(splitPair[0], splitPair[1])
    })
  }
  return queryParams
}

async function checkUrlForAuthCode(): Promise<void> {
  const queryParams = getQueryParamsFromUrl(window.location.href)
  if (!queryParams.has('code') || queryParams.get('code') == null) {
    console.log(`No 'code' found in url.`)
    return
  }
  showInitiazeLink.value = false
  try {
    const response = await authServerService.requestToken(queryParams.get('code')!)
    if (response == null) {
      console.log('No token response from server')
    } else {
      console.log('token response from server:')
      console.log(response)
    }
  } catch (error: unknown) {
    console.error(error)
  }
}

onMounted(async () => {
  document.title = 'OAuth2 Client App'
  await checkUrlForAuthCode()
})
</script>

<template>
  <div class="container">
    <h1>OAuth2 Client</h1>
    <a v-if="showInitiazeLink" :href="authRequestUri" @click.prevent="initiateOAuth2()">
      Connect Your Account
    </a>
  </div>
</template>

<style>
</style>
