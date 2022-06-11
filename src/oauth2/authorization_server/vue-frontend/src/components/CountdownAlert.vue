<script setup lang="ts">
import { ref, Ref } from 'vue';

const props = defineProps<{
  countdownSeconds: number
}>()

const emit = defineEmits<{
  (event: 'countdownReached'): void
}>()

defineExpose({
  startCountdown,
})

const remainingSeconds: Ref<number> = ref(0)

function startCountdown(): void {
  remainingSeconds.value = props.countdownSeconds
  const intervalId: number = setInterval(() => {
    remainingSeconds.value -= 1
    if (remainingSeconds.value === 0) {
      clearInterval(intervalId)
      emit('countdownReached')
    }
  }, 1000)
}

</script>

<template>
  <div class="alert">
    <p>Login successful. You will be redirected in</p>
    <p class="timer">{{ remainingSeconds }}</p>
    <p>seconds.</p>
  </div>
</template>

<style scoped>
.alert {
  flex: 1;
  padding: 1em 1.5em;
  margin-bottom: 1em;
  color: var(--c-text-light);
  border-radius: var(--border-radius-s);
  background-color: var(--c-info);
}

.timer {
  font-size: xx-large;
}
</style>
