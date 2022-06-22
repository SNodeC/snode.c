<script setup lang="ts">
import { onMounted, Ref, ref } from 'vue';
import './styles/app.scss'
import { authorizationService } from './services/authorization.service'
import { resourceService} from './services/resource.service'

const authServerUrl: string = 'http://localhost:8082/oauth2'
const clientId: string = 'bf0aed2a-ecfc-11ec-97b9-08002771075f'
const redirectUri: string = 'http://localhost:8081/oauth2'
const state: string = 'defaultstate'
const scope: string = 'defaultscope'

let authRequestUri: string = `${authServerUrl}/authorize?response_type=code`
authRequestUri += `&client_id=${clientId}`
authRequestUri += `&redirect_uri=${redirectUri}`
authRequestUri += `&state=${state}`
authRequestUri += `&scope=${scope}`

const showInitiazeLink: Ref<boolean> = ref(true)
const content: Ref<string | null> = ref(null)

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
    const tokenResponse = await authorizationService.requestToken(queryParams.get('code')!, clientId, redirectUri)
    if (tokenResponse == null) {
      console.log('No token response from authorization server')
      return
    }
    console.log('Token response from authorization server:')
    console.log(tokenResponse)
    const resourceResponse = await resourceService.accessResource(tokenResponse.access_token, clientId)
    console.log('Resource response from resource server:', resourceResponse.content)
    content.value = resourceResponse.content
  } catch (error) {
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
    <article v-if="content">
      <h2>Content from the resource server</h2>
      <p>{{content}}</p>
    </article>
  </div>
</template>

<style>
</style>
