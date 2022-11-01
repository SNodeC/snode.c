<script setup lang="ts">
defineProps<{ text: string }>()

</script>

<template>
  <div class="alert">
    {{text}}
  </div>
</template>

<style scoped>
.alert {
  flex: 1;
  padding: 1em 1.5em;
  margin-bottom: 1em;
  color: var(--c-text-light);
  border-radius: var(--border-radius-s);
  background-color: var(--c-error);
}
</style>
