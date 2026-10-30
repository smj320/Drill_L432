# Memo

## printfをUART2に接続

```c++
/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    // ここに周期割込で実行したい処理を書く
    // 例: フラグを立てる、GPIOを反転させるなど
  }
}
/* USER CODE END 4 */
```

## SysTicの変更

cubemxでコード生成するときに警告画出るので、
FreeRTOS側でタイマーをSysTickからTM6に変更する。

## Call と Ansが入れ替わることがある。

理由は謎。IDを入れ替えて対処するしかない。なかなかハンドシェークしないので
詰んだと思った。
